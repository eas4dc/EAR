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

//#define SHOW_DEBUGS 1

#ifdef CUDA_BASE
#include <nvml.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <management/gpu/archs/nvml.h>

#define NVML_N 16

#ifndef CUDA_BASE
typedef int nvmlDevice_t;
typedef int nvmlEnableState_t;
typedef int nvmlClockType_t;
typedef int nvmlClockId_t;
typedef int nvmlReturn_t;
typedef int nvmlDriverModel_t;

#define NVML_PATH            ""
#define NVML_SUCCESS         0
#define NVML_CLOCK_GRAPHICS  0
#define NVML_CLOCK_ID_APP_CLOCK_TARGET 0
#else
#define NVML_PATH            CUDA_BASE
#endif

static const char *nvml_names[] =
{
	"nvmlInit_v2",
	"nvmlDeviceGetCount_v2",
	"nvmlDeviceGetHandleByIndex_v2",
	"nvmlDeviceGetDefaultApplicationsClock",
	"nvmlDeviceGetSupportedMemoryClocks",
	"nvmlDeviceGetSupportedGraphicsClocks",
	"nvmlDeviceGetClock",
	"nvmlDeviceSetGpuLockedClocks",
	"nvmlDeviceSetApplicationsClocks",
	"nvmlDeviceResetApplicationsClocks",
	"nvmlDeviceResetGpuLockedClocks",
	"nvmlDeviceGetPowerManagementLimit",
	"nvmlDeviceGetPowerManagementDefaultLimit",
	"nvmlDeviceGetPowerManagementLimitConstraints",
	"nvmlDeviceSetPowerManagementLimit",
	"nvmlErrorString",
};

static struct nvml_s
{
	nvmlReturn_t (*Init)				(void);
	nvmlReturn_t (*Count)				(uint *devCount);
	nvmlReturn_t (*Handle)				(uint index, nvmlDevice_t *device);
	nvmlReturn_t (*GetDefaultAppsClock)	(nvmlDevice_t dev, nvmlClockType_t clockType, uint* mhz);
	nvmlReturn_t (*GetMemoryClocks)		(nvmlDevice_t dev, uint *count, uint *mhz);
	nvmlReturn_t (*GetGraphicsClocks)	(nvmlDevice_t dev, uint mem_mhz, uint *count, uint *mhz);
	nvmlReturn_t (*GetClock)			(nvmlDevice_t device, nvmlClockType_t clockType, nvmlClockId_t clockId, uint* mhz);
	nvmlReturn_t (*SetLockedClocks)		(nvmlDevice_t dev, uint min_mhz, uint max_mhz);
	nvmlReturn_t (*SetAppsClocks)		(nvmlDevice_t dev, uint mem_mgz, uint gpu_mhz);
	nvmlReturn_t (*ResetAppsClocks)		(nvmlDevice_t dev);
	nvmlReturn_t (*ResetLockedClocks)	(nvmlDevice_t dev);
	nvmlReturn_t (*GetPowerLimit)		(nvmlDevice_t dev, uint *watts);
	nvmlReturn_t (*GetPowerDefaultLimit)(nvmlDevice_t dev, uint *watts);
	nvmlReturn_t (*GetPowerLimitConstraints)(nvmlDevice_t dev, uint *min_watts, uint *max_watts);
	nvmlReturn_t (*SetPowerLimit)		(nvmlDevice_t dev, uint watts);
	char*        (*ErrorString)			(nvmlReturn_t result);
} nvml;

static struct error_s {
	char *init;
	char *init_not;
	char *null_data;
	char *gpus_not;
	char *dlopen;
} Error = {
	.init         = "context already initialized or not empty",
	.init_not     = "context not initialized",
	.null_data    = "data pointer is NULL",
	.gpus_not     = "no GPUs detected",
	.dlopen       = "error during dlopen",
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint			  dev_count;
static nvmlDevice_t  *devices;
static ulong 		 *clock_max_default; // KHz
static ulong		 *clock_max; // KHz
static ulong		 *clock_max_mem; // MHz
static ulong		**clock_list; // KHz
static uint			 *clock_lens;
static ulong		 *power_max_default; // W
static ulong		 *power_max; // W
static ulong		 *power_min; // W
static uint 		  ok_status;
static uint			  ok_unprivileged;
static uint			  ok_init;

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

	if ((r = nvml.Init()) != NVML_SUCCESS) {
		debug("nvml.Init");
		return_msg(EAR_ERROR, (char *) nvml.ErrorString(r));
	}
	if ((r = nvml.Count(&dev_count)) != NVML_SUCCESS) {
		debug("nvml.Count %u",dev_count);
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
		debug("dlopen fail %s",dlerror());
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
	if (load_test(getenv(HACK_FILE_NVML))) return 1;
	if (load_test(NVML_PATH "/targets/x86_64-linux/lib/libnvidia-ml.so")) return 1;
	if (load_test(NVML_PATH "/lib64/libnvidia-ml.so")) return 1;
	if (load_test(NVML_PATH "/lib/libnvidia-ml.so")) return 1;
	if (load_test("/lib64/libnvidia-ml.so")) return 1;
	if (load_test("/usr/lib64/libnvidia-ml.so")) return 1;
	return 0;
}

state_t mgt_nvml_status()
{
	state_t s;

	#ifndef CUDA_BASE
	return EAR_ERROR;
	#endif
	debug("mgt_nvml_status");
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

static state_t static_init_unprivileged_undo(char *error)
{
	int d;
	for (d = 0; d < dev_count; ++d) {
		static_free(clock_list[d]);
	}
	static_free(clock_max_default);
	static_free(clock_max);
	static_free(clock_max_mem);
	static_free(power_max_default);
	static_free(power_max);
	static_free(power_min);
	static_free(clock_list);
	static_free(clock_lens);
	return_msg(EAR_ERROR, error);
}

static state_t static_init_unprivileged()
{
	nvmlReturn_t r;
	uint aux_mem[1000];
	uint aux_gpu[1000];
	uint c1, c2, aux;
	int d, m, g;

	// Allocation
	#define static_alloc(v, l, s) \
		if ((v = calloc(l, s)) == NULL) { return static_init_unprivileged_undo(strerror(errno)); }
	
	debug("static_init_unprivileged");

	static_alloc(clock_max_default, dev_count, sizeof(ulong));
	static_alloc(clock_max,         dev_count, sizeof(ulong));
	static_alloc(clock_max_mem,     dev_count, sizeof(ulong));
	static_alloc(power_max_default, dev_count, sizeof(ulong));
	static_alloc(power_max,         dev_count, sizeof(ulong));
	static_alloc(power_min,         dev_count, sizeof(ulong));
	static_alloc(clock_list,        dev_count, sizeof(ulong *));
	static_alloc(clock_lens,        dev_count, sizeof(uint));

	/*
	 *
	 * Power fill
	 *
	 */
	debug("power list:");
	for (d = 0; d < dev_count; ++d)
	{
		if ((r = nvml.GetPowerDefaultLimit(devices[d], &aux)) != NVML_SUCCESS) {
			debug("nvmlDeviceGetPowerManagementDefaultLimit returned %d (%s)",
				  r, nvml.ErrorString(r));
			return static_init_unprivileged_undo(myErrorString(r));
		}
		if ((r = nvml.GetPowerLimitConstraints(devices[d], &c1, &c2)) != NVML_SUCCESS) {
			debug("nvmlDeviceGetPowerManagementLimitConstraints returned %d (%s)",
				  r, nvml.ErrorString(r));
			return static_init_unprivileged_undo(myErrorString(r));
		}
		power_max_default[d] = ((ulong) aux) / 1000LU;
		power_max[d] = ((ulong) c2) / 1000LU;
		power_min[d] = ((ulong) c1) / 1000LU;
		debug("D%d default: %lu W", d, power_max_default[d]);
		debug("D%d new max: %lu W", d, power_max[d]);
		debug("D%d new min: %lu W", d, power_min[d]);
	}

	/*
	 *
	 * Clocks fill
	 *
	 */
	debug("clock list:");
	for (d = 0; d < dev_count; ++d)
	{
		clock_max_default[d] = 0LU;
		clock_max[d]         = 0LU;

		//Retrieving default application clocks
		if ((r = nvml.GetDefaultAppsClock(devices[d], NVML_CLOCK_GRAPHICS, &aux)) != NVML_SUCCESS) {
			debug("nvmlDeviceGetDefaultApplicationsClock(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			return static_init_unprivileged_undo(myErrorString(r));
		}

		clock_max_default[d] = ((ulong) aux) * 1000LU;
		debug("D%d default: %lu KHz", d, clock_max_default[d]);

		/*
		 * Retrieving a list of clock P_STATEs and saving its maximum.
		 */
		c1 = 1000;
		if ((r = nvml.GetMemoryClocks(devices[d], &c1, aux_mem)) != NVML_SUCCESS) {
			debug("nvmlDeviceGetSupportedMemoryClocks(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			return static_init_unprivileged_undo(myErrorString(r));
		}
		for (m = 0; m < c1; ++m)
		{
			if (aux_mem[m] > clock_max_mem[d]) {
				clock_max_mem[d] = (ulong) aux_mem[m];
			}
			debug("D%d MEM %d: %d (%u)", d, m, aux_mem[m], c1);

			c2 = 1000;
			if ((r = nvml.GetGraphicsClocks(devices[d], aux_mem[m], &c2, aux_gpu)) != NVML_SUCCESS) {
				debug("\t\tnvmlDeviceGetSupportedGraphicsClocks(dev: %d) returned %d (%s)",
					  d, r, nvml.ErrorString(r));
				return static_init_unprivileged_undo(myErrorString(r));
			}

			for (g = 0; g < c2; ++g) {
				if (aux_gpu[g] > clock_max[d]) {
					debug("D%d new max: %d MHz", d, aux_gpu[g]);
					clock_max[d] = (ulong) aux_gpu[g];
				} else {
					//debug("D%d: %d", d, aux_gpu[g]);
				}
			}
		}
		clock_max[d] *= 1000LU;
	}

	/*
	 *
	 * P_STATEs fill
	 *
	 */
	for (d = 0; d < dev_count; ++d)
	{
		clock_lens[d] = 1000;
		if ((r = nvml.GetGraphicsClocks(devices[d], clock_max_mem[d], &clock_lens[d], aux_gpu)) != NVML_SUCCESS) {
			clock_lens[d] = 0;
			return static_init_unprivileged_undo(myErrorString(r));
		}
		clock_list[d] = calloc(clock_lens[d], sizeof(ulong *));
		for (m = 0; m < clock_lens[d]; ++m) {
			clock_list[d][m] = ((ulong) aux_gpu[m]) * 1000LU;
			debug("D%u,M%u: %lu", d, m, clock_list[d][m]);
		}
	}

	ok_unprivileged = 1;

	return EAR_SUCCESS;
}

static state_t static_init()
{
	nvmlReturn_t r;
	uint aux;
	int d;
	debug("static_init");
	// Full clocks reset (application + locked)
	for (d = 0; d < dev_count; ++d) {
		if ((r = nvml.ResetAppsClocks(devices[d])) != NVML_SUCCESS) {
			debug("nvmlDeviceResetApplicationsClocks(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			return_msg(EAR_ERROR, myErrorString(r));
		}
		if ((r = nvml.ResetLockedClocks(devices[d])) != NVML_SUCCESS) {
			debug("nvmlDeviceResetGpuLockedClocks(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			//return_msg(EAR_ERROR, myErrorString(r));
		}
	}

	// Power limit reset
	for (d = 0; d < dev_count; ++d) {
		aux = ((uint) power_max_default[d]) * 1000U;
		if ((r = nvml.SetPowerLimit(devices[d], aux)) != NVML_SUCCESS) {
			debug("nvmlDeviceSetPowerManagementLimit(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			return_msg(EAR_ERROR, myErrorString(r));
		}
	}
	return EAR_SUCCESS;
}

state_t mgt_nvml_init(ctx_t *c)
{
	state_t s;
	debug("mgt_nvml_init");
	if (xtate_fail(s, mgt_nvml_init_unprivileged(c))) {
		return s;
	}
#if 1
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
#endif
	return EAR_SUCCESS;
}

state_t mgt_nvml_init_unprivileged(ctx_t *c)
{
	state_t s;
	debug("mgt_nvml_init_unprivileged");
	while (pthread_mutex_trylock(&lock));
	if (ok_unprivileged) {
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
	if (xtate_fail(s, static_init_unprivileged())) {
		pthread_mutex_unlock(&lock);
		return s;
	}
	ok_unprivileged = 1;
	debug("OK init unprivileged");
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t mgt_nvml_dispose(ctx_t *c)
{
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	return EAR_SUCCESS;
}

state_t mgt_nvml_count(ctx_t *c, uint *_dev_count)
{
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (_dev_count != NULL) {
		*_dev_count = dev_count;
	}
	return EAR_SUCCESS;
}

state_t nvml_freq_limit_get_current(ctx_t *c, ulong *khz)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	uint mhz;
	for (i = 0; i < dev_count; ++i)
	{
		nvml.GetClock(devices[i], NVML_CLOCK_GRAPHICS, NVML_CLOCK_ID_APP_CLOCK_TARGET, &mhz);
		debug("NVML_CLOCK_ID_APP_CLOCK_TARGET %u KHz", mhz * 1000U);
		khz[i] = ((ulong) mhz) * 1000LU;
	}
	return EAR_SUCCESS;
}

state_t nvml_freq_limit_get_default(ctx_t *c, ulong *khz)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0; i < dev_count; ++i) {
		khz[i] = clock_max_default[i];
	}
	return EAR_SUCCESS;
}

state_t nvml_freq_limit_get_max(ctx_t *c, ulong *khz)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0; i < dev_count; ++i) {
		khz[i] = clock_max[i];
	}
	return EAR_SUCCESS;
}

static state_t clocks_reset(int i)
{
	nvmlReturn_t r;
	debug("resetting clocks of device %d", i);

	#if 0
	if ((r = nvml.ResetLockedClocks(devices[i])) != NVML_SUCCESS)
	{
		debug("nvmlDeviceResetGpuLockedClocks(dev: %d) returned %d (%s)",
			  i, r, nvml.ErrorString(r));
	#endif
		if ((r = nvml.ResetAppsClocks(devices[i])) != NVML_SUCCESS) {
			return_msg(EAR_ERROR, myErrorString(r));
		}
	#if 0
	}
	#endif
	return EAR_SUCCESS;
}

state_t nvml_freq_limit_reset(ctx_t *c)
{
	state_t s, e;
	int i;
	if (!ok_init) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0, e = EAR_SUCCESS; i < dev_count; ++i) {
		if (xtate_fail(s, clocks_reset(i))) {
			e = s;
		}
	}
	return e;
}

state_t nvml_freq_get_valid(ctx_t *c, uint d, ulong freq_ref, ulong *freq_near)
{
	ulong *_clock_list = clock_list[d];
	uint   _clock_lens = clock_lens[d];
	ulong freq_floor;
	ulong freq_ceil;
	uint i;

	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}

	if (freq_ref > _clock_list[0]) {
		*freq_near = _clock_list[0];
		return EAR_SUCCESS;
	}

	for (i = 0; i < _clock_lens-1; ++i)
	{
		freq_ceil  = _clock_list[i+0];
		freq_floor = _clock_list[i+1];

		if (freq_ref <= freq_ceil && freq_ref >= freq_floor) {
			if ((freq_ceil - freq_ref) <= (freq_ref - freq_floor)) {
				*freq_near = freq_ceil;
				return EAR_SUCCESS;
			} else {
				*freq_near = freq_floor;
				return EAR_SUCCESS;
			}
		}
	}
	*freq_near  = _clock_list[i];
	return EAR_SUCCESS;
}

state_t nvml_freq_get_next(ctx_t *c, uint d, ulong freq_ref, uint *freq_idx, uint flag)
{
	ulong *_clock_list = clock_list[d];
	uint   _clock_lens = clock_lens[d];
	ulong freq_floor;
	ulong freq_ceil;
	uint i;
	
	if (freq_ref >= _clock_list[0]) {
		if (flag == FREQ_TOP) {
			*freq_idx = 0;
		} else {
			*freq_idx = 1;
		}
		return EAR_SUCCESS;
	}
	for (i = 1; i < _clock_lens-1; ++i)
	{
		freq_ceil  = _clock_list[i+0];
		freq_floor = _clock_list[i+1];
		
		if (freq_ref == freq_ceil || (freq_ref < freq_ceil && freq_ref > freq_floor)) {
			if (flag == FREQ_TOP) {
				*freq_idx = i-1;
			} else {
				*freq_idx = i+1;
			}
			return EAR_SUCCESS;
		}
	}
	if (flag == FREQ_TOP) {
		*freq_idx = i-1;
	} else {
		*freq_idx = i;
	}
	return EAR_SUCCESS;
}

static state_t clocks_set(int i, uint mhz)
{
	nvmlReturn_t r;
	uint parsed_mhz;
	ulong aux;

	nvml_freq_get_valid(NULL, i, ((ulong) mhz) * 1000LU, &aux);
	parsed_mhz = ((uint) aux) / 1000U;

	debug("D%d setting clock %u KHz (parsed => %u)", i, mhz * 1000U, parsed_mhz * 1000U);
#if 0
	if ((r = nvml.SetLockedClocks(devices[i], 0, parsed_mhz)) != NVML_SUCCESS)
	{
		debug("nvmlDeviceSetGpuLockedClocks(dev: %d) returned %d (%s)",
			i, r, nvml.ErrorString(r));
#endif
		if ((r = nvml.SetAppsClocks(devices[i], clock_max_mem[i], parsed_mhz)) != NVML_SUCCESS)
		{
			debug("nvmlDeviceSetApplicationsClocks(dev: %d) returned %d (%s)",
				i, r, nvml.ErrorString(r));
			// Unlike POWER functions, which have a function ask about the current
			// POWER LIMIT value, CLOCK functions does not have that possibilty. So
			// If the CLOCK SET function fails, it will try to set the default clock
			// frequency, which we already know its value.
			clocks_reset(i);
			//
			return_msg(EAR_ERROR, myErrorString(r));
		}
#if 0
	}
#endif
	return EAR_SUCCESS;
}

state_t nvml_freq_limit_set(ctx_t *c, ulong *khz)
{
	state_t s, e;
	int i;
	if (!ok_init) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	debug("nvml_clock_limit_set devices %d",dev_count);
	for (i = 0, e = EAR_SUCCESS; i < dev_count; ++i) {
		if (khz[i] > 0){
		  if (xtate_fail(s, clocks_set(i, (uint) (khz[i] / 1000LU)))) {
			  e = s;
		  }
		}
	}
	return e;
}

state_t nvml_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
	debug("nvml_freq_list");
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	if (list_khz != NULL) {
		*list_khz = (const ulong **) clock_list;
	}
	if (list_len != NULL) {
		*list_len = (const uint *) clock_lens;
	}
	return EAR_SUCCESS;
}

state_t nvml_power_cap_get_current(ctx_t *c, ulong *watts)
{
	nvmlReturn_t r;
	state_t e;
	uint aux;
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0, e = EAR_SUCCESS; i < dev_count; ++i) {
		if ((r = nvml.GetPowerLimit(devices[i], &aux)) != NVML_SUCCESS) {
			debug("nvmlDeviceGetPowerManagementLimit(dev: %d) returned %d (%s)",
				  i, r, nvml.ErrorString(r));
			// Setting error and unknown value parameter
			watts[i]  = 0;
			state_msg = (char *) nvml.ErrorString(r);
			e         = EAR_ERROR;
		}
		watts[i] = aux / 1000u;
	}
	return e;
}

state_t nvml_power_cap_get_default(ctx_t *c, ulong *watts)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0; i < dev_count; ++i) {
		watts[i] = power_max_default[i];
	}
	return EAR_SUCCESS;
}

state_t nvml_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
	int i;
	if (!ok_unprivileged) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0; i < dev_count && watts_max != NULL; ++i) {
		watts_max[i] = power_max[i];
	}
	for (i = 0; i < dev_count && watts_min != NULL; ++i) {
		watts_min[i] = power_min[i];
	}
	return EAR_SUCCESS;
}

static state_t powers_set(int i, uint mw)
{
	nvmlReturn_t r;
	
	debug("D%d setting power to %u mW", i, mw);
	if ((r = nvml.SetPowerLimit(devices[i], mw)) != NVML_SUCCESS)
	{
		debug("nvmlDeviceSetPowerManagementLimit(dev: %d) returned %d (%s)",
			  i, r, nvml.ErrorString(r));
		return_msg(EAR_ERROR, myErrorString(r));
	}
	return EAR_SUCCESS;
}

state_t nvml_power_cap_reset(ctx_t *c)
{
	return nvml_power_cap_set(c, power_max_default);
}

state_t nvml_power_cap_set(ctx_t *c, ulong *watts)
{
	state_t s, e;
	int i;
	if (!ok_init) {
		return_msg(EAR_NOT_INITIALIZED, Error.init_not);
	}
	for (i = 0, e = EAR_SUCCESS; i < dev_count; ++i) {
		if (xtate_fail(s, powers_set(i, (uint) (watts[i] * 1000LU)))) {
			e = s;
		}
	}
	return e;
}
