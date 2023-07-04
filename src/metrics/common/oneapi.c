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
#include <common/config/config_env.h>
#include <metrics/common/oneapi.h>

// Provisional
#define HACK_ONEAPI_FILE  "HACK_ONEAPI_FILE"

#ifndef ONEAPI_BASE
#define ZE_PATH          ""
#else
#define ZE_PATH          ONEAPI_BASE
#endif
#define ZE_N             34
#define ZE_LIB           "libze_loader.so"

static const char *ze_names[] = {
    // Core
    "zeInit",
    "zeDriverGet",
    "zeDriverGetApiVersion",
    "zeDriverGetProperties",
    "zeDriverGetExtensionProperties",
    "zeDeviceGet",
    "zeDeviceGetProperties",
    "zeDeviceGetSubDevices",
    "zeDeviceGetMemoryProperties",
    "zeDriverGetLastErrorDescription",
    // Sysman
    "zesDeviceGetProperties",
    "sDeviceProcessesGetState",
    "zesDeviceEnumPowerDomains",
    "zesDeviceGetCardPowerDomain",
    "zesPowerGetProperties",
    "zesPowerGetLimits",
    "zesPowerSetLimits",
    "zesPowerGetEnergyCounter",
    "zesPowerGetLimitsExt",
    "zesPowerSetLimitsExt",
    "zesDeviceEnumEngineGroups",
    "zesEngineGetProperties",
    "zesEngineGetActivity",
    "zesDeviceEnumFrequencyDomains",
    "zesFrequencyGetProperties",
    "zesFrequencyGetState",
    "zesFrequencyGetAvailableClocks",
    "zesFrequencyGetRange",
    "zesFrequencySetRange",
    "zesDeviceEnumTemperatureSensors",
    "zesTemperatureGetProperties",
    "zesTemperatureGetState",
    "zesDeviceEnumPsus",
    "zesPsuGetState",
};

static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static ze_handlers_t       *hs;
static uint                 hs_count;
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
    _open_test(ear_getenv(HACK_ONEAPI_FILE));
    open_test(ZE_PATH "/targets/x86_64-linux/lib/");
    open_test(ZE_PATH "/lib64/");
    open_test(ZE_PATH "/lib/");
    open_test("/usr/lib64/");
    open_test("/usr/lib/x86_64-linux-gnu/");
    open_test("/usr/lib32/");
    open_test("/usr/lib/");

    return_msg(EAR_ERROR, "Can not load ");
}

static state_t driver_info(ze_driver_handle_t driver, int dr)
{
    ze_driver_extension_properties_t ext[1024];
    uint                             ext_count = 1024;
    ze_driver_properties_t           props;
    ze_api_version_t                 version;
    ze_result_t                      z;

    #define ret_fail(function) \
    if ((z = function) != ZE_RESULT_SUCCESS) { \
        debug("Failed " #function ": %s (0x%x)", oneapi_strerror(z), z); \
    }
    //return EAR_ERROR; \
            //debug("Succeded " #function); \

    // Driver info
    ret_fail(ze.DriverGetProperties(driver, &props));
    ret_fail(ze.DriverGetApiVersion(driver, &version));
    ret_fail(ze.DriverGetExtensionProperties(driver, &ext_count, ext));

    debug("DRIVER%d version: %d (API %d) ", dr, props.driverVersion, version);
#if 0
    uint i;
    // Too much info
    for (i = 0; i < ext_count; ++i) {
		debug("DRIVER%d extension: %s v%d", dr, ext[i].name, ext[i].version);
	}
#endif

    return EAR_SUCCESS;
}

static state_t device_info(int h)
{
    ze_device_memory_properties_t mem[1024];
    uint                          mem_count = 1024;
    uint                          sub_count = 0;
    ze_device_properties_t        props;
    ze_result_t                   z;
    int                           i;

    //ret_fail(ze.DeviceGetProperties(hs[h].device, &props));
    ret_fail(ze.DeviceGetMemoryProperties(hs[h].device, &mem_count, mem));
    ret_fail(ze.DeviceGetSubDevices(hs[h].device, &sub_count, NULL));
    // Printing information
#if 0
    for (i = 0; i < mem_count; ++i) {
        debug("%s total %lu MB, %u bus width (%u MHz)",
              mem[i].name, mem[i].totalSize / 1000000LU, mem[i].maxBusWidth, mem[i].maxClockRate);
    }
#endif
    debug("name: %s", hs[h].device_props.name);
    debug("type: %s (%d)", ((hs[h].device_props.vendorId == ZE_DEVICE_TYPE_GPU) ? "GPU" : "OTHER"), hs[h].device_props.type);
    debug("vendorId: 0x%x", hs[h].device_props.vendorId);
    debug("deviceId: 0x%x", hs[h].device_props.deviceId);
    debug("subdeviceId: 0x%x", hs[h].device_props.subdeviceId);
    debug("coreClockRate: @%u MHz", hs[h].device_props.coreClockRate);
    debug("maxMemAllocSize: %lu MB", hs[h].device_props.maxMemAllocSize / 1000000LU);
    debug("uuid: '%s'", hs[h].device_props.uuid.id);
    debug("numSubdevices: %u", hs[h].sdevice_props.numSubdevices);
    debug("serialNumber: '%s'", hs[h].sdevice_props.serialNumber);
    debug("boardNumber: '%s'", hs[h].sdevice_props.boardNumber);
    debug("brandName: '%s'", hs[h].sdevice_props.brandName);
    debug("modelName: '%s'", hs[h].sdevice_props.modelName);
    debug("vendorName: '%s'", hs[h].sdevice_props.vendorName);

    //zes_process_state_t
    debug("#sub-devices: %u", sub_count);
    return EAR_SUCCESS;
}

static state_t static_init()
{
    ze_driver_handle_t    *drivers;
    ze_device_handle_t    *devices;
    uint                   drivers_count = 0;
    uint                   devices_count = 0;
    uint                   uuid_length;
    ze_result_t            z;
    int                    dr;
    int                    dv;
    int                    dp;

    // Enabling SYSMAN
    setenv("ZES_ENABLE_SYSMAN", "1", 1);
    // This functions is protected from above. Lock is not required.
    ret_fail(ze.Init(0));
    // Discovering all drivers
    ret_fail(ze.DriverGet(&drivers_count, NULL));
    drivers = calloc(drivers_count, sizeof(ze_driver_handle_t));
    ret_fail(ze.DriverGet(&drivers_count, drivers));
    debug("#drivers %u", drivers_count);

    // Counting to allocate
    for (dr = 0; dr < drivers_count; ++dr) {
        // Printing driver information
        driver_info(drivers[dr], dr);
        // Counting devices per driver
        ret_fail(ze.DeviceGet(drivers[dr], &devices_count, NULL));
        hs_count += devices_count;
    }
    // No GPUs
    if (hs_count == 0) {
        return_msg(EAR_ERROR, "No GPUs detected");
    }
    // Allocating all the devices
    debug("#devices %u", hs_count);
    devices  = calloc(hs_count, sizeof(ze_device_handle_t));
    hs       = calloc(hs_count, sizeof(ze_handlers_t));
    hs_count = 0;
    // Filling
    for (dr = 0; dr < drivers_count; ++dr) {
        devices_count = 0;
        // Discovering all devices
        ret_fail(ze.DeviceGet(drivers[dr], &devices_count, NULL));
        ret_fail(ze.DeviceGet(drivers[dr], &devices_count, devices));

        for (dv = 0; dv < devices_count; ++dv) {
            debug("-- DEVICE%d ----------------------------", dv);
            // Copying main (single) domains
            memcpy(&hs[hs_count].driver , &drivers[dr], sizeof(ze_driver_handle_t));
            memcpy(&hs[hs_count].device , &devices[dv], sizeof(ze_device_handle_t));
            memcpy(&hs[hs_count].sdevice, &devices[dv], sizeof(zes_device_handle_t));
            // Counting specific domains
            ret_fail(ze.sDeviceEnumPowerDomains      (devices[dv], &hs[hs_count].spowers_count , NULL));
            ret_fail(ze.sDeviceEnumFrequencyDomains  (devices[dv], &hs[hs_count].sfreqs_count  , NULL));
            ret_fail(ze.sDeviceEnumEngineGroups      (devices[dv], &hs[hs_count].sengines_count, NULL));
            ret_fail(ze.sDeviceEnumTemperatureSensors(devices[dv], &hs[hs_count].stemps_count  , NULL));
            ret_fail(ze.sDeviceEnumPsus              (devices[dv], &hs[hs_count].spsus_count   , NULL));
            // Allocating specific domains
            hs[hs_count].spowers  = calloc(hs[hs_count].spowers_count , sizeof(zes_pwr_handle_t));
            hs[hs_count].sfreqs   = calloc(hs[hs_count].sfreqs_count  , sizeof(zes_freq_handle_t));
            hs[hs_count].sengines = calloc(hs[hs_count].sengines_count, sizeof(zes_engine_handle_t));
            hs[hs_count].stemps   = calloc(hs[hs_count].stemps_count  , sizeof(zes_temp_handle_t));
            hs[hs_count].spsus    = calloc(hs[hs_count].spsus_count   , sizeof(zes_psu_handle_t));
            // Getting specific domains
            ret_fail(ze.sDeviceEnumPowerDomains      (devices[dv], &hs[hs_count].spowers_count , hs[hs_count].spowers ));
            ret_fail(ze.sDeviceEnumFrequencyDomains  (devices[dv], &hs[hs_count].sfreqs_count  , hs[hs_count].sfreqs  ));
            ret_fail(ze.sDeviceEnumEngineGroups      (devices[dv], &hs[hs_count].sengines_count, hs[hs_count].sengines));
            ret_fail(ze.sDeviceEnumTemperatureSensors(devices[dv], &hs[hs_count].stemps_count  , hs[hs_count].stemps  ));
            ret_fail(ze.sDeviceEnumPsus              (devices[dv], &hs[hs_count].spsus_count   , hs[hs_count].spsus   ));
            // Allocating properties
            hs[hs_count].spowers_props  = calloc(hs[hs_count].spowers_count , sizeof(zes_power_properties_t));
            hs[hs_count].sfreqs_props   = calloc(hs[hs_count].sfreqs_count  , sizeof(zes_freq_properties_t));
            hs[hs_count].sengines_props = calloc(hs[hs_count].sengines_count, sizeof(zes_engine_properties_t));
            hs[hs_count].stemps_props   = calloc(hs[hs_count].stemps_count  , sizeof(zes_temp_properties_t));
            // Getting properties
            for (dp = 0; dp < hs[hs_count].spowers_count ; ++dp) ret_fail(ze.sPowerGetProperties      (hs[hs_count].spowers[dp] , &hs[hs_count].spowers_props[dp] ));
            for (dp = 0; dp < hs[hs_count].sfreqs_count  ; ++dp) ret_fail(ze.sFrequencyGetProperties  (hs[hs_count].sfreqs[dp]  , &hs[hs_count].sfreqs_props[dp]  ));
            for (dp = 0; dp < hs[hs_count].sengines_count; ++dp) ret_fail(ze.sEngineGetProperties     (hs[hs_count].sengines[dp], &hs[hs_count].sengines_props[dp]));
            for (dp = 0; dp < hs[hs_count].stemps_count  ; ++dp) ret_fail(ze.sTemperatureGetProperties(hs[hs_count].stemps[dp]  , &hs[hs_count].stemps_props[dp]  ));
            // Getting properties to see if is a GPU device
            ret_fail(ze.DeviceGetProperties(hs[hs_count].device, &hs[dv].device_props));
            ret_fail(ze.sDeviceGetProperties(hs[hs_count].sdevice, &hs[dv].sdevice_props));
            device_info(dv);
            // If not a GPU
            if(ZE_DEVICE_TYPE_GPU != hs[dv].device_props.type) {
                // continue;
            }
            // If serial doesn't exists, then we are building it.
            if (!(uuid_length = strnlen(hs[dv].device_props.uuid.id, ZE_MAX_DEVICE_UUID_SIZE))) {
                sprintf(hs[dv].device_props.uuid.id, "%u", 1337+hs_count);
            }
            hs[hs_count].serial = strtoull(hs[dv].device_props.uuid.id, NULL, 10);
            // Last printings
            debug("#power  domains: %u", hs[hs_count].spowers_count );
            debug("#freq   domains: %u", hs[hs_count].sfreqs_count  );
            debug("#engine domains: %u", hs[hs_count].sengines_count);
            debug("#temp   domains: %u", hs[hs_count].stemps_count  );
            debug("----------------------------------------");

            hs_count++;
        }
    }
    free(devices);
    free(drivers);
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

state_t oneapi_get_devices(ze_handlers_t **devs, uint *devs_count)
{
    if (devs != NULL) {
        *devs = calloc(hs_count, sizeof(ze_handlers_t));
        memcpy(*devs, hs, sizeof(ze_handlers_t) * hs_count);
    }
    if (devs_count != NULL) {
        *devs_count = hs_count;
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

char *oneapi_strerror(ze_result_t z)
{
    //ze.StrError(devs[dv].driver, (const char **) &pointer);
    if (z == 0x78000001) return "uninitialized driver";
    if (z == 0x78000003) return "unsupported feature";
    if (z == 0x78000004) return "invalid argument";
    if (z == 0x78000005) return "invalid handler";
    return "unknown error";
}