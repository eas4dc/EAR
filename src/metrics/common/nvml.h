/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_NVML_H
#define METRICS_COMMON_NVML_H

#ifdef CUDA_BASE
#include <nvml.h>
#endif
#include <common/types.h>
#include <common/states.h>

#define NVML_ERROR_UNINIT_MSG       "NVML was not first initialized."
#define NVML_ERROR_INVALID_ARG_MSG  "A supplied argument is invalid."

#ifndef CUDA_BASE
#define NVML_SUCCESS                    0
#define NVML_ERROR_UNINITIALIZED        1
#define NVML_ERROR_INVALID_ARGUMENT     2
#define NVML_FEATURE_ENABLED            0
#define NVML_CLOCK_MEM                  0
#define NVML_CLOCK_SM                   0
#define NVML_CLOCK_GRAPHICS             0
#define NVML_CLOCK_ID_APP_CLOCK_TARGET	0
#define NVML_TEMPERATURE_GPU            0

#define NVML_DEVICE_ARCH_KEPLER  2
#define NVML_DEVICE_ARCH_MAXWELL 3
#define NVML_DEVICE_ARCH_PASCAL  4
#define NVML_DEVICE_ARCH_VOLTA   5
#define NVML_DEVICE_ARCH_TURING  6
#define NVML_DEVICE_ARCH_AMPERE  7
#define NVML_DEVICE_ARCH_ADA     8
#define NVML_DEVICE_ARCH_HOPPER  9
#define NVML_DEVICE_ARCH_UNKNOWN 0xffffffff

typedef int nvmlDevice_t;
typedef int nvmlDriverModel_t;
typedef int nvmlEnableState_t;
typedef int nvmlClockType_t;
typedef int nvmlTemperatureSensors_t;
typedef int nvmlProcessInfo_t;
typedef int nvmlClockId_t;
typedef int nvmlReturn_t;
typedef uint nvmlDeviceArchitecture_t;

typedef struct nvmlUtilization_s {
    int memory;
    int gpu;
} nvmlUtilization_t;
#endif // CUDA_BASE

#ifndef NVML_GPM_SUPPORT_VERSION
#define NVML_GPM_METRIC_MAX             1
#define NVML_GPM_METRICS_GET_VERSION    0

typedef int nvmlGpmSample_t;

typedef struct {
    uint version;
    uint isSupportedDevice;
} nvmlGpmSupport_t;

typedef struct {
    uint metricId;
    nvmlReturn_t nvmlReturn;
    double value;
    struct {
        char *longName;
        char *shortName;
        char *unit;
    } metricInfo;
} nvmlGpmMetric_t;

typedef struct {
    uint version;
    uint numMetrics;
    nvmlGpmSample_t sample1;
    nvmlGpmSample_t sample2;
    nvmlGpmMetric_t metrics[NVML_GPM_METRIC_MAX];
} nvmlGpmMetricsGet_t;
#endif

typedef struct nvml_s
{
    nvmlReturn_t (*Init)                         (void);
    nvmlReturn_t (*Count)                		 (uint *devCount);
    nvmlReturn_t (*Handle)               		 (uint index, nvmlDevice_t *device);
    nvmlReturn_t (*GetSerial)            		 (nvmlDevice_t dev, char *serial, uint length);
    nvmlReturn_t (*GetPowerMode)         		 (nvmlDevice_t dev, nvmlEnableState_t *mode);
    nvmlReturn_t (*GetPowerUsage)        		 (nvmlDevice_t dev, uint *power);
    nvmlReturn_t (*GetClocks)            		 (nvmlDevice_t dev, nvmlClockType_t type, uint *clock);
    nvmlReturn_t (*GetTemp)              		 (nvmlDevice_t dev, nvmlTemperatureSensors_t type, uint *temp);
    nvmlReturn_t (*GetUtil)              		 (nvmlDevice_t dev, nvmlUtilization_t *utilization);
    nvmlReturn_t (*GetProcs)             		 (nvmlDevice_t dev, uint *infoCount, nvmlProcessInfo_t *infos);
    nvmlReturn_t (*GetDefaultAppsClock)	 		 (nvmlDevice_t dev, nvmlClockType_t clockType, uint* mhz);
    nvmlReturn_t (*GetMemoryClocks)	             (nvmlDevice_t dev, uint *count, uint *mhz);
    nvmlReturn_t (*GetGraphicsClocks)            (nvmlDevice_t dev, uint mem_mhz, uint *count, uint *mhz);
    nvmlReturn_t (*GetClock)                     (nvmlDevice_t device, nvmlClockType_t clockType, nvmlClockId_t clockId, uint* mhz);
    nvmlReturn_t (*SetLockedClocks)              (nvmlDevice_t dev, uint min_mhz, uint max_mhz);
    nvmlReturn_t (*SetAppsClocks)                (nvmlDevice_t dev, uint mem_mgz, uint gpu_mhz);
    nvmlReturn_t (*ResetAppsClocks)              (nvmlDevice_t dev);
    nvmlReturn_t (*ResetLockedClocks)            (nvmlDevice_t dev);
    nvmlReturn_t (*GetPowerLimit)                (nvmlDevice_t dev, uint *watts);
    nvmlReturn_t (*GetPowerDefaultLimit) 		 (nvmlDevice_t dev, uint *watts);
    nvmlReturn_t (*GetPowerLimitConstraints)     (nvmlDevice_t dev, uint *min_watts, uint *max_watts);
    nvmlReturn_t (*SetPowerLimit)                (nvmlDevice_t dev, uint watts);
    char*        (*ErrorString)                  (nvmlReturn_t result);
    nvmlReturn_t (*GetArchitecture)              (nvmlDevice_t device, nvmlDeviceArchitecture_t* arch);
    nvmlReturn_t (*GpmMetricsGet)                (nvmlGpmMetricsGet_t *metricsGet); //GPM
    nvmlReturn_t (*GpmSampleFree)                (nvmlGpmSample_t gpmSample);
    nvmlReturn_t (*GpmSampleAlloc)               (nvmlGpmSample_t *gpmSample);
    nvmlReturn_t (*GpmSampleGet)                 (nvmlDevice_t device, nvmlGpmSample_t gpmSample);
    nvmlReturn_t (*GpmQueryDeviceSupport)        (nvmlDevice_t device, nvmlGpmSupport_t *gpmSupport);
} nvml_t;

state_t nvml_open(nvml_t *nvml);

state_t nvml_close();

state_t nvml_get_devices(nvmlDevice_t **devs, uint *devs_count);

void nvml_get_serials(const ullong **serials);

// Return if a serial is present in the GPU list.
int nvml_is_serial(ullong serial);

int nvml_is_privileged();

#endif //METRICS_COMMON_NVML_H
