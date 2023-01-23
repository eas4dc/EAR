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

#ifdef ONEAPI_BASE
#include <ze_api.h>
#include <ze_driver.h>
#include <zet_sysman.h>
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
#include <metrics/gpu/archs/oneapi.h>

#ifndef ONEAPI_BASE
// ZE Fake Struct
typedef struct zefs_s {
	ullong something_imaginary;
} zefs_t;

typedef int    ze_init_flag_t;
typedef int    ze_result_t;
typedef zefs_t ze_driver_handle_t;
typedef zefs_t ze_device_handle_t;
typedef zefs_t zet_device_handle_t;

#define ZE_PATH          ""
#define ZE_LIB           "nothing"
#define ZE_SUCCESS       0
#define ZE_SYMS          0
#else
#define ZE_PATH          ONEAPI_BASE
#define ZE_LIB           "libze_loader.so"
#define ZE_SUCCESS       ZE_RESULT_SUCCESS
#define ZE_SYMS          10
#endif

static const char *ze_names[] = {
	"zeInit",
	"zetInit",
	"zeDriverGet",
	"zeDeviceGet",
	"zeDeviceGetProperties",
	"zetSysmanGet",
};

static struct ze_s {
	ze_result_t (*Init)      (ze_init_flag_t flags);
	ze_result_t (*tInit)     (ze_init_flag_t flags);
	ze_result_t (*DriverGet) (uint *n, ze_driver_handle_t *h);
	ze_result_t (*DeviceGet) (ze_driver_handle_t d, uint *n, ze_device_handle_t *h);
	ze_result_t (*DeviceGetProperties) (ze_device_handle_t d, ze_device_properties_t *props);
	ze_result_t (*SysmanGet) (zet_device_handle_t d, zet_sysman_version_t version, zet_sysman_handle_t *h);
} ze;

static zet_sysman_handle_t sysmans[32]; // 32 GPUs are fairly enough
static uint                sysmans_count;
static gpu_t              *sysmans_pool[32];

static int load_test(char *path)
{
	void **p = (void **) &ze;
	void *libze;
	int error;
	int i;

	//
	debug("trying to access to '%s'", path);
	if (access(path, X_OK) != 0) {
		return 0;
	}
	if ((libze = dlopen(path, RTLD_NOW | RTLD_LOCAL)) == NULL) {
		debug("dlopen fail");
		return 0;
	}
	debug("dlopen ok");

	//
	plug_join(libze, (void **) &ze, ze_names, NVML_N);

	for(i = 0, error = 0; i < NVML_N; ++i) {
		debug("symbol %s: %d", ze_names[i], (p[i] != NULL));
		error += (p[i] == NULL);
	}
	if (error > 0) {
		memset((void *) &ze, 0, sizeof(ze));
		dlclose(libze);
		return 0;
	}

	return 1;
}

static state_t static_load()
{
	state_t s = EAR_SUCCESS;
	// Looking for nvidia library in tipical paths.
	if (load_test(getenv(HACK_FILE_NVML))) return s;
	if (load_test(ZE_PATH "/targets/x86_64-linux/lib/" ZE_LIB)) return s;
	if (load_test(ZE_PATH "/lib64/" ZE_LIB)) return s;
	if (load_test(ZE_PATH "/lib/" ZE_LIB)) return s;
	if (load_test("/usr/lib64/" ZE_LIB)) return s;
	if (load_test("/usr/lib/x86_64-linux-gnu/" ZE_LIB)) return s;
	if (load_test("/usr/lib32/" ZE_LIB)) return s;
	if (load_test("/usr/lib/" ZE_LIB)) return s;
	return_msg(EAR_ERROR, "Can not load " ZE_LIB);
}

state_t oneapi_load()
{
	state_t s;
	// This function is protected from above. Lock is not required.
	if (state_fail(s = static_load())) {
		return s;
	}
	return EAR_SUCCESS;
}

state_t oneapi_init(ctx_t *c)
{
	ze_device_properties_t props;
	ze_driver_handle_t *drivers;
	ze_driver_handle_t *devices;
	uint count_drivers;
	uint count_devices;
	ze_result_t z;

	#define ret_fail(function) \
		if ((z = function) != ZE_SUCCESS) { \
			return EAR_ERROR; \
		}

	// This functions is protected from above. Lock is not required.
	ret_fail(ze.Init(ZE_INIT_FLAG_NONE));
	ret_fail(ze.tInit(ZE_INIT_FLAG_NONE));

	// Discovering all drivers
	ret_fail(ze.DriverGet(&count_drivers, NULL));
	drivers = calloc(count_drivers, sizeof(ze_driver_handle_t));
	ret_fail(ze.DriverGet(&count_drivers, drivers));

	for (dr = 0; dr < count_drivers; ++dr) {
		// Discovering all devices
		ret_fail(ze.DeviceGet(drivers[drd], &count_devices, NULL));
		devices = calloc(count_devices, sizeof(ze_device_handle_t));
		ret_fail(ze.DeviceGet(drivers[drd], &count_devices, devices));

		// Getting all sysmans
		for (dv = 0; dv < count_devices; ++dv) {
			ret_fail(ze.DeviceGetProperties(devices[dv], &props));
			// If not a GPU
			if(ZE_DEVICE_TYPE_GPU != props.type) {
				continue;
			}
			// Ok, its a GPU
			ret_fail(ze.tSysmanGet(devices[dv], ZET_SYSMAN_VERSION_CURRENT, &sysman[sysmans_count]));
			++sysmans_count;
		}
		free(devices);
	}
	free(drivers);

	// No GPUs
	if (sysmans_count == 0) {
		return_msg(EAR_ERROR, Error.gpus_not);
	}

	return EAR_SUCCESS;
}

state_t oneapi_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t oneapi_count(ctx_t *c, uint *dev_count)
{
	*dev_count = sysmans_count;
	return EAR_SUCCESS;
}

static void oneapi_read_power(gpu_t *data)
{
	zes_pwr_handle_t domains[4];
	uint             domains_count;


}

static void oneapi_read_frequency(gpu_t *data)
{
	zet_sysman_freq_handle_t domains[4]; // Its enough
	uint                     domains_count;
	zet_freq_properties_t    props;
	zet_freq_state_t         info;
	uint                     sm;
	uint                     ma;

	// Retrieving available data from GPUs
	for (sm = 0; sm < sysmans_count; ++sm) {
		// Retrieving the domains
		ret_fail(ze.tSysmanFrequencyGet(sysmans[i], &domains_count, domains));
		for (ma = 0; ma < domains_count; ++ma) {
			// Getting domain characteristics
			ret_fail(ze.tSysmanFrequencyGetProperties(domains[sm], &props));
			// If is not GPU is MEMORY and we need both
			if (props.type != ZET_FREQ_DOMAIN_GPU) {
				data[sm]->freq_gpu = info.actual;
			}
			if (props.type != ZET_FREQ_DOMAIN_MEMORY) {
				data[sm]->freq_mem = info.actual;
			}
		}
	}

	return EAR_SUCCESS;
}

static void oneapi_read_utilization(gpu_t *data)
{

}

state_t oneapi_pool(void *p)
{
	gpu_t current;

	// Cleaning
	memset(&current, 0, sizeof(gpu_t)*sysmans_count);
	// Reading
	oneapi_read_power(current);
	oneapi_read_frequency(current);
	oneapi_read_utilization(current);
	// Adding to the pool
	for (i = 0; i < sysmans_count; ++i) {
		pool[i].time      = time;
		pool[i].samples  += current.samples;
		pool[i].freq_mem += current.freq_mem;
		pool[i].freq_gpu += current.freq_gpu;
		pool[i].util_mem += current.util_mem;
		pool[i].util_gpu += current.util_gpu;
		pool[i].temp_gpu += current.temp_gpu;
		pool[i].temp_mem += current.temp_mem;
		pool[i].energy_j  = current.energy_j;
		pool[i].power_w  += current.power_w;
		pool[i].working   = current.working;
		pool[i].correct   = current.correct;
	}

	return EAR_SUCCESS;
}

state_t oneapi_read(ctx_t *c, gpu_t *data)
{

	return EAR_SUCCESS;
}

state_t oneapi_read_raw(ctx_t *c, gpu_t *data)
{
	oneapi_read_freq(data);

	return EAR_SUCCESS;
}