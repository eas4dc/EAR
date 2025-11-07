/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/environment_common.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/common/oneapi.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Provisional
#define HACK_ONEAPI_FILE "HACK_ONEAPI_FILE"

#ifndef ONEAPI_BASE
#define ZE_PATH ""
#else
#define ZE_PATH ONEAPI_BASE
#endif
#define ZE_N   38
#define ZE_LIB "libze_loader.so"

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
    "zesInit",
    "zesDriverGet",
    "zesDeviceGet",
    "zesDeviceGetProperties",
    "zesDevicePciGetProperties",
    "zesDeviceProcessesGetState",
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

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static ze_handlers_t *hs;
static uint hs_count;
static uint initialized;
static ze_t ze;

static state_t static_open()
{
    static uint opened = 0;

#define _open_test(path)                                                                                               \
    debug("openning %s", path);                                                                                        \
    if (state_ok(plug_open(path, (void **) &ze, ze_names, ZE_N, RTLD_NOW | RTLD_LOCAL))) {                             \
        opened = 1;                                                                                                    \
        return EAR_SUCCESS;                                                                                            \
    }
#define open_test(path)                                                                                                \
    _open_test(path ZE_LIB);                                                                                           \
    _open_test(path ZE_LIB ".1");

    if (opened) {
        return EAR_SUCCESS;
    }
    // Looking for level zero library in tipical paths.
    _open_test(ear_getenv(HACK_ONEAPI_FILE));
    open_test(ZE_PATH "/targets/x86_64-linux/lib/");
    open_test(ZE_PATH "/lib64/");
    open_test(ZE_PATH "/lib/");
    open_test("/usr/lib64/");
    open_test("/usr/lib/x86_64-linux-gnu/");
    open_test("/usr/lib32/");
    open_test("/usr/lib/");

    return_msg(EAR_ERROR, "Can not load");
}

#define reterr return EAR_ERROR;
#define if_fail(function, action)                                                                                      \
    if ((z = function) != ZE_RESULT_SUCCESS) {                                                                         \
        debug("Failed " #function ": %s (0x%x)", oneapi_strerror(z), z);                                               \
        action;                                                                                                        \
    }

// Fowler-Noll-Vo hash function
static ullong fnv1a(char c)
{
    ullong hash0 = 0LLU;
    ullong hash1 = 0LLU;
    hash0        = 16777619LLU ^ (ullong) c;
    hash1 += (hash0 << 1);
    hash1 += (hash0 << 4);
    hash1 += (hash0 << 7);
    hash1 += (hash0 << 8);
    hash1 += (hash0 << 24);
    return hash1 % 2147483647LLU;
}

static ullong hash(char *string)
{
    uint hash = 0;
    uint i    = 0;

    while (i++ < ZE_MAX_DEVICE_UUID_SIZE) {
        hash += fnv1a(string[i]);
    }
    return hash;
}

static void print_core_driver_info(ze_driver_handle_t driver, int dr)
{
    ze_driver_properties_t props;
    ze_api_version_t version;
    ze_result_t z;

    // Driver info
    if_fail(ze.DriverGetProperties(driver, &props), return);
    if_fail(ze.DriverGetApiVersion(driver, &version), return);
    debug("DRIVER%d version: %d (API %d) ", dr, props.driverVersion, version);
}

static void print_core_device_info(int h)
{
    // debug("----------------------------------------");
    debug("name         : '%s'", hs[h].device_props.name);
    debug("pci ids      : vendor 0x%04x, device 0x%04x", hs[h].device_props.vendorId, hs[h].device_props.deviceId);
    debug("type         : %s (%d)", ((hs[h].device_props.type == ZE_DEVICE_TYPE_GPU) ? "GPU" : "OTHER"),
          hs[h].device_props.type);
    debug("uuid         : '%llu' (%u length)", hs[h].uuid, ZE_MAX_DEVICE_UUID_SIZE);
    debug("coreClockRate: @%u MHz", hs[h].device_props.coreClockRate);
}

static void print_sysman_device_info(int h)
{
    // debug("----------------------------------------");
    debug("name         : '%s'", hs[h].device_props.name);
    debug("modelName    : '%s'", hs[h].sdevice_props.modelName);
    debug("serials      : '%s' '%s'", hs[h].sdevice_props.serialNumber, hs[h].sdevice_props.boardNumber);
    debug("uuid         : '%llu' (PCI based)", hs[h].uuid);
    debug("domain:bus:device:function %x:%x:%x:%x",
          hs[h].spci_props.address.domain, hs[h].spci_props.address.bus,
          hs[h].spci_props.address.device, hs[h].spci_props.address.function);
    debug("numSubdevices: %u", hs[h].sdevice_props.numSubdevices);
    debug("#domains PFET: %u/%u/%u/%u",
          hs[hs_count].spowers_count, hs[hs_count].sfreqs_count,
          hs[hs_count].sengines_count, hs[hs_count].stemps_count);
}

static void oneapi_reorder_by_pci(uint devices_count, zes_device_handle_t *devices_tmp, zes_device_handle_t *devices,
                                  ze_handlers_t *hs)
{
    uint dv, dvaux;
    uint *gpu_done = calloc(devices_count, sizeof(uint));
    uint min_pci;

    for (dv = 0; dv < devices_count; dv++) {
        debug("GPU [%u] domain:bus:device:function %x:%x:%x:%x", dv,
              hs[dv].spci_props.address.domain, hs[dv].spci_props.address.bus,
              hs[dv].spci_props.address.device, hs[dv].spci_props.address.function);
    }
    // Reordering
    for (dv = 0; dv < devices_count; dv++) {
        min_pci = 0;
        // reference is the first not already ordered
        while (gpu_done[min_pci] && min_pci < devices_count)
            min_pci++;
        debug("Current min pci element %u", min_pci);
        // Comparing with the others
        for (dvaux = 0; dvaux < devices_count; dvaux++) {
            if ((gpu_done[dvaux] == 0) && (hs[dvaux].spci_props.address.bus < hs[min_pci].spci_props.address.bus))
                min_pci = dvaux;
        }
        gpu_done[min_pci] = 1;
        debug("Moving GPU handler in %u to %u", min_pci, dv);
        memcpy(&devices[dv], &devices_tmp[min_pci], sizeof(zes_device_handle_t));
    }
}

static state_t static_core_init()
{
    ze_driver_handle_t *drivers;
    ze_device_handle_t *devices;
    uint drivers_count = 0;
    uint devices_count = 0;
    ze_result_t z;
    int dr;
    int dv;

    // Note: The documentation says that now it is not needed.
    // setenv("ZES_ENABLE_SYSMAN", "1", 1);
    // setenv("ZES_ENABLE_SYSMAN_LOW_POWER", "1", 1);
    if_fail(ze.Init(0), reterr);
    // Discovering all drivers
    if_fail(ze.DriverGet(&drivers_count, NULL), reterr);
    drivers = calloc(drivers_count, sizeof(ze_driver_handle_t));
    if_fail(ze.DriverGet(&drivers_count, drivers), reterr);
    debug("#drivers %u", drivers_count);

    // Counting to allocate
    for (dr = 0; dr < drivers_count; ++dr) {
        // Printing driver information
        print_core_driver_info(drivers[dr], dr);
        // Counting devices per driver
        if_fail(ze.DeviceGet(drivers[dr], &devices_count, NULL), reterr);
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
        if_fail(ze.DeviceGet(drivers[dr], &devices_count, NULL), reterr);
        if_fail(ze.DeviceGet(drivers[dr], &devices_count, devices), reterr);

        for (dv = 0; dv < devices_count; ++dv) {
            debug("-- DEVICE%d -----------------------------", dv);
            // Getting properties to see if is a GPU device
            if_fail(ze.DeviceGetProperties(devices[dv], &hs[dv].device_props), reterr);
            // If not a GPU
            if (ZE_DEVICE_TYPE_GPU != hs[dv].device_props.type) {
                continue;
            }
            // Copying main (single) domains
            memcpy(&hs[hs_count].driver, &drivers[dr], sizeof(ze_driver_handle_t));
            memcpy(&hs[hs_count].device, &devices[dv], sizeof(ze_device_handle_t));
            // Getting the serial (FUTURE: convert this to PCI address)
            hs[hs_count].uuid = hash((char *) hs[dv].device_props.uuid.id);
            // Last printings
            print_core_device_info(dv);

            hs_count++;
        }
    }
    debug("----------------------------------------");
    free(devices);
    free(drivers);
    return EAR_SUCCESS;
}

static state_t static_sysman_init()
{
    zes_driver_handle_t *drivers;
    zes_device_handle_t *devices;
    zes_device_handle_t *devices_tmp;
    uint drivers_count = 0;
    uint devices_count = 0;
    ze_result_t z;
    int dr;
    int dv;
    int dp;

    // Its weird to enable SYSMAN when you are initializing SYSMAN, but it seems
    // a requirement to initialize.
    setenv("ZES_ENABLE_SYSMAN", "1", 1);
    setenv("ZES_ENABLE_SYSMAN_LOW_POWER", "1", 1);
    // Each forked process must call this function.
    if_fail(ze.sInit(0), reterr);
    // Discovering all drivers
    if_fail(ze.sDriverGet(&drivers_count, NULL), reterr);
    debug("#drivers %u", drivers_count);
    drivers = calloc(drivers_count, sizeof(zes_driver_handle_t));
    if_fail(ze.sDriverGet(&drivers_count, drivers), reterr);

    // Counting to allocate
    for (dr = 0; dr < drivers_count; ++dr) {
        // Counting devices per driver
        if_fail(ze.sDeviceGet(drivers[dr], &devices_count, NULL), reterr);
        hs_count += devices_count;
    }
    // No GPUs
    if (hs_count == 0) {
        return_msg(EAR_ERROR, "No GPUs detected");
    }
    // Allocating all the devices
    debug("#devices %u", hs_count);
    devices     = calloc(hs_count, sizeof(zes_device_handle_t));
    devices_tmp = calloc(hs_count, sizeof(zes_device_handle_t));
    hs          = calloc(hs_count, sizeof(ze_handlers_t));
    hs_count    = 0;
    // Filling
    for (dr = 0; dr < drivers_count; ++dr) {
        devices_count = 0;
        // Discovering all devices
        if_fail(ze.sDeviceGet(drivers[dr], &devices_count, NULL), reterr);
        if_fail(ze.sDeviceGet(drivers[dr], &devices_count, devices_tmp), reterr);

        // We have to reorder devices based on pci addresses. We will ask for pci info,
        // reorder in devices vector and uses devices. devices_tmp will be released
        for (dv = 0; dv < devices_count; ++dv) {
            // Getting PCI properties
            ze.sDevicePciGetProperties(devices_tmp[dv], &hs[dv].spci_props);
        }
        // Sort using pci
        oneapi_reorder_by_pci(devices_count, devices_tmp, devices, hs);
        // Release
        free(devices_tmp);

        for (dv = 0; dv < devices_count; ++dv) {
            debug("-- DEVICE%d -----------------------------", dv);
            // Getting properties to see if is a GPU device
            if_fail(ze.sDeviceGetProperties(devices[dv], &hs[dv].sdevice_props), reterr);
            if_fail(ze.sDevicePciGetProperties(devices[dv], &hs[dv].spci_props), reterr);
            // If not a GPU (this don't work for Sysman, you have to check
            // zes_device_ext_properties_t, but there is no way to get that
            // structure yet.
            if (ZE_DEVICE_TYPE_GPU != hs[dv].sdevice_props.core.type) {
                // continue;
            }
            // Copying main (single) domains
            memcpy(&hs[hs_count].sdriver, &drivers[dr], sizeof(zes_driver_handle_t));
            memcpy(&hs[hs_count].sdevice, &devices[dv], sizeof(zes_device_handle_t));
            // Counting specific domains
            if_fail(ze.sDeviceEnumPowerDomains(devices[dv], &hs[hs_count].spowers_count, NULL), );
            if_fail(ze.sDeviceEnumFrequencyDomains(devices[dv], &hs[hs_count].sfreqs_count, NULL), );
            if_fail(ze.sDeviceEnumEngineGroups(devices[dv], &hs[hs_count].sengines_count, NULL), );
            if_fail(ze.sDeviceEnumTemperatureSensors(devices[dv], &hs[hs_count].stemps_count, NULL), );
            if_fail(ze.sDeviceEnumPsus(devices[dv], &hs[hs_count].spsus_count, NULL), );
            // Allocating specific domains
            hs[hs_count].spowers  = calloc(hs[hs_count].spowers_count, sizeof(zes_pwr_handle_t));
            hs[hs_count].sfreqs   = calloc(hs[hs_count].sfreqs_count, sizeof(zes_freq_handle_t));
            hs[hs_count].sengines = calloc(hs[hs_count].sengines_count, sizeof(zes_engine_handle_t));
            hs[hs_count].stemps   = calloc(hs[hs_count].stemps_count, sizeof(zes_temp_handle_t));
            hs[hs_count].spsus    = calloc(hs[hs_count].spsus_count, sizeof(zes_psu_handle_t));
            // Getting specific domains
            if_fail(ze.sDeviceEnumPowerDomains(devices[dv], &hs[hs_count].spowers_count, hs[hs_count].spowers), );
            if_fail(ze.sDeviceEnumFrequencyDomains(devices[dv], &hs[hs_count].sfreqs_count, hs[hs_count].sfreqs), );
            if_fail(ze.sDeviceEnumEngineGroups(devices[dv], &hs[hs_count].sengines_count, hs[hs_count].sengines), );
            if_fail(ze.sDeviceEnumTemperatureSensors(devices[dv], &hs[hs_count].stemps_count, hs[hs_count].stemps), );
            if_fail(ze.sDeviceEnumPsus(devices[dv], &hs[hs_count].spsus_count, hs[hs_count].spsus), );
            // Allocating properties
            hs[hs_count].spowers_props  = calloc(hs[hs_count].spowers_count, sizeof(zes_power_properties_t));
            hs[hs_count].sfreqs_props   = calloc(hs[hs_count].sfreqs_count, sizeof(zes_freq_properties_t));
            hs[hs_count].sengines_props = calloc(hs[hs_count].sengines_count, sizeof(zes_engine_properties_t));
            hs[hs_count].stemps_props   = calloc(hs[hs_count].stemps_count, sizeof(zes_temp_properties_t));
            // Getting properties
            for (dp = 0; dp < hs[hs_count].spowers_count; ++dp) {
                hs[hs_count].spowers_props[dp].stype = ZES_STRUCTURE_TYPE_POWER_PROPERTIES;
                if_fail(ze.sPowerGetProperties(hs[hs_count].spowers[dp], &hs[hs_count].spowers_props[dp]), );
            }
            for (dp = 0; dp < hs[hs_count].sfreqs_count; ++dp) {
                hs[hs_count].sfreqs_props[dp].stype = ZES_STRUCTURE_TYPE_FREQ_PROPERTIES;
                if_fail(ze.sFrequencyGetProperties(hs[hs_count].sfreqs[dp], &hs[hs_count].sfreqs_props[dp]), );
            }
            for (dp = 0; dp < hs[hs_count].sengines_count; ++dp) {
                hs[hs_count].sengines_props[dp].stype = ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES;
                if_fail(ze.sEngineGetProperties(hs[hs_count].sengines[dp], &hs[hs_count].sengines_props[dp]), );
            }
            for (dp = 0; dp < hs[hs_count].stemps_count; ++dp) {
                hs[hs_count].stemps_props[dp].stype = ZES_STRUCTURE_TYPE_TEMP_PROPERTIES;
                if_fail(ze.sTemperatureGetProperties(hs[hs_count].stemps[dp], &hs[hs_count].stemps_props[dp]), );
            }
            // Card power domain
            if (ze.sDeviceGetCardPowerDomain(devices[dv], &hs[hs_count].scard) == ZE_RESULT_SUCCESS) {
                hs[hs_count].scard_count = 1;
            }
            // Getting the serial (this don't work for Sysman)
            // hs[hs_count].uuid = hash(hs[dv].sdevice_props.uuid.id);
            // So we are going to replace it by PCI address: 256 buses (8 bits),
            // 32 devices (6 bits) and 8 functions (3 bits) and domain (?).
            hs[hs_count].uuid = 0LLU;
            hs[hs_count].uuid = (hs[dv].spci_props.address.domain << 24) | (hs[dv].spci_props.address.bus << 16) |
                                (hs[dv].spci_props.address.device <<  8) | (hs[dv].spci_props.address.function);
            // Last printings
            print_sysman_device_info(dv);

            hs_count++;
        }
    }
    debug("----------------------------------------");
    free(devices);
    free(drivers);
    return EAR_SUCCESS;
}

static state_t static_init(ze_t *ze_in, int sysman)
{
    static state_t s = EAR_ERROR;
#ifndef ONEAPI_BASE
    return EAR_ERROR;
#endif
    while (pthread_mutex_trylock(&lock));
    if (initialized) {
        goto leave;
    }
    if (state_ok(s = static_open())) {
        if (!sysman) s = static_core_init();
        if ( sysman) s = static_sysman_init();
    }
    initialized = 1;
leave:
    pthread_mutex_unlock(&lock);
    if (state_ok(s)) {
        if (ze_in != NULL) {
            memcpy(ze_in, &ze, sizeof(ze_t));
        }
    }
    return s;
}

state_t oneapi_open_core(ze_t *ze_in)
{
    return static_init(ze_in, 0);
}

state_t oneapi_open_sysman(ze_t *ze_in)
{
    return static_init(ze_in, 1);
}

state_t oneapi_get_handlers(ze_handlers_t **devs, uint *devs_count)
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

char *oneapi_strerror(ze_result_t z)
{
    // ze.StrError(devs[dv].driver, (const char **) &pointer);
    if (z == 0x78000001)
        return "uninitialized driver";
    if (z == 0x78000003)
        return "unsupported feature";
    if (z == 0x78000004)
        return "invalid argument";
    if (z == 0x78000005)
        return "invalid handler";
    return "unknown error";
}

int oneapi_is_privileged()
{
    return (getuid() == 0);
}
