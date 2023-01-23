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

#ifndef METRICS_COMMON_NVML_H
#define METRICS_COMMON_NVML_H

#ifdef CUDA_BASE
#include <nvml.h>
#endif
#include <common/types.h>
#include <common/states.h>

#ifndef CUDA_BASE
#define NVML_SUCCESS         0
#define NVML_FEATURE_ENABLED 0
#define NVML_CLOCK_MEM       0
#define NVML_CLOCK_SM        0
#define NVML_TEMPERATURE_GPU 0
#define NVML_CLOCK_GRAPHICS  0
#define NVML_CLOCK_ID_APP_CLOCK_TARGET 0

typedef int nvmlDevice_t;
typedef int nvmlDriverModel_t;
typedef int nvmlEnableState_t;
typedef int nvmlClockType_t;
typedef int nvmlTemperatureSensors_t;
typedef int nvmlProcessInfo_t;
typedef int nvmlClockId_t;
typedef int nvmlReturn_t;
typedef struct nvmlUtilization_s {
    int memory;
    int gpu;
} nvmlUtilization_t;
#endif

typedef struct nvml_s
{
    nvmlReturn_t (*Init)                (void);
    nvmlReturn_t (*Count)               (uint *devCount);
    nvmlReturn_t (*Handle)              (uint index, nvmlDevice_t *device);
    nvmlReturn_t (*GetSerial)           (nvmlDevice_t dev, char *serial, uint length);
    nvmlReturn_t (*GetPowerMode)        (nvmlDevice_t dev, nvmlEnableState_t *mode);
    nvmlReturn_t (*GetPowerUsage)       (nvmlDevice_t dev, uint *power);
    nvmlReturn_t (*GetClocks)           (nvmlDevice_t dev, nvmlClockType_t type, uint *clock);
    nvmlReturn_t (*GetTemp)             (nvmlDevice_t dev, nvmlTemperatureSensors_t type, uint *temp);
    nvmlReturn_t (*GetUtil)             (nvmlDevice_t dev, nvmlUtilization_t *utilization);
    nvmlReturn_t (*GetProcs)            (nvmlDevice_t dev, uint *infoCount, nvmlProcessInfo_t *infos);
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
} nvml_t;

state_t nvml_open(nvml_t *nvml);

state_t nvml_close();

state_t nvml_get_devices(nvmlDevice_t **devs, uint *devs_count);

int nvml_is_privileged();

#endif //METRICS_COMMON_NVML_H
