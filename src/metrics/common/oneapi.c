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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/common/oneapi.h>

// Provisional
#define HACK_ONEAPI_FILE  "HACK_ONEAPI_FILE"

#ifndef ONEAPI_BASE
#define ZE_PATH          ""
#else
#define ZE_PATH          ONEAPI_BASE
#endif
#define ZE_N             17
#define ZE_LIB           "libze_loader.so"

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

static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static zes_device_handle_t *devs;
static uint                 devs_count;
static ze_t                 ze;
static uint                 ok;

static state_t static_open()
{
    #define _open_test(path) \
        debug("openning %s", path); \
        if (state_ok(plug_open(path, (void **) &ze, ze_names, ZE_N, RTLD_NOW | RTLD_LOCAL))) { \
            return EAR_SUCCESS; \
        }
    #define open_test(path) \
        _open_test(path ZE_LIB); \
        _open_test(path ZE_LIB ".1");

    // Looking for level zero library in tipical paths.
    _open_test(getenv(HACK_ONEAPI_FILE));
    open_test(ZE_PATH "/targets/x86_64-linux/lib/");
    open_test(ZE_PATH "/lib64/");
    open_test(ZE_PATH "/lib/");
    open_test("/usr/lib64/");
    open_test("/usr/lib/x86_64-linux-gnu/");
    open_test("/usr/lib32/");
    open_test("/usr/lib/");

    return_msg(EAR_ERROR, "Can not load ");
}

#if 0
static state_t static_free(state_t s, char *error)
{
    return_msg(s, error);
}
#endif

static state_t print_driver_info(ze_driver_handle_t driver, int dr)
{
    ze_driver_extension_properties_t ext[1024];
    uint                             ext_count = 1024;
    ze_driver_properties_t           props;
    ze_api_version_t                 version;
    ze_result_t                      z;
    //uint                             i;

    #define ret_fail(function) \
		if ((z = function) != ZE_RESULT_SUCCESS) { \
			debug("Failed " #function " with error 0x%x", z); \
			return EAR_ERROR; \
		}
    #if 0
    } else {
		debug("Succeded " #function); \
	}
    #endif

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

    debug("DEVICE%d name: %s %s (id %s.0x%x)", dv, ((props.type == ZE_DEVICE_TYPE_GPU) ? "GPU" : "OTHER"),
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

static state_t static_init()
{
    ze_device_properties_t props;
    ze_driver_handle_t    *drivers;
    ze_device_handle_t    *predevs;
    uint                   drivers_count = 0;
    uint                   predevs_count = 0;
    ze_result_t            z;
    int                    dr;
    int                    dv;
    //state_t                s;

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
           devs = calloc(predevs_count, sizeof(ze_device_handle_t));
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
            devs[devs_count] = predevs[dv];
            devs_count++;
        }
        free(predevs);
    }
    free(drivers);

    // No GPUs
    if (devs_count == 0) {
        return_msg(EAR_ERROR, "No GPUs detected");
    }
    return EAR_SUCCESS;
}

state_t oneapi_open(ze_t *ze_in)
{
    state_t s = EAR_SUCCESS;
    #ifndef ONEAPI_BASE
    return EAR_ERROR;
    #endif
    while (pthread_mutex_trylock(&lock));
    if (ok) {
        goto fini;
    }
    if (state_ok(s = static_open())) {
        if (state_ok(s = static_init())) {
            ok = 1;
        }
    }
fini:
    pthread_mutex_unlock(&lock);
    if (ze_in != NULL) {
        memcpy(ze_in, &ze, sizeof(ze_t));
    }
    return s;
}

state_t oneapi_close()
{
    return EAR_SUCCESS;
}

state_t oneapi_get_devices(zes_device_handle_t **devs_in, uint *devs_count_in)
{
    if (devs != NULL) {
        *devs_in = malloc(sizeof(zes_device_handle_t)*devs_count);
        memcpy(*devs_in, devs, sizeof(zes_device_handle_t)*devs_count);
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
    return EAR_SUCCESS;
}

int oneapi_is_privileged()
{
    if (!ok) {
        return 0;
    }
    // Provisional
    return (getuid() == 0);
}
