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

#ifndef METRICS_COMMON_ONEAPI_H
#define METRICS_COMMON_ONEAPI_H

#ifdef ONEAPI_BASE
#include <level_zero/ze_api.h>
#include <level_zero/zes_ddi.h>
#endif
#include <common/types.h>
#include <common/states.h>

#ifndef ONEAPI_BASE
#define ZE_RESULT_SUCCESS      0
#define ZE_DEVICE_TYPE_GPU     0
#define ZES_FREQ_DOMAIN_GPU    0
#define ZES_FREQ_DOMAIN_MEMORY 0
#define ZES_ENGINE_GROUP_ALL   0

typedef struct ze_info_s {
    int type;
    int activeTime;
    int energy;
    int actual;
    char *name;
    char *vendorId;
    int deviceId;
    int driverVersion;
    uint maxClockRate;
    uint maxBusWidth;
    uint coreClockRate;
    ulong maxMemAllocSize;
    ulong totalSize;
} ze_info_t;

typedef int ze_result_t;
typedef int ze_init_flags_t;
typedef int ze_device_handle_t;
typedef ze_info_t ze_device_properties_t;
typedef ze_info_t ze_device_memory_properties_t;
typedef int ze_driver_handle_t;
typedef ze_info_t ze_driver_properties_t;
typedef int ze_driver_extension_properties_t;
typedef int ze_api_version_t;
typedef int zes_device_handle_t;
typedef int zes_engine_handle_t;
typedef ze_info_t zes_engine_stats_t;
typedef ze_info_t zes_engine_properties_t;
typedef int zes_freq_handle_t;
typedef ze_info_t zes_freq_properties_t;
typedef ze_info_t zes_freq_state_t;
typedef int zes_pwr_handle_t;
typedef int zes_power_properties_t;
typedef ze_info_t zes_power_energy_counter_t;
typedef int zes_temp_handle_t;
#endif

typedef struct ze_s {
    ze_result_t (*Init)                    (ze_init_flags_t flags);
    ze_result_t (*DriverGet)               (uint *n, ze_driver_handle_t *ph);
    ze_result_t (*DeviceGet)               (ze_driver_handle_t h, uint *n, ze_device_handle_t *ph);
    ze_result_t (*DeviceGetProperties)     (ze_device_handle_t h, ze_device_properties_t *props);
    ze_result_t (*sDeviceEnumPowerDomains) (zes_device_handle_t h, uint *n, zes_pwr_handle_t *ph);
    ze_result_t (*sPowerGetEnergyCounter)  (zes_pwr_handle_t h, zes_power_energy_counter_t *info);
    ze_result_t (*sDeviceEnumFrequencyDomains) (zes_device_handle_t h, uint *n, zes_freq_handle_t *ph);
    ze_result_t (*sFrequencyGetProperties) (zes_freq_handle_t h, zes_freq_properties_t *props);
    ze_result_t (*sFrequencyGetState)      (zes_freq_handle_t h, zes_freq_state_t *info);
    ze_result_t (*sDeviceEnumEngineGroups) (zes_device_handle_t h, uint *n, zes_engine_handle_t *ph);
    ze_result_t (*sEngineGetProperties)    (zes_engine_handle_t h, zes_engine_properties_t *props);
    ze_result_t (*sEngineGetActivity)      (zes_engine_handle_t h, zes_engine_stats_t *info);
    ze_result_t (*DriverGetApiVersion)     (ze_driver_handle_t h, ze_api_version_t *v);
    ze_result_t (*DriverGetProperties)     (ze_driver_handle_t h, ze_driver_properties_t *p);
    ze_result_t (*DriverGetExtensionProperties) (ze_driver_handle_t h, uint *count, ze_driver_extension_properties_t *e);
    ze_result_t (*DeviceGetMemoryProperties) (ze_device_handle_t h, uint *count, ze_device_memory_properties_t *p);
    ze_result_t (*DeviceGetSubDevices)     (ze_device_handle_t h, uint *p, ze_device_handle_t *hs);
} ze_t;

state_t oneapi_open(ze_t *ze);

state_t oneapi_close();

state_t oneapi_get_devices(zes_device_handle_t **devs, uint *devs_count);

int oneapi_is_privileged();

#endif //METRICS_COMMON_ONEAPI_H
