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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/system/symplug.h>
#include <metrics/common/nvml.h>

// Provisional
#define HACK_NVML_FILE       "HACK_NVML_FILE"

#ifndef CUDA_BASE
#define NVML_PATH            ""
#define NVML_SUCCESS         0
#define NVML_FEATURE_ENABLED 0
#define NVML_CLOCK_MEM       0
#define NVML_CLOCK_SM        0
#define NVML_TEMPERATURE_GPU 0
#define NVML_N               0
#else
#define NVML_PATH            CUDA_BASE
#define NVML_N               23
#endif

static const char *nvml_names[] =
{
    "nvmlInit_v2", //NP (No Permissions)
    "nvmlDeviceGetCount_v2",
    "nvmlDeviceGetHandleByIndex_v2", //NP
    "nvmlDeviceGetSerial",
    "nvmlDeviceGetPowerManagementMode",
    "nvmlDeviceGetPowerUsage",
    "nvmlDeviceGetClockInfo",
    "nvmlDeviceGetTemperature",
    "nvmlDeviceGetUtilizationRates",
    "nvmlDeviceGetComputeRunningProcesses", //NP
    "nvmlDeviceGetDefaultApplicationsClock",
    "nvmlDeviceGetSupportedMemoryClocks",
    "nvmlDeviceGetSupportedGraphicsClocks",
    "nvmlDeviceGetClock",
    "nvmlDeviceSetGpuLockedClocks", //NP
    "nvmlDeviceSetApplicationsClocks", //NP
    "nvmlDeviceResetApplicationsClocks",
    "nvmlDeviceResetGpuLockedClocks",
    "nvmlDeviceGetPowerManagementLimit",
    "nvmlDeviceGetPowerManagementDefaultLimit",
    "nvmlDeviceGetPowerManagementLimitConstraints",
    "nvmlDeviceSetPowerManagementLimit",
    "nvmlErrorString",
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static nvmlDevice_t   *devices;
static nvml_t          nvml;
static uint            devs_count;
static uint            ok;

static state_t static_open()
{
    #define open_test(path) \
        if (state_ok(plug_open(path, (void **) &nvml, nvml_names, NVML_N, RTLD_NOW | RTLD_LOCAL))) { \
            return EAR_SUCCESS; \
        }

    // Looking for nvidia library in tipical paths.
    open_test(getenv(HACK_NVML_FILE));
    open_test(NVML_PATH "/targets/x86_64-linux/lib/libnvidia-ml.so");
    open_test(NVML_PATH "/targets/x86_64-linux/lib/libnvidia-ml.so.1");
    open_test(NVML_PATH "/lib64/libnvidia-ml.so");
    open_test(NVML_PATH "/lib64/libnvidia-ml.so.1");
    open_test(NVML_PATH "/lib/libnvidia-ml.so");
    open_test(NVML_PATH "/lib/libnvidia-ml.so.1");
    open_test("/usr/lib64/libnvidia-ml.so");
    open_test("/usr/lib64/libnvidia-ml.so.1");
    open_test("/usr/lib/x86_64-linux-gnu/libnvidia-ml.so");
    open_test("/usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1");
    open_test("/usr/lib32/libnvidia-ml.so");
    open_test("/usr/lib32/libnvidia-ml.so.1");
    open_test("/usr/lib/libnvidia-ml.so");
    open_test("/usr/lib/libnvidia-ml.so.1");
    return_msg(EAR_ERROR, "Can not load libnvidia-ml.so");
}

static state_t static_free(state_t s, char *error)
{
    return_msg(s, error);
}

static state_t static_init()
{
    nvmlReturn_t r;
    int d;

    if ((r = nvml.Init()) != NVML_SUCCESS) {
        debug("nvml.Init");
        return static_free(EAR_ERROR, (char *) nvml.ErrorString(r));
    }
    if ((r = nvml.Count(&devs_count)) != NVML_SUCCESS) {
        debug("nvml.Count %u",devs_count);
        return static_free(EAR_ERROR, (char *) nvml.ErrorString(r));
    }
    if (((int) devs_count) <= 0) {
        return static_free(EAR_ERROR, Generr.gpus_not);
    }
    if ((devices = calloc(devs_count, sizeof(nvmlDevice_t))) == NULL) {
        return static_free(EAR_ERROR, strerror(errno));
    }
    for (d = 0; d < devs_count; ++d) {
        if ((r = nvml.Handle(d, &devices[d])) != NVML_SUCCESS) {
            debug("nvmlDeviceGetHandleByIndex returned %d (%s)",
                  r, nvml.ErrorString(r));
            return static_free(EAR_ERROR, (char *) nvml.ErrorString(r));
        }
    }
    return EAR_SUCCESS;
}

state_t nvml_open(nvml_t *nvml_in)
{
    state_t s = EAR_SUCCESS;
    #ifndef CUDA_BASE
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
    if (nvml_in != NULL) {
        memcpy(nvml_in, &nvml, sizeof(nvml_t));
    }
    return s;
}

state_t nvml_close()
{
    return EAR_SUCCESS;
}

state_t nvml_get_devices(nvmlDevice_t **devs, uint *devs_count_in)
{
    if (devs != NULL) {
        *devs = malloc(sizeof(nvmlDevice_t)*devs_count);
        memcpy(*devs, devices, sizeof(nvmlDevice_t)*devs_count);
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
    return EAR_SUCCESS;
}

int nvml_is_privileged()
{
    #if 0
    nvmlEnableState_t e;
    if ((r = nvml.GetAPIRestriction(devs[d], NVML_APP_CLOCKS_PERMISSIONS, &e)) != NVML_SUCCESS) {}
    if ((r = nvml.GetAPIRestriction(devs[d], NVML_BOOSTED_CLOCKS_PERMISSIONS, &e)) != NVML_SUCCESS) {}
    #endif
    // Provisional
    return (getuid() == 0);
}
