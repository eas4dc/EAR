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

// #define SHOW_DEBUGS 1

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <level_zero/ze_api.h>
#include <level_zero/zes_ddi.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include "zero.h"

#define ZE_PATH          ZERO_BASE
#define ZE_LIB           "libze_loader.so"
#define ZE_SUCCESS       ZE_RESULT_SUCCESS
#define ZE_N             17

static const char *ze_names[] = {
	"zeInit",
	"zeDriverGet",
	"zeDeviceGet",
	"zeDeviceGetProperties",
	"zesDeviceEnumPowerDomains",
	"zesPowerGetEnergyCounter",
	"zesDeviceEnumFrequencyDomains",
	"zesFrequencyGetProperties",
	"zesFrequencyGetState",
	"zesDeviceEnumEngineGroups",
	"zesEngineGetProperties",
	"zesEngineGetActivity",

	"zeDriverGetApiVersion",
	"zeDriverGetProperties",
	"zeDriverGetExtensionProperties",
	"zeDeviceGetMemoryProperties",
	"zeDeviceGetSubDevices",
};

static struct ze_s {
	ze_result_t (*Init)                    (ze_init_flags_t flags);
	ze_result_t (*DriverGet)               (uint32_t *n, ze_driver_handle_t *ph);
	ze_result_t (*DeviceGet)               (ze_driver_handle_t h, uint32_t *n, ze_device_handle_t *ph);
	ze_result_t (*DeviceGetProperties)     (ze_device_handle_t h, ze_device_properties_t *props);
	ze_result_t (*sDeviceEnumPowerDomains) (zes_device_handle_t h, uint32_t *n, zes_pwr_handle_t *ph);
	ze_result_t (*sPowerGetEnergyCounter)  (zes_pwr_handle_t h, zes_power_energy_counter_t *info);
	ze_result_t (*sDeviceEnumFrequencyDomains) (zes_device_handle_t h, uint32_t *n, zes_freq_handle_t *ph);
	ze_result_t (*sFrequencyGetProperties) (zes_freq_handle_t h, zes_freq_properties_t *props);
	ze_result_t (*sFrequencyGetState)      (zes_freq_handle_t h, zes_freq_state_t *info);
	ze_result_t (*sDeviceEnumEngineGroups) (zes_device_handle_t h, uint32_t *n, zes_engine_handle_t *ph);
	ze_result_t (*sEngineGetProperties)    (zes_engine_handle_t h, zes_engine_properties_t *props);
	ze_result_t (*sEngineGetActivity)      (zes_engine_handle_t h, zes_engine_stats_t *info);

	ze_result_t (*DriverGetApiVersion)     (ze_driver_handle_t h, ze_api_version_t *v);
	ze_result_t (*DriverGetProperties)     (ze_driver_handle_t h, ze_driver_properties_t *p);
        ze_result_t (*DriverGetExtensionProperties) (ze_driver_handle_t h, uint32_t *count, ze_driver_extension_properties_t *e);
        ze_result_t (*DeviceGetMemoryProperties) (ze_device_handle_t h, uint32_t *count, ze_device_memory_properties_t *p);
	ze_result_t (*DeviceGetSubDevices)     (ze_device_handle_t h, uint32_t *p, ze_device_handle_t *hs);
} ze;

static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static zes_device_handle_t  devices[32];
static uint                 devices_count;
static gpu_t               *pool; // change to sysmans_pool
static timestamp_t          pool_time1;
static timestamp_t          pool_time2;
static suscription_t       *sus;

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
	symplug_join(libze, (void **) &ze, ze_names, ZE_N);

	for(i = 0, error = 0; i < ZE_N; ++i) {
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
	if (load_test(getenv("HACK_FILE_ZERO"))) return s;
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

static state_t print_driver_info(ze_driver_handle_t driver, int dr)
{
	ze_driver_extension_properties_t ext[1024];
	uint                             ext_count = 1024;
	ze_driver_properties_t           props;
	ze_api_version_t                 version;
	ze_result_t                      z;
	uint                             i;
	
	#define ret_fail(function) \
		if ((z = function) != ZE_SUCCESS) { \
			debug("Failed " #function " with error 0x%x", z); \
			return EAR_ERROR; \
		} 
		//} else { \
		//	debug("Succeded " #function); \
		//}

	// Driver info
	ret_fail(ze.DriverGetProperties(driver, &props));
	ret_fail(ze.DriverGetApiVersion(driver, &version));
	ret_fail(ze.DriverGetExtensionProperties(driver, &ext_count, ext));

	debug("DRIVER%d version: %d (API %d) ", dr, props.driverVersion, version);
	// Too much info
	#if 0
	for (i = 0; i < ext_count; ++i) {
		debug("DRIVER%d extension: %s v%d", dr, ext[i].name, ext[i].version);
	}
	#endif

	return EAR_SUCCESS;
}

static state_t print_device_info(ze_device_handle_t device, int dv)
{
	ze_device_memory_properties_t mem[32];
	uint                          mem_count = 32;
	uint                          sub_count = 0;
	ze_device_properties_t        props;
	ze_result_t                   z;
	int                           i;

	ret_fail(ze.DeviceGetProperties(device, &props));
	ret_fail(ze.DeviceGetMemoryProperties(device, &mem_count, mem));
	ret_fail(ze.DeviceGetSubDevices(device, &sub_count, NULL));

	debug("DEVICE%d name: %s %s (id 0x%x.0x%x)", dv, ((props.type == ZE_DEVICE_TYPE_GPU) ? "GPU" : "OTHER"),
		props.name, props.vendorId, props.deviceId);
	debug("DEVICE%d performance: %u MHz@%lu MB allocatable", dv,
		props.coreClockRate, props.maxMemAllocSize / 1000000LU);
	debug("DEVICE%d sub-devices: %u", dv, sub_count);
	for (i = 0; i < mem_count; ++i) {
		debug("DEVICE%d MEM%d: %s, %u MHz, %u bus width, %lu total MB", dv, i,
		    mem[i].name, mem[i].maxClockRate, mem[i].maxBusWidth, mem[i].totalSize / 1000000LU);
	}

	return EAR_SUCCESS;
}

state_t oneapi_init(ctx_t *c)
{
	ze_device_properties_t props;
	ze_driver_handle_t    *drivers;
	ze_device_handle_t    *predevs;
	uint                   drivers_count = 0;
	uint                   predevs_count = 0;
	ze_result_t            z;
	int                    dr;
	int                    dv;
	state_t                s;

	// This functions is protected from above. Lock is not required.
	ret_fail(ze.Init(0));
	// Discovering all drivers
	ret_fail(ze.DriverGet(&drivers_count, NULL));
	drivers = calloc(drivers_count, sizeof(ze_driver_handle_t));
	ret_fail(ze.DriverGet(&drivers_count, drivers));

	for (dr = 0; dr < drivers_count; ++dr) {
		// Printing driver information
		print_driver_info(drivers[dr], dr);

		// Discovering all devices
		ret_fail(ze.DeviceGet(drivers[dr], &predevs_count, NULL));
		predevs = calloc(predevs_count, sizeof(ze_device_handle_t));
		ret_fail(ze.DeviceGet(drivers[dr], &predevs_count, predevs));

		// Getting all sysmans
		for (dv = 0; dv < predevs_count; ++dv) {
			// Printing device information
			print_device_info(predevs[dv], dv);

			ret_fail(ze.DeviceGetProperties(predevs[dv], &props));
			// If not a GPU
			if(ZE_DEVICE_TYPE_GPU != props.type) {
				continue;
			}
			// Ok, its a GPU
			devices[devices_count] = predevs[dv];
			devices_count++;
		}
		free(predevs);
	}
	free(drivers);

	// No GPUs
	if (devices_count == 0) {
		return_msg(EAR_ERROR, "No GPUs detected");
	}

#if 1
	// Monitor suscription
	pool = calloc(devices_count, sizeof(gpu_t));
	// Initializing monitoring thread suscription.
	sus = suscription();
	sus->call_main  = oneapi_pool;
	sus->time_relax = 1000;
	sus->time_burst = 300;
	// Initializing monitoring thread.
	if (state_fail(s = sus->suscribe(sus))) {
		debug("GPU monitor FAILS");
		monitor_unregister(sus);
		free(pool);
	}
#endif

	return EAR_SUCCESS;
}

state_t oneapi_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t oneapi_count(ctx_t *c, uint *dev_count)
{
	*dev_count = devices_count;
	return EAR_SUCCESS;
}

static state_t oneapi_read_power(int dv, gpu_t *data)
{
	zes_pwr_handle_t           domains[4]; // Its enough
	uint                       domains_count = 4;
	zes_power_properties_t     props;
	zes_power_energy_counter_t info;
	uint                       ma;
	ze_result_t                z;

	// Retrieving the domains
	ret_fail(ze.sDeviceEnumPowerDomains(devices[dv], &domains_count, domains));
	
	for (ma = 0; ma < domains_count; ++ma) {
		ret_fail(ze.sPowerGetEnergyCounter(domains[ma], &info));
		// It has to be tested.
		data->energy_j = ((double) info.energy) / 1000000.0;
	}

	return EAR_SUCCESS;
}

static state_t oneapi_read_utilization(int dv, gpu_t *data)
{
	zes_engine_handle_t     domains[4]; // Its enough
	uint                    domains_count = 4;
	zes_engine_properties_t props;
	zes_engine_stats_t      info;
	uint                    ma;
	ze_result_t             z;

	// Retrieving the domains
	ret_fail(ze.sDeviceEnumEngineGroups(devices[dv], &domains_count, domains));
	
	for (ma = 0; ma < domains_count; ++ma) {
		// Getting domain characteristics
		ret_fail(ze.sEngineGetProperties(domains[ma], &props));
		// Maybe ZES_ENGINE_GROUP_COMPUTE_ALL?
		if (props.type == ZES_ENGINE_GROUP_ALL) {
			ret_fail(ze.sEngineGetActivity(domains[ma], &info));
			// Utilization here, it has to be tested
			data->util_gpu = (ulong) (info.activeTime); // In microseconds
		}
	}

	return EAR_SUCCESS;
}

static state_t oneapi_read_frequency(int dv, gpu_t *data)
{
	zes_freq_handle_t     domains[4]; // Its enough
	uint                  domains_count;
	zes_freq_properties_t props;
	zes_freq_state_t      info;
	uint                  ma;
	ze_result_t           z;

	// Retrieving the domains
	ret_fail(ze.sDeviceEnumFrequencyDomains(devices[dv], &domains_count, domains));
	for (ma = 0; ma < domains_count; ++ma) {
		// Getting domain characteristics
		ret_fail(ze.sFrequencyGetProperties(domains[ma], &props));
		//
		ret_fail(ze.sFrequencyGetState(domains[ma], &info));
		// If is not GPU is MEMORY and we need both
		if (props.type == ZES_FREQ_DOMAIN_GPU) {
			data->freq_gpu = (ulong) info.actual; // MHz
		}
		if (props.type == ZES_FREQ_DOMAIN_MEMORY) {
			data->freq_mem = (ulong) info.actual; // MHz
		}
	}

	return EAR_SUCCESS;
}

static state_t oneapi_read_temperature(int dv, gpu_t *data)
{
	zes_temp_handle_t domains[6]; // Its enough
	// Future.
}

static void read_single(int dv, gpu_t *gpu)
{
	// Cleanin
	memset(gpu, 0, sizeof(gpu_t));
	// Reading
	oneapi_read_power(dv, gpu);
	oneapi_read_frequency(dv, gpu);
	oneapi_read_utilization(dv, gpu);
	oneapi_read_temperature(dv, gpu);
}

state_t oneapi_pool(void *p)
{
	gpu_t       current;
	int         dv;

	// Locking
	while (pthread_mutex_trylock(&lock));
	// Getting current time
	timestamp_getfast(&pool_time2);
	// Adding to the pool
	for (dv = 0; dv < devices_count; ++dv)
	{
		read_single(dv, &current);

		pool[dv].time      = pool_time2;
		pool[dv].samples  += 1;
		pool[dv].freq_mem += current.freq_mem;
		pool[dv].freq_gpu += current.freq_gpu;
		pool[dv].util_mem += 0LU;
		pool[dv].util_gpu += current.util_gpu;
		pool[dv].temp_gpu += current.temp_gpu;
		pool[dv].temp_mem += current.temp_mem;
		pool[dv].energy_j += current.energy_j;
		pool[dv].power_w  += 0.0;
		pool[dv].working   = current.working;
		pool[dv].correct   = current.correct;
		
		debug("dev | metric   | value");
		debug("--- | -------- | -----");
		debug("%d   | freq     | %lu MHz", dv, current.freq_gpu);
		debug("%d   | energy   | %0.2lf J", dv, current.energy_j);
		debug("%d   | util     | %lu %%", dv, current.util_gpu);
	}
	// Copying time
	pool_time1 = pool_time2;
	// Unlock
	pthread_mutex_unlock(&lock);

	return EAR_SUCCESS;
}

state_t oneapi_read(ctx_t *c, gpu_t *data)
{
	while (pthread_mutex_trylock(&lock));
	memcpy(data, pool, devices_count * sizeof(gpu_t));
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t oneapi_read_raw(ctx_t *c, gpu_t *data)
{
	// First it requires tests
	return EAR_SUCCESS;
}

#if 0
int main(int argc, char *argv)
{
	oneapi_load();
	oneapi_init(NULL);

	monitor_init();
	sleep(20);
	
	return 0;
}
#endif
