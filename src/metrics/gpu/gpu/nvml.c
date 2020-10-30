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

#include <metrics/gpu/gpu/nvml.h>
//#define SHOW_DEBUGS 1
#ifdef CUDA_BASE

#include <nvml.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/types.h>
#include <common/monitor.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>

const char *nvml_names[] =
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
	"nvmlErrorString",
};

typedef struct nvml_s
{
	nvmlReturn_t (*Init)		(void);
	nvmlReturn_t (*DevCount)	(uint *deviceCount);
	nvmlReturn_t (*DevHandle)	(uint index, nvmlDevice_t *device);
	nvmlReturn_t (*DevMode)		(nvmlDevice_t device, nvmlEnableState_t *mode);
	nvmlReturn_t (*DevPower)	(nvmlDevice_t device, uint *power);
	nvmlReturn_t (*DevClocks)	(nvmlDevice_t device, nvmlClockType_t type, uint *clock);
	nvmlReturn_t (*DevTemp)		(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, uint *temp);
	nvmlReturn_t (*DevUtil)		(nvmlDevice_t device, nvmlUtilization_t *utilization);
	nvmlReturn_t (*DevProcs)	(nvmlDevice_t device, uint *infoCount, nvmlProcessInfo_t *infos);
	char* (*ErrorString)		(nvmlReturn_t result);
} nvml_t;

#define NVML_N 10

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static suscription_t *sus;
static uint initialized;
static uint dev_count;
static nvml_t nvml;
static gpu_t *pool;

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
		memset((void *) &nvml, 0, sizeof(nvml_t));
		dlclose(libnvml);
		return 0;
	}

	return 1;
}

static state_t nvml_load_library()
{
	if (load_test(getenv(HACK_FILE_NVML))) return 1;
	if (load_test(CUDA_BASE "/targets/x86_64-linux/lib/libnvidia-ml.so")) return 1;
	if (load_test(CUDA_BASE "/lib64/libnvidia-ml.so")) return 1;
	if (load_test(CUDA_BASE "/lib/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib64/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib/libnvidia-ml.so")) return 1;
	return 0;	
}

static state_t nvml_init_prime()
{
	nvmlReturn_t r;
	state_t s;
	while (pthread_mutex_trylock(&lock));
	if (initialized) {
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
	if (!nvml_load_library()) {
		return_msg(EAR_ERROR, Error.dlopen);
	}
	dev_count = 0;
	if ((r = nvml.Init()) != NVML_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return_msg(EAR_ERROR, (char *) nvml.ErrorString(r));
	}
	if ((r = nvml.DevCount(&dev_count)) != NVML_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return_msg(EAR_ERROR, (char *) nvml.ErrorString(r));
	}
	if (((int) dev_count) <= 0) {
		pthread_mutex_unlock(&lock);
		return_msg(EAR_ERROR, Error.gpus_not);
	}
	if (xtate_fail(s, nvml_data_alloc(&pool))) {
		nvml_data_free(&pool);
		pthread_mutex_unlock(&lock);
		return s;
	}
	sus = suscription();
	sus->call_main  = nvml_pool;
	sus->time_relax = 1000;
	sus->time_burst = 300;
	if (xtate_fail(s, monitor_register(sus))) {
		nvml_data_free(&pool);
		monitor_unregister(sus);
		pthread_mutex_unlock(&lock);
		return s;
	}
	initialized = 1;
	pthread_mutex_unlock(&lock);
	
	return EAR_SUCCESS;
}

state_t nvml_status()
{
	return nvml_init(NULL);
}

state_t nvml_init(ctx_t *c)
{
	return nvml_count(NULL, NULL);
}

state_t nvml_dispose(ctx_t *c)
{
	if (!initialized) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	return EAR_SUCCESS;
}

state_t nvml_count(ctx_t *c, uint *_dev_count)
{
	state_t s;

	if (_dev_count != NULL) {
		*_dev_count = 0;
	}
	if (!initialized) {
		if (xtate_fail(s, nvml_init_prime())) {
			return s;
		}
	}
	if (_dev_count != NULL) {
		*_dev_count = dev_count;
	}
	
	return EAR_SUCCESS;
}

state_t nvml_pool(void *p)
{
	if (!initialized) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}

	//
	timestamp_t time;
	int working = 0;
	int i;

	// Lock
	while (pthread_mutex_trylock(&lock));
	timestamp_getfast(&time);

	//
	for (i = 0; i < dev_count; ++i)
	{
		nvmlEnableState_t mode;
		nvmlDevice_t device;
		int s;

		// Testing if all is right
		if ((s = nvml.DevHandle(i, &device)) != NVML_SUCCESS) {
			continue;
		}
		if ((s = nvml.DevMode(device, &mode)) != NVML_SUCCESS) {
			continue;
		}
		if (mode != NVML_FEATURE_ENABLED) {
			continue;
		}

		nvmlProcessInfo_t procs[8];
		nvmlUtilization_t util;
		uint proc_count = 8;
		uint freq_gpu_mhz;
		uint freq_mem_mhz;
		uint temp_gpu;
		uint power_mw;

		// Getting the metrics by calling NVML (no MEM temp)
		s = nvml.DevPower(device, &power_mw);
		s = nvml.DevClocks(device, NVML_CLOCK_MEM, &freq_mem_mhz);
		s = nvml.DevClocks(device, NVML_CLOCK_SM , &freq_gpu_mhz);
		s = nvml.DevTemp(device, NVML_TEMPERATURE_GPU, &temp_gpu);
		s = nvml.DevUtil(device, &util);
		s = nvml.DevProcs(device, &proc_count, procs);

		// Pooling the data
		pool[i].samples      += 1;
		pool[i].time          = time;
		pool[i].freq_mem_mhz += (ulong) freq_mem_mhz;
		pool[i].freq_gpu_mhz += (ulong) freq_gpu_mhz;
		pool[i].util_mem     += (ulong) util.memory;
		pool[i].util_gpu     += (ulong) util.gpu;
		pool[i].temp_gpu     += (ulong) temp_gpu;
		pool[i].temp_mem      = 0;
		pool[i].energy_j      = 0;
		pool[i].power_w      += (double) power_mw;
		pool[i].working       = proc_count > 0;
		pool[i].correct       = 1;

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
	if (!initialized) {
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

static void nvml_read_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff, int i)
{
	gpu_t *d3 = &data_diff[i];
	gpu_t *d2 = &data2[i];
	gpu_t *d1 = &data1[i];
	ullong time_i;
	double time_f;

	// Cleaning
	nvml_data_null(d3);

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
	d3->freq_gpu_mhz = (d2->freq_gpu_mhz - d1->freq_gpu_mhz) / d3->samples;
	d3->freq_mem_mhz = (d2->freq_mem_mhz - d1->freq_mem_mhz) / d3->samples;
	d3->util_gpu     = (d2->util_gpu     - d1->util_gpu)     / d3->samples;
	d3->util_mem     = (d2->util_mem     - d1->util_mem)     / d3->samples;
	d3->temp_gpu     = (d2->temp_gpu     - d1->temp_gpu)     / d3->samples;
	d3->temp_mem     = 0;
	d3->power_w      = (d2->power_w      - d1->power_w )     / (d3->samples * 1000);
	d3->energy_j     = (d3->power_w)     * time_f;
	d3->working      = (d2->working);
	//
#if 0
	debug("%d freq gpu (MHz), %lu = %lu - %lu", i, d3->freq_gpu_mhz, d2->freq_gpu_mhz, d1->freq_gpu_mhz);
	debug("%d freq mem (MHz), %lu = %lu - %lu", i, d3->freq_mem_mhz, d2->freq_mem_mhz, d1->freq_mem_mhz);
	debug("%d power    (W)  , %0.2lf = %0.2lf - %0.2lf", i, d3->power_w, d2->power_w, d1->power_w);
	debug("%d energy   (J)  , %0.2lf = %0.2lf - %0.2lf", i, d3->energy_j, d2->energy_j, d1->energy_j);
#endif
}

state_t nvml_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	int i;
	if (data2 == NULL || data1 == NULL || data_diff == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	for (i = 0; i < dev_count; i++) {
		nvml_read_diff(data2, data1, data_diff, i);
	}
	#ifdef SHOW_DEBUGS
	nvml_data_print(data_diff, debug_channel);
	#endif
	return EAR_SUCCESS;
}

state_t nvml_data_init(uint _dev_count)
{
	dev_count = _dev_count;
	return EAR_SUCCESS;
}

state_t nvml_data_alloc(gpu_t **data)
{
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
	if (data == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	memset(data, 0, dev_count * sizeof(gpu_t));
	return EAR_SUCCESS;
}

state_t nvml_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
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

	for (i = 0; i < dev_count; ++i)
	{
		dprintf(fd, "gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
		i                   ,
		data[i].energy_j    , data[i].power_w,
		data[i].freq_gpu_mhz, data[i].freq_mem_mhz,
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

	for (i = 0; i < dev_count && length > 0; ++i)
	{
		s = snprintf(&buffer[accuml], length,
			"gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
			i                   ,
			data[i].energy_j    , data[i].power_w,
			data[i].freq_gpu_mhz, data[i].freq_mem_mhz,
			data[i].util_gpu    , data[i].util_mem,
			data[i].temp_gpu    , data[i].temp_mem,
			data[i].working     , data[i].samples);
		length = length - s;
		accuml = accuml + s;
	}
	return EAR_SUCCESS;
}

#else

state_t nvml_status() { return EAR_ERROR; }
state_t nvml_init(ctx_t *c) { return EAR_ERROR; }
state_t nvml_dispose(ctx_t *c) { return EAR_ERROR; }
state_t nvml_count(ctx_t *c, uint *_dev_count) { return EAR_ERROR; }
state_t nvml_pool(void *p) { return EAR_ERROR; }
state_t nvml_read(ctx_t *c, gpu_t *data) { return EAR_ERROR; }
state_t nvml_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff) { return EAR_ERROR; }
state_t nvml_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff) { return EAR_ERROR; }
state_t nvml_data_init(uint _dev_count) { return EAR_ERROR; }
state_t nvml_data_alloc(gpu_t **data) { return EAR_ERROR; }
state_t nvml_data_free(gpu_t **data) { return EAR_ERROR; }
state_t nvml_data_null(gpu_t *data) { return EAR_ERROR; }
state_t nvml_data_copy(gpu_t *data_dst, gpu_t *data_src) { return EAR_ERROR; }
state_t nvml_data_print(gpu_t *data, int fd) { return EAR_ERROR; }
state_t nvml_data_tostr(gpu_t *data, char *buffer, int length) { return EAR_ERROR; }

#endif
