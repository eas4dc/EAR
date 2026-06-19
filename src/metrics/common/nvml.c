/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <metrics/common/nvml.h>

// Provisional
#define HACK_NVML_FILE "HACK_NVML_FILE"

#ifndef CUDA_BASE
#define NVML_PATH            ""
#define NVML_SUCCESS         0
#define NVML_FEATURE_ENABLED 0
#define NVML_CLOCK_MEM       0
#define NVML_CLOCK_SM        0
#define NVML_TEMPERATURE_GPU 0
#define NVML_N               0
#else
#define NVML_PATH CUDA_BASE
#define NVML_N    37
#endif

static const char *nvml_names[] = {
    "nvmlInit_v2", // NP (No Permissions)
    "nvmlShutdown",
    "nvmlDeviceGetCount_v2",
    "nvmlDeviceGetHandleByIndex_v2", // NP
    "nvmlDeviceGetSerial",
    "nvmlDeviceGetPowerManagementMode",
    "nvmlDeviceGetPowerUsage",
    "nvmlDeviceGetClockInfo",
    "nvmlDeviceGetTemperature",
    "nvmlDeviceGetUtilizationRates",
    "nvmlDeviceGetComputeRunningProcesses", // NP
    "nvmlDeviceGetDefaultApplicationsClock",
    "nvmlDeviceGetSupportedMemoryClocks",
    "nvmlDeviceGetSupportedGraphicsClocks",
    "nvmlDeviceGetClock",
    "nvmlDeviceSetGpuLockedClocks",    // NP
    "nvmlDeviceSetApplicationsClocks", // NP
    "nvmlDeviceResetApplicationsClocks",
    "nvmlDeviceResetGpuLockedClocks",
    "nvmlDeviceGetPowerManagementLimit",
    "nvmlDeviceGetPowerManagementDefaultLimit",
    "nvmlDeviceGetPowerManagementLimitConstraints",
    "nvmlDeviceSetPowerManagementLimit",
    "nvmlErrorString",
    "nvmlDeviceGetArchitecture",
    "nvmlGpmMetricsGet", // GPM
    "nvmlGpmSampleFree",
    "nvmlGpmSampleAlloc",
    "nvmlGpmSampleGet",
    "nvmlGpmQueryDeviceSupport",
    "nvmlDeviceGetName", // MIG
    "nvmlDeviceGetMigMode",
    "nvmlDeviceGetMaxMigDeviceCount",
    "nvmlDeviceGetMigDeviceHandleByIndex",
    "nvmlDeviceGetUUID",
    "nvmlDeviceGetMemoryInfo_v2",
    "nvmlDeviceGetComputeRunningProcesses_v3",
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static void         *handler;
static nvmlDevice_t *handlers;
static uint          handlers_count;
static ullong       *serials;
static nvml_t        nvml;
static uint          counter; // Counts the times this class has been opened

static state_t static_open()
{
    #define open_test(path)                                                                                                \
    debug("Opening %s", path);                                                                                         \
    if ((handler = plug_open2(path, (void **) &nvml, nvml_names, NVML_N, RTLD_NOW | RTLD_LOCAL)) != NULL) {            \
        return EAR_SUCCESS;                                                                                            \
    }
    #define build_path_and_test(base_path)                                                                                 \
    if (base_path) {                                                                                                   \
        char path[MAX_PATH_SIZE];                                                                                      \
        strncpy(path, base_path, sizeof(path) - 1);                                                                    \
        strncat(path, "/lib64/libnvidia-ml.so", sizeof(path) - strlen(path) - 1);                                      \
        open_test(path);                                                                                               \
        strncat(path, ".1", sizeof(path) - strlen(path) - 1);                                                          \
        open_test(path);                                                                                               \
    }
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
    open_test("/usr/lib/aarch64-linux-gnu/libnvidia-ml.so");
    open_test("/.singularity.d/libs/libnvidia-ml.so") open_test("/.singularity.d/libs/libnvidia-ml.so.1")
        open_test(EAR_INSTALL_PATH "/lib/deps/libnvidia-ml.so.1");
    open_test(EAR_INSTALL_PATH "/lib/deps/libnvidia-ml.so");
    return_msg(EAR_ERROR, "Can not load libnvidia-ml.so");
}

static state_t static_free(state_t s, char *error)
{
    if (handlers != NULL) {
        free(handlers);
        handlers = NULL;
    }
    if (serials != NULL) {
        free(serials);
        serials = NULL;
    }
    // If counter is 1 it means NVML was initialized
    if (counter) {
        nvml.Shutdown();
    }
    memset(&nvml, 0, sizeof(nvml_t));
    if (handler != NULL) {
        dlclose(handler);
        handler = NULL;
    }
    if (error == NULL) {
        return s;
    }
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
    if ((r = nvml.Count(&handlers_count)) != NVML_SUCCESS) {
        debug("nvml.Count %u", handlers_count);
        return static_free(EAR_ERROR, (char *) nvml.ErrorString(r));
    }
    debug("nvml.Count %u", handlers_count);
    if (((int) handlers_count) <= 0) {
        return static_free(EAR_ERROR, Generr.gpus_not);
    }
    if ((handlers = calloc(handlers_count, sizeof(nvmlDevice_t))) == NULL) {
        return static_free(EAR_ERROR, strerror(errno));
    }
    if ((serials = calloc(handlers_count, sizeof(ullong))) == NULL) {
        return static_free(EAR_ERROR, strerror(errno));
    }
    // Some fillings
    for (d = 0; d < handlers_count; ++d) {
        // By now we now the MIG devices can not be read. So, these
        if ((r = nvml.Handle(d, &handlers[d])) != NVML_SUCCESS) {
            debug("nvmlDeviceGetHandleByIndex returned %d (%s)", r, nvml.ErrorString(r));
            return static_free(EAR_ERROR, (char *) nvml.ErrorString(r));
        }
        if ((r = nvml.GetSerial(handlers[d], buffer, 32)) == NVML_SUCCESS) {
            serials[d] = (ullong) atoll(buffer);
            debug("Dev %d serial %llu", d, serials[d]);
        }
    }
    return EAR_SUCCESS;
}

state_t nvml_open(nvml_t *nvml_in)
{
    state_t s = EAR_SUCCESS;
    #ifndef CUDA_BASE
    debug("No CUDA_BASE path provided");
    return EAR_ERROR;
    #endif
    while (pthread_mutex_trylock(&lock));
    if (counter) {
        ++counter;
        goto fini;
    }
    if (state_ok(s = static_open())) {
        if (state_ok(s = static_init())) {
            debug("Initialized NVML");
            counter = 1;
        }
    }
    if (state_fail(s)) {
        static_free(EAR_ERROR, NULL);
    }
fini:
    pthread_mutex_unlock(&lock);
    if (counter && nvml_in != NULL) {
        memcpy(nvml_in, &nvml, sizeof(nvml_t));
    }
    return s;
}

state_t nvml_close()
{
    if (counter > 0) {
        if (counter == 1) {
            static_free(EAR_SUCCESS, NULL);
        }
        --counter;
    }
    return EAR_SUCCESS;
}

static void get_device_info(gpu_devs_t *dev, uint is_subdevice)
{
    nvmlMemory_v2_t mem = {0};
    char serial[32]     = {0};

    nvml.GetName(*((nvmlDevice_t *) dev->handler), dev->name, sizeof(dev->name));
    nvml.GetUUID(*((nvmlDevice_t *) dev->handler), dev->uuid, sizeof(dev->uuid));
    nvml.GetMemoryInfo_v2(*((nvmlDevice_t *) dev->handler), &mem);
    nvml.GetMaxMigDeviceCount(*((nvmlDevice_t *) dev->handler), &dev->subdevices_count);
    nvml.GetSerial(*((nvmlDevice_t *) dev->handler), serial, 32);
    dev->serial         = (ullong) atoll(serial);
    dev->is_readable    = is_subdevice ? 0 : 1;
    dev->is_subdevice   = is_subdevice;
    dev->has_subdevices = is_subdevice ? 0 : dev->subdevices_count > 0;
    dev->mem_total      = mem.total;
}

void nvml_get_devices(gpu_devs_t **devs, uint *devs_count_in, int add_subdevices)
{
    nvmlReturn_t r;
    uint aux;
    int m; // main
    int s; // sub
    int n = 0;

    for (m = 0; m < handlers_count; ++m) {
        aux = 0U;
        // Getting the maximum number of MIG devices per MAIN device
        if ((r = nvml.GetMaxMigDeviceCount(handlers[m], &aux)) != NVML_SUCCESS) {
            debug("nvml.GetMaxMigDeviceCount returned %d (%s)", r, nvml.ErrorString(r));
        }
        debug("NVML device %d has a max of %u subdevices", m, aux);
        *devs_count_in += aux;
    }
    *devs_count_in += handlers_count;
    debug("Detected a maximum of %u NVML devices including MIGs", *devs_count_in);
    *devs = calloc(*devs_count_in, sizeof(gpu_devs_t));
    // Filling the data per device including MIGs
    for (m = 0; m < handlers_count; ++m) {
        (*devs)[n].handler = &handlers[m];
        get_device_info(&(*devs)[n], 0);
        // The number of sub-devices is provided by get_device_info() function
        for (s = 0; add_subdevices && s < (*devs)[n].subdevices_count; ++s) {
            (*devs)[n + 1 + s].handler = calloc(1, sizeof(nvmlDevice_t));
            // Getting sub-device handler
            r = nvml.GetMigDeviceHandleByIndex(handlers[m], s, (*devs)[n + 1 + s].handler);
            if (r == NVML_ERROR_NOT_FOUND) {
                break;
            }
            get_device_info(&(*devs)[n + 1 + s], 1);
        }
        // Continuing from last sub-device
        n = n + 1 + s;
    }
    *devs_count_in = n;
}

void nvml_get_handlers(nvmlDevice_t **devs, uint *devs_count_in)
{
    if (devs != NULL) {
        *devs = calloc(handlers_count, sizeof(nvmlDevice_t));
        memcpy(*devs, handlers, sizeof(nvmlDevice_t) * handlers_count);
    }
    if (devs_count_in != NULL) {
        *devs_count_in = handlers_count;
    }
}

void nvml_get_serials(const ullong **serials_in)
{
    *serials_in = serials;
}

int nvml_is_serial(ullong serial)
{
    int d;
    for (d = 0; d < handlers_count; ++d) {
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
    if ((r = nvml.GetAPIRestriction(devs[d], NVML_APP_CLOCKS_PERMISSIONS    , &e)) != NVML_SUCCESS) {}
    if ((r = nvml.GetAPIRestriction(devs[d], NVML_BOOSTED_CLOCKS_PERMISSIONS, &e)) != NVML_SUCCESS) {}
#endif
    // Provisional
    return (getuid() == 0);
}
