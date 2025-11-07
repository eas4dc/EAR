/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_ONEAPI_H
#define METRICS_COMMON_ONEAPI_H

#ifdef ONEAPI_BASE
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include <level_zero/zes_ddi.h>
#endif
#include <common/states.h>
#include <common/types.h>

#ifndef ONEAPI_BASE
#define ZE_RESULT_SUCCESS                         0
#define ZE_DEVICE_TYPE_GPU                        0
#define ZES_FREQ_DOMAIN_GPU                       0
#define ZES_FREQ_DOMAIN_MEMORY                    0
#define ZES_ENGINE_GROUP_ALL                      0
#define ZE_MAX_DEVICE_UUID_SIZE                   1
#define ZES_TEMP_SENSORS_MEMORY                   0
#define ZES_TEMP_SENSORS_GPU                      0
#define ZES_ENGINE_GROUP_ALL                      0
#define ZES_ENGINE_GROUP_COMPUTE_ALL              0
#define ZES_ENGINE_GROUP_COMPUTE_SINGLE           0
#define ZES_ENGINE_GROUP_RENDER_ALL               0
#define ZES_ENGINE_GROUP_RENDER_SINGLE            0
#define ZES_ENGINE_GROUP_3D_RENDER_COMPUTE_ALL    0
#define ZES_ENGINE_GROUP_3D_ALL                   0
#define ZES_ENGINE_GROUP_3D_SINGLE                0
#define ZES_ENGINE_GROUP_MEDIA_ALL                0
#define ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE      0
#define ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE      0
#define ZES_ENGINE_GROUP_MEDIA_ENHANCEMENT_SINGLE 0
#define ZES_ENGINE_GROUP_COPY_ALL                 0
#define ZES_ENGINE_GROUP_COPY_SINGLE              0
#define ZES_POWER_LEVEL_SUSTAINED                 0
#define ZES_POWER_LEVEL_UNKNOWN                   0
#define ZES_POWER_LEVEL_BURST                     0
#define ZES_POWER_LEVEL_PEAK                      0
#define ZES_POWER_LEVEL_INSTANTANEOUS             0
#define ZES_LIMIT_UNIT_UNKNOWN                    0
#define ZES_LIMIT_UNIT_CURRENT                    0
#define ZES_LIMIT_UNIT_POWER                      0
#define ZES_POWER_SOURCE_ANY                      0
#define ZES_POWER_SOURCE_MAINS                    0
#define ZES_POWER_SOURCE_BATTERY                  0
#define ZES_STRUCTURE_TYPE_POWER_PROPERTIES       0
#define ZES_STRUCTURE_TYPE_FREQ_PROPERTIES        0
#define ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES      0
#define ZES_STRUCTURE_TYPE_TEMP_PROPERTIES        0


typedef struct ze_info_s {
    char  *name;
    int    vendorId;
    char  *vendorName;
    char  *modelName;
    char  *brandName;
    char  *serialNumber;
    char  *boardNumber;
    int    subdeviceId;
    int    numSubdevices;
    int    onSubdevice;
    int    deviceId;
    int    driverVersion;
    uint   maxClockRate;
    uint   maxBusWidth;
    uint   coreClockRate;
    ulong  maxMemAllocSize;
    ulong  totalSize;
    struct {
        char *id;
    } uuid;
    int    type;
    int    stype;
    ulong  activeTime;
    ulong  timestamp;
    ulong  energy;
    double actual;
    uint   powerDC;
    uint   powerAC;
    uint   power;
    double max;
    double min;
    double tdp;
    double efficient;
    int    enabled;
    int    canControl;
    uint   interval;
    uint   minLimit;
    uint   maxLimit;
    uint   defaultLimit;
    uint   temperature;
    double maxTemperature;
    uint   enabledStateLocked;
    uint   level;
    double limit;
    struct {
        uint type;
    } core;
    struct {
        uint domain;
        uint bus;
        uint device;
        uint function;
    } address;
} ze_info_t;

// Core
typedef int       ze_result_t;
typedef int       ze_init_flags_t;
typedef int       ze_api_version_t;
typedef int       ze_driver_handle_t;
typedef ze_info_t ze_driver_properties_t;
typedef ze_info_t ze_driver_extension_properties_t;
typedef int       ze_device_handle_t;
typedef ze_info_t ze_device_properties_t;
typedef ze_info_t ze_device_memory_properties_t;
typedef int       ze_device_type_t;
// Sysman
typedef int       zes_init_flags_t;
typedef int       zes_driver_handle_t;
typedef int       zes_device_handle_t; // Cast from ze_device_handle_t directly
typedef ze_info_t zes_device_properties_t;
typedef ze_info_t zes_pci_properties_t;
typedef ze_info_t zes_process_state_t;
typedef int       zes_pwr_handle_t;
typedef ze_info_t zes_power_properties_t;
typedef ze_info_t zes_power_sustained_limit_t;
typedef ze_info_t zes_power_burst_limit_t;
typedef ze_info_t zes_power_peak_limit_t;
typedef ze_info_t zes_power_energy_counter_t;
typedef ze_info_t zes_power_limit_ext_desc_t;
typedef int       zes_engine_handle_t;
typedef ze_info_t zes_engine_properties_t;
typedef ze_info_t zes_engine_stats_t;
typedef int       zes_freq_handle_t;
typedef ze_info_t zes_freq_properties_t;
typedef ze_info_t zes_freq_range_t;
typedef ze_info_t zes_freq_state_t;
typedef int       zes_temp_handle_t;
typedef ze_info_t zes_temp_properties_t;
typedef int       zes_psu_handle_t;
typedef ze_info_t zes_psu_state_t;
#endif

typedef struct ze_handlers_s {
    ze_driver_handle_t      driver;
    ze_device_handle_t      device; // core handle
    ze_device_properties_t  device_props;
    ullong                  uuid; // auxiliar
    zes_driver_handle_t     sdriver;
    zes_device_handle_t     sdevice; // sysman handle
    zes_device_properties_t sdevice_props;
    zes_pci_properties_t    spci_props;
    zes_pwr_handle_t       *spowers;
    zes_power_properties_t *spowers_props;
    uint                    spowers_count;
    zes_pwr_handle_t        scard;
    uint                    scard_count;
    zes_freq_handle_t      *sfreqs;
    zes_freq_properties_t  *sfreqs_props;
    uint                    sfreqs_count;
    zes_engine_handle_t    *sengines;
    zes_engine_properties_t *sengines_props;
    uint                    sengines_count;
    zes_temp_handle_t      *stemps;
    zes_temp_properties_t  *stemps_props;
    uint                    stemps_count;
    zes_psu_handle_t       *spsus;
    uint                    spsus_count;
} ze_handlers_t;

typedef struct ze_s {
    // Core
    ze_result_t (*Init)                    (ze_init_flags_t flags);
    ze_result_t (*DriverGet)               (uint *n, ze_driver_handle_t *ph);
    ze_result_t (*DriverGetApiVersion)     (ze_driver_handle_t h, ze_api_version_t *v);
    ze_result_t (*DriverGetProperties)     (ze_driver_handle_t h, ze_driver_properties_t *p);
    ze_result_t (*DriverGetExtensionProperties)(ze_driver_handle_t h, uint *count, ze_driver_extension_properties_t *e);
    ze_result_t (*DeviceGet)               (ze_driver_handle_t h, uint *n, ze_device_handle_t *ph);
    ze_result_t (*DeviceGetProperties)     (ze_device_handle_t h, ze_device_properties_t *props);
    ze_result_t (*DeviceGetSubDevices)     (ze_device_handle_t h, uint *p, ze_device_handle_t *hs);
    ze_result_t (*DeviceGetMemoryProperties)(ze_device_handle_t h, uint *count, ze_device_memory_properties_t *p);
    ze_result_t (*StrError)                (ze_driver_handle_t h, const char **p);
    // Sysman
    ze_result_t (*sInit)                   (zes_init_flags_t flags);
    ze_result_t (*sDriverGet)              (uint *n, zes_driver_handle_t *ph);
    ze_result_t (*sDeviceGet)              (zes_driver_handle_t h, uint *n, zes_device_handle_t *ph);
    ze_result_t (*sDeviceGetProperties)    (zes_device_handle_t h, zes_device_properties_t *p);
    ze_result_t (*sDevicePciGetProperties) (zes_device_handle_t h, zes_pci_properties_t *p);
    ze_result_t (*sDeviceProcessesGetState)(zes_device_handle_t h, uint32_t *count, zes_process_state_t *p); // Get if is working
    ze_result_t (*sDeviceEnumPowerDomains) (zes_device_handle_t h, uint *n, zes_pwr_handle_t *ph);
    ze_result_t (*sDeviceGetCardPowerDomain) (zes_device_handle_t h, zes_pwr_handle_t *ph);
    ze_result_t (*sPowerGetProperties)     (zes_pwr_handle_t h, zes_power_properties_t *p); // Get min/max power limits
    ze_result_t (*sPowerGetLimits)         (zes_pwr_handle_t h, zes_power_sustained_limit_t *ps, zes_power_burst_limit_t *pb, zes_power_peak_limit_t *pp); // Get current sustained limit
    ze_result_t (*sPowerSetLimits)         (zes_pwr_handle_t h, const zes_power_sustained_limit_t *ps, const zes_power_burst_limit_t *pb, const zes_power_peak_limit_t *pp); // Set current sustained limit
    ze_result_t (*sPowerGetEnergyCounter)  (zes_pwr_handle_t h, zes_power_energy_counter_t *info); // Energy read
    ze_result_t (*sPowerGetLimitsExt)      (zes_pwr_handle_t h, uint32_t *count, zes_power_limit_ext_desc_t *ps);
    ze_result_t (*sPowerSetLimitsExt)      (zes_pwr_handle_t h, uint32_t *count, zes_power_limit_ext_desc_t *ps);
    ze_result_t (*sDeviceEnumEngineGroups) (zes_device_handle_t h, uint *n, zes_engine_handle_t *ph);
    ze_result_t (*sEngineGetProperties)    (zes_engine_handle_t h, zes_engine_properties_t *props);
    ze_result_t (*sEngineGetActivity)      (zes_engine_handle_t h, zes_engine_stats_t *info); // Utilization
    ze_result_t (*sDeviceEnumFrequencyDomains)(zes_device_handle_t h, uint *n, zes_freq_handle_t *ph);
    ze_result_t (*sFrequencyGetProperties) (zes_freq_handle_t h, zes_freq_properties_t *props);
    ze_result_t (*sFrequencyGetState)      (zes_freq_handle_t h, zes_freq_state_t *info); // Frequency read
    ze_result_t (*sFrequencyGetAvailableClocks)(zes_freq_handle_t h, uint32_t *count, double *f); // Get available frequency
    ze_result_t (*sFrequencyGetRange)      (zes_freq_handle_t h, zes_freq_range_t *l); // Get the current min/max frequency limits
    ze_result_t (*sFrequencySetRange)      (zes_freq_handle_t h, const zes_freq_range_t *l); // Set the current min/max frequency limits
    ze_result_t (*sDeviceEnumTemperatureSensors)(zes_device_handle_t h, uint32_t *count, zes_temp_handle_t *th);
    ze_result_t (*sTemperatureGetProperties)(zes_temp_handle_t h, zes_temp_properties_t *p);
    ze_result_t (*sTemperatureGetState)    (zes_temp_handle_t h, double *t);
    ze_result_t (*sDeviceEnumPsus)         (zes_device_handle_t h, uint32_t *count, zes_psu_handle_t *p); // Also temperature
    ze_result_t (*sPsuGetState)            (zes_psu_handle_t h, zes_psu_state_t *p);
} ze_t;

state_t oneapi_open_core(ze_t *ze);

state_t oneapi_open_sysman(ze_t *ze);

state_t oneapi_get_handlers(ze_handlers_t **handlers, uint *devs_count);

char *oneapi_strerror(ze_result_t z);

int oneapi_is_privileged();

#endif // METRICS_COMMON_ONEAPI_H