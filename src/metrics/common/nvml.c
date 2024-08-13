/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


//#define SHOW_DEBUGS 1
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
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
#define NVML_N               29
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
    "nvmlDeviceGetArchitecture",
    "nvmlGpmMetricsGet", //GPM
    "nvmlGpmSampleFree",
    "nvmlGpmSampleAlloc",
    "nvmlGpmSampleGet",
    "nvmlGpmQueryDeviceSupport",
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static nvmlDevice_t   *devices;
static ullong         *serials;
static nvml_t          nvml;
static uint            devs_count;
static uint            ok;

static state_t static_open()
{
    #define open_test(path) \
        debug("Opening %s", path); \
        if (state_ok(plug_open(path, (void **) &nvml, nvml_names, NVML_N, RTLD_NOW | RTLD_LOCAL))) { \
            return EAR_SUCCESS; \
        }

#define build_path_and_test(base_path) \
	if (base_path) {\
		char path[MAX_PATH_SIZE];\
		strncpy(path, base_path, sizeof(path));\
		strncat(path, "/lib64/libnvidia-ml.so", sizeof(path) - strlen(path) - 1);\
		open_test(path);\
		strncat(path, ".1", sizeof(path) - strlen(path) - 1);\
		open_test(path);}

    // Looking for nvidia library in tipical paths.
    open_test(ear_getenv(HACK_NVML_FILE));

		char *cuda_root = ear_getenv("CUDA_ROOT");
		char *cuda_home = ear_getenv("CUDA_HOME");

		build_path_and_test(cuda_root);
		build_path_and_test(cuda_home);

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
		open_test("/.singularity.d/libs/libnvidia-ml.so")
		open_test("/.singularity.d/libs/libnvidia-ml.so.1")
    open_test(EAR_INSTALL_PATH "/lib/deps/libnvidia-ml.so.1");
    open_test(EAR_INSTALL_PATH "/lib/deps/libnvidia-ml.so");
    return_msg(EAR_ERROR, "Can not load libnvidia-ml.so");
}

static state_t static_free(state_t s, char *error)
{
    return_msg(s, error);
}

static state_t static_init()
{
    char buffer[32];
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
    if ((serials = calloc(devs_count, sizeof(ullong))) == NULL) {
        return static_free(EAR_ERROR, strerror(errno));
    }
    // Some fillings
    for (d = 0; d < devs_count; ++d) {
        if ((r = nvml.Handle(d, &devices[d])) != NVML_SUCCESS) {
            debug("nvmlDeviceGetHandleByIndex returned %d (%s)",
                  r, nvml.ErrorString(r));
            return static_free(EAR_ERROR, (char *) nvml.ErrorString(r));
        }
        if ((r = nvml.GetSerial(devices[d], buffer, 32)) == NVML_SUCCESS) {
            serials[d] = (ullong) atoll(buffer);
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
    debug("Initialized NVML");
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

void nvml_get_serials(const ullong **serials_in)
{
    *serials_in = serials;
}

int nvml_is_serial(ullong serial)
{
    int d;
    for (d = 0; d < devs_count; ++d) {
        if (serials[d] == serial) {
            return 1;
        }
    }
    return 0;
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
