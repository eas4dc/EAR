/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifdef CUDA_BASE
#include <nvml.h>
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/types.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <metrics/gpu/archs/nvml.h>

#ifndef CUDA_BASE
typedef int nvmlDevice_t;
typedef int nvmlEnableState_t;
typedef int nvmlClockType_t;
typedef int nvmlTemperatureSensors_t;
typedef int nvmlProcessInfo_t;
typedef int nvmlClockId_t;
typedef int nvmlReturn_t;
typedef struct nvmlUtilization_s {
	int memory;
	int gpu;
} nvmlUtilization_t;

#define NVML_PATH            ""
#define NVML_SUCCESS         0
#define NVML_FEATURE_ENABLED 0
#define NVML_CLOCK_MEM       0
#define NVML_CLOCK_SM        0
#define NVML_TEMPERATURE_GPU 0
#else
#define NVML_PATH            CUDA_BASE
#endif


static const char *nvml_names[] =
{
	"nvmlInit_v2",
	"nvmlDeviceGetCount_v2",
	"nvmlDeviceGetHandleByIndex_v2",
	"nvmlDeviceGetPowerManagementMode",
	"nvmlDeviceGetPowerUsage",
	"nvmlDeviceGetClockInfo",
	"nvmlDeviceGetTemperature",
	"nvmlDeviceGetUtilizationRates",
	"nvmlDeviceGetComputeRunningProcesses",
	"nvmlDeviceGetSerial",
	"nvmlErrorString",
};

static struct nvml_s
{
	nvmlReturn_t (*Init)	(void);
	nvmlReturn_t (*Count)	(uint *devCount);
	nvmlReturn_t (*Handle)	(uint index, nvmlDevice_t *device);
	nvmlReturn_t (*Mode)	(nvmlDevice_t dev, nvmlEnableState_t *mode);
	nvmlReturn_t (*Power)	(nvmlDevice_t dev, uint *power);
	nvmlReturn_t (*Clocks)	(nvmlDevice_t dev, nvmlClockType_t type, uint *clock);
	nvmlReturn_t (*Temp)	(nvmlDevice_t dev, nvmlTemperatureSensors_t type, uint *temp);
	nvmlReturn_t (*Util)	(nvmlDevice_t dev, nvmlUtilization_t *utilization);
	nvmlReturn_t (*Procs)	(nvmlDevice_t dev, uint *infoCount, nvmlProcessInfo_t *infos);
	nvmlReturn_t (*Serials) (nvmlDevice_t dev, char *serial, uint length);
	char* (*ErrorString)	(nvmlReturn_t result);
} nvml;

#define NVML_N 10

//static nvml_t nvml;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static suscription_t *sus;
static uint           dev_count;
static gpu_t         *pool;
static nvmlDevice_t  *devices;
static uint           ok_status;
static uint           ok_unprivileged;
static uint           ok_init;

static struct error_s {
	char *init;
	char *init_not;
	char *null_context;
	char *null_data;
	char *gpus_not;
	char *dlopen;
} Error = {
	.init         = "context already initialized or not empty",
	.init_not     = "context not initialized",
	.null_context = "context pointer is NULL",
	.null_data    = "data pointer is NULL",
	.gpus_not     = "no GPUs detected",
	.dlopen       = "error during dlopen",
};
	
#define myErrorString(r) \
	((char *) nvml.ErrorString(r))

static state_t static_status_undo(char *error)
{
	#define static_free(v) \
		if (v != NULL) { free(v); v = NULL; }

	static_free(devices);

	return_msg(EAR_ERROR, error);
}

static state_t static_status()
{
	nvmlReturn_t r;
	int d;

	#ifndef CUDA_BASE
	return EAR_ERROR;
	#endif
	if ((r = nvml.Init()) != NVML_SUCCESS) {
		return_msg(EAR_ERROR, (char *) nvml.ErrorString(r));
	}
	if ((r = nvml.Count(&dev_count)) != NVML_SUCCESS) {
		return_msg(EAR_ERROR, (char *) nvml.ErrorString(r));
	}
	if (((int) dev_count) <= 0) {
		return_msg(EAR_ERROR, Error.gpus_not);
	}
	if ((devices = calloc(dev_count, sizeof(nvmlDevice_t))) == NULL) {
		return_msg(EAR_SYSCALL_ERROR, strerror(errno));
	}
	for (d = 0; d < dev_count; ++d) {
		if ((r = nvml.Handle(d, &devices[d])) != NVML_SUCCESS) {
			debug("nvmlDeviceGetHandleByIndex returned %d (%s)",
				  r, nvml.ErrorString(r));
			return static_status_undo(myErrorString(r));
		}
	}
	return EAR_SUCCESS;
}

static int load_test(char *path)
{
	void **p = (void **) &nvml;
	void *libnvml;
	int error;
	int i;

	//
	debug("trying to access to '%s'", path);
	if (access(path, X_OK) != 0) {
		return 0;
	}
	if ((libnvml = dlopen(path, RTLD_NOW | RTLD_LOCAL)) == NULL) {
		debug("dlopen fail");
		return 0;
	}
	debug("dlopen ok");

	//
	symplug_join(libnvml, (void **) &nvml, nvml_names, NVML_N);

	for(i = 0, error = 0; i < NVML_N; ++i) {
		debug("symbol %s: %d", nvml_names[i], (p[i] != NULL));
		error += (p[i] == NULL);
	}
	if (error > 0) {
		memset((void *) &nvml, 0, sizeof(nvml));
		dlclose(libnvml);
		return 0;
	}

	return 1;
}

static int static_load()
{
	// Looking for nvidia library in tipical paths.
	if (load_test(getenv(HACK_FILE_NVML))) return 1;
	if (load_test(NVML_PATH "/targets/x86_64-linux/lib/libnvidia-ml.so")) return 1;
	if (load_test(NVML_PATH "/lib64/libnvidia-ml.so")) return 1;
	if (load_test(NVML_PATH "/lib/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib64/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib/x86_64-linux-gnu/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib32/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib/libnvidia-ml.so")) return 1;
	return 0;
}

state_t nvml_status()
{
	state_t s;
	while (pthread_mutex_trylock(&lock));
	if (ok_status) {
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
	if (!static_load()) {
		pthread_mutex_unlock(&lock);
		return_msg(EAR_ERROR, Error.dlopen);
	}
	dev_count = 0;
	if (xtate_fail(s, static_status())) {
		pthread_mutex_unlock(&lock);
		return s;
	}
	ok_status = 1;
	debug("OK status");
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

static state_t static_init()
{
	state_t s;

	if (xtate_fail(s, nvml_data_alloc(&pool))) {
		nvml_data_free(&pool);
		return s;
	}
	// Initializing monitoring thread suscription.
	sus = suscription();
	sus->call_main  = nvml_pool;
	sus->time_relax = 1000;
	sus->time_burst = 300;
	// Initializing monitoring thread.
	if (xtate_fail(s, monitor_register(sus))) {
		nvml_data_free(&pool);
		monitor_unregister(sus);
		debug("GPU monitor FAILS");
		return s;
	}

	return EAR_SUCCESS;
}

state_t nvml_init(ctx_t *c)
{
	state_t s;
	if (xtate_fail(s, nvml_init_unprivileged(c))) {
		return s;
	}
	while (pthread_mutex_trylock(&lock));
	if (ok_init) {
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
	if (xtate_fail(s, static_init())) {
		pthread_mutex_unlock(&lock);
		return s;
	}
	ok_init = 1;
	debug("OK init");
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t nvml_init_unprivileged(ctx_t *c)
{
	ok_unprivileged = 1;
	debug("OK init unprivileged");
	return EAR_SUCCESS;
}

state_t nvml_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t nvml_count(ctx_t *c, uint *dev_count_in)
{
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (dev_count_in != NULL) {
		*dev_count_in = dev_count;
	}
	return EAR_SUCCESS;
}

state_t nvml_get_info(ctx_t *c, const gpu_info_t **info_in, uint *dev_count_in)
{
	// 128 GPUs is enough for today and probably forever
	static gpu_info_t info[128];
	char serial[32];
	nvmlReturn_t r;
	int i;

	for (i = 0; i < dev_count; ++i) { 
		if ((r = nvml.Serials(devices[i], serial, 32)) != NVML_SUCCESS) {
			return_msg(EAR_ERROR, myErrorString(r));
		}
		info[i].serial = (ullong) atoll(serial);
		info[i].index  = i;
	}
	*info_in = info;

	if (dev_count_in != NULL) {
		*dev_count_in = dev_count;
	}

	return EAR_SUCCESS;
}

static int static_read(int i, gpu_t *metric)
{
	nvmlEnableState_t mode;
	int s;

	// Cleaning
	memset(metric, 0, sizeof(gpu_t));

	// Testing if all is right
	if ((s = nvml.Mode(devices[i], &mode)) != NVML_SUCCESS) {
		return 0;
	}
	if (mode != NVML_FEATURE_ENABLED) {
		return 0;
	}

	nvmlProcessInfo_t procs[8];
	nvmlUtilization_t util;
	uint proc_count = 8;
	uint freq_gpu_mhz;
	uint freq_mem_mhz;
	uint temp_gpu;
	uint power_mw;

	// Getting the metrics by calling NVML (no MEM temp)
	s = nvml.Power(devices[i], &power_mw);
	s = nvml.Clocks(devices[i], NVML_CLOCK_MEM, &freq_mem_mhz);
	s = nvml.Clocks(devices[i], NVML_CLOCK_SM , &freq_gpu_mhz);
	s = nvml.Temp(devices[i], NVML_TEMPERATURE_GPU, &temp_gpu);
	s = nvml.Util(devices[i], &util);
	s = nvml.Procs(devices[i], &proc_count, procs);

	// Pooling the data (time is not set here)
	metric->samples  = 1;
	metric->freq_mem = (ulong) freq_mem_mhz * 1000LU;
	metric->freq_gpu = (ulong) freq_gpu_mhz * 1000LU;
	metric->util_mem = (ulong) util.memory;
	metric->util_gpu = (ulong) util.gpu;
	metric->temp_gpu = (ulong) temp_gpu;
	metric->temp_mem = 0;
	metric->energy_j = 0;
	metric->power_w  = (double) power_mw;
	metric->working  = proc_count > 0;
	metric->correct  = 1;

	return 1;
}

state_t nvml_pool(void *p)
{
	//
	timestamp_t time;
	gpu_t metric;
	int working;
	int i;

	if (!ok_init) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}

	//
	working = 0;

	// Lock
	while (pthread_mutex_trylock(&lock));
	timestamp_getfast(&time);

	//
	for (i = 0; i < dev_count; ++i)
	{
		if (!static_read(i, &metric)) {
			continue;
		}

		// Pooling the data
		pool[i].time      = time;
		pool[i].samples  += metric.samples;
		pool[i].freq_mem += metric.freq_mem;
		pool[i].freq_gpu += metric.freq_gpu;
		pool[i].util_mem += metric.util_mem;
		pool[i].util_gpu += metric.util_gpu;
		pool[i].temp_gpu += metric.temp_gpu;
		pool[i].temp_mem += metric.temp_mem;
		pool[i].energy_j  = metric.energy_j;
		pool[i].power_w  += metric.power_w;
		pool[i].working   = metric.working;
		pool[i].correct   = metric.correct;

		// Burst or not
		working += pool[i].working;
	}
	
	// Lock
	pthread_mutex_unlock(&lock);
		
	if (working  > 0 && !monitor_is_bursting(sus)) {
		debug("bursting");
		monitor_burst(sus);
	}
	if (working == 0 &&  monitor_is_bursting(sus)) {
		debug("relaxing");
		monitor_relax(sus);
	}

	return EAR_SUCCESS;
}

state_t nvml_read(ctx_t *c, gpu_t *data)
{
	if (!ok_init) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	return nvml_data_copy(data, pool);
}

state_t nvml_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (xtate_fail(s, nvml_read(c, data2))) {
		return s;
	}
	if (xtate_fail(s, nvml_data_diff(data2, data1, data_diff))) {
		return s;
	}
	return nvml_data_copy(data1, data2);
}

state_t nvml_read_raw(ctx_t *c, gpu_t *data)
{
	timestamp_t time;
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	timestamp_getfast(&time);
	for (i = 0; i < dev_count; ++i) {
		static_read(i, &data[i]);
		data[i].time     = time;
		data[i].power_w /= 1000;
	}
	return EAR_SUCCESS;
}

static void nvml_read_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff, int i)
{
	gpu_t *d3 = &data_diff[i];
	gpu_t *d2 = &data2[i];
	gpu_t *d1 = &data1[i];
	ullong time_i;
	double time_f;

	#if 0
	debug("%d freq gpu (MHz), %lu = %lu - %lu", i, d3->freq_gpu_mhz, d2->freq_gpu_mhz, d1->freq_gpu_mhz);
	debug("%d freq mem (MHz), %lu = %lu - %lu", i, d3->freq_mem_mhz, d2->freq_mem_mhz, d1->freq_mem_mhz);
	debug("%d power    (W)  , %0.2lf = %0.2lf - %0.2lf", i, d3->power_w, d2->power_w, d1->power_w);
	debug("%d energy   (J)  , %0.2lf = %0.2lf - %0.2lf", i, d3->energy_j, d2->energy_j, d1->energy_j);
	#endif
	if (d2->correct != 1 || d1->correct != 1) {
		return;
	}
	// Computing time
	time_i = timestamp_diff(&d2->time, &d1->time, TIME_USECS);
	time_f = ((double) time_i) / 1000000.0;
	// Averages
	d3->samples = d2->samples - d1->samples;

	if (d3->samples == 0) {
		memset(d3, 0, sizeof(gpu_t));
		return;
	}
	// No overflow control is required (64-bits are enough)
	d3->freq_gpu = (d2->freq_gpu - d1->freq_gpu) / d3->samples;
	d3->freq_mem = (d2->freq_mem - d1->freq_mem) / d3->samples;
	d3->util_gpu = (d2->util_gpu - d1->util_gpu) / d3->samples;
	d3->util_mem = (d2->util_mem - d1->util_mem) / d3->samples;
	d3->temp_gpu = (d2->temp_gpu - d1->temp_gpu) / d3->samples;
	d3->temp_mem = (d2->temp_mem - d1->temp_mem) / d3->samples;
	d3->power_w  = (d2->power_w  - d1->power_w ) / (d3->samples * 1000);
	d3->energy_j = (d3->power_w) * time_f;
	d3->working  = (d2->working);
	d3->correct  = 1;
	//
#if 0
	debug("%d freq gpu (MHz), %lu = %lu - %lu", i, d3->freq_gpu, d2->freq_gpu, d1->freq_gpu);
	debug("%d freq mem (MHz), %lu = %lu - %lu", i, d3->freq_mem, d2->freq_mem, d1->freq_mem);
	debug("%d power    (W)  , %0.2lf = %0.2lf - %0.2lf", i, d3->power_w, d2->power_w, d1->power_w);
	debug("%d energy   (J)  , %0.2lf = %0.2lf - %0.2lf", i, d3->energy_j, d2->energy_j, d1->energy_j);
#endif
}

state_t nvml_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (data2 == NULL || data1 == NULL || data_diff == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	if (xtate_fail(s, nvml_data_null(data_diff))) {
		return s;
	}
	for (i = 0; i < dev_count; i++) {
		nvml_read_diff(data2, data1, data_diff, i);
	}
	#ifdef SHOW_DEBUGS
	nvml_data_print(data_diff, debug_channel);
	#endif
	return EAR_SUCCESS;
}

state_t nvml_data_alloc(gpu_t **data)
{
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (data == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	*data = calloc(dev_count, sizeof(gpu_t));
	if (*data == NULL) {
		return_msg(EAR_SYSCALL_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t nvml_data_free(gpu_t **data)
{
	if (data != NULL) {
		free(*data);
	}
	return EAR_SUCCESS;
}

state_t nvml_data_null(gpu_t *data)
{
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (data == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	memset(data, 0, dev_count * sizeof(gpu_t));
	return EAR_SUCCESS;
}

state_t nvml_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (data_dst == NULL || data_src == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	while (pthread_mutex_trylock(&lock));
	memcpy(data_dst, data_src, dev_count * sizeof(gpu_t));
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t nvml_data_print(gpu_t *data, int fd)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0; i < dev_count; ++i)
	{
		dprintf(fd, "gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
		i                   ,
		data[i].energy_j    , data[i].power_w,
		data[i].freq_gpu    , data[i].freq_mem,
		data[i].util_gpu    , data[i].util_mem,
		data[i].temp_gpu    , data[i].temp_mem,
		data[i].working     , data[i].samples);
	}
	return EAR_SUCCESS;
}

state_t nvml_data_tostr(gpu_t *data, char *buffer, int length)
{
	int accuml = 0;
	size_t s;
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0; i < dev_count && length > 0; ++i)
	{
		s = snprintf(&buffer[accuml], length,
			"gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu, %lu, %lu, %lu, %u, %lu correct=%u\n",
			i                   ,
			data[i].energy_j    , data[i].power_w,
			data[i].freq_gpu    , data[i].freq_mem,
			data[i].util_gpu    , data[i].util_mem,
			data[i].temp_gpu    , data[i].temp_mem,
			data[i].working     , data[i].samples,
			data[i].correct);
		length = length - s;
		accuml = accuml + s;
	}
	return EAR_SUCCESS;
}

state_t nvml_data_merge(gpu_t *data_diff, gpu_t *data_merge)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (data_diff == NULL || data_merge == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	// Cleaning
	memset((void *) data_merge, 0, sizeof(gpu_t));
	// Accumulators: power and energy
	for (i = 0; i < dev_count; ++i)
	{
		data_merge->freq_mem += data_diff[i].freq_mem;
		data_merge->freq_gpu += data_diff[i].freq_gpu;
		data_merge->util_mem += data_diff[i].util_mem;
		data_merge->util_gpu += data_diff[i].util_gpu;
		data_merge->temp_gpu += data_diff[i].temp_gpu;
		data_merge->temp_mem += data_diff[i].temp_mem;
		data_merge->energy_j += data_diff[i].energy_j;
		data_merge->power_w  += data_diff[i].power_w;
	}
	// Static
	data_merge->time          = data_diff[0].time;
	data_merge->samples       = data_diff[0].samples;
	// Averages
	data_merge->freq_mem /= dev_count;
	data_merge->freq_gpu /= dev_count;
	data_merge->util_mem /= dev_count;
	data_merge->util_gpu /= dev_count;
	data_merge->temp_gpu /= dev_count;
	data_merge->temp_mem /= dev_count;
	
	return EAR_SUCCESS;
}
