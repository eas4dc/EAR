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
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/common/cupti.h>

// Provisional
#define HACK_CUDA_FILE  "HACK_CUDA_FILE"
#define HACK_CUPTI_FILE "HACK_CUPTI_FILE"

#ifndef CUPTI_BASE
#define CUPTI_PATH   ""
#else
#define CUPTI_PATH   CUPTI_BASE
#endif
#define CUPTI_N      20
#define CUDA_N       4
#define CUDA_LIB     "libcuda.so"
#define CUPTI_LIB    "libcupti.so"

static const char *cuda_names[] = {
    "cuInit",
    "cuCtxGetDevice",
    "cuDeviceGetCount",
    "cuGetErrorString",
};

static const char *cupti_names[] = {
    "cuptiSubscribe",
    "cuptiEnableCallback",
    "cuptiEnableDomain",
    "cuptiMetricGetIdFromName",
    "cuptiMetricGetNumEvents",
    "cuptiMetricCreateEventGroupSets",
    "cuptiMetricGetAttribute",
    "cuptiMetricGetValue",
    "cuptiSetEventCollectionMode",
    "cuptiEventGroupSetsDestroy",
    "cuptiEventGroupSetAttribute",
    "cuptiEventGroupGetAttribute",
    "cuptiEventGroupEnable",
    "cuptiEventGroupDisable",
    "cuptiEventGroupSetDisable",
    "cuptiEventGroupReadAllEvents",
    "cuptiEventGetAttribute",
    "cuptiEventGroupAddEvent",
    "cuptiEventGroupCreate",
    "cuptiGetResultString",
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint            devs_count;
static cupti_t         cupti;
static cuda_t          cu;
static uint            ok;

static state_t static_open_cuda()
{
    #define _open_test_cuda(path) \
    debug("Openning %s", (char *) path); \
    if (state_ok(plug_open(path, (void **) &cu, cuda_names, CUDA_N, RTLD_NOW | RTLD_LOCAL))) { \
        return EAR_SUCCESS; \
    }
    #define open_test_cuda(path) \
    _open_test_cuda(path CUDA_LIB); \
    _open_test_cuda(path CUDA_LIB ".1");

    _open_test_cuda(NULL);
    _open_test_cuda(getenv(HACK_CUDA_FILE));
    open_test_cuda("/usr/lib64/");

    return_print(EAR_ERROR, "Can not load %s", CUDA_LIB);
}

static state_t static_open_cupti()
{
    #define _open_test(path) \
    debug("Openning %s", path); \
    if (state_ok(plug_open(path, (void **) &cupti, cupti_names, CUPTI_N, RTLD_NOW | RTLD_LOCAL))) { \
        return EAR_SUCCESS; \
    }
    #define open_test(path) \
    _open_test(path CUPTI_LIB); \
    _open_test(path CUPTI_LIB ".1");

    // Looking for cupti library in tipical paths.
    _open_test(getenv(HACK_CUPTI_FILE));
    open_test(CUPTI_PATH "/targets/x86_64-linux/lib/");
    open_test(CUPTI_PATH "/lib64/");
    open_test(CUPTI_PATH "/lib/");
    open_test("/usr/lib64/");
    open_test("/usr/lib/x86_64-linux-gnu/");
    open_test("/usr/lib32/");
    open_test("/usr/lib/");

    return_print(EAR_ERROR, "Can not load %s", CUPTI_LIB);
}

static state_t static_open()
{
    state_t s;
    if (state_fail(s = static_open_cuda())) {
        return s;
    }
    return static_open_cupti();
}

static state_t static_free(state_t s, char *error)
{
    return_msg(s, error);
}

static state_t static_init()
{
    CUresult s;
    char *str;

    if ((s = cu.Init(0)) != CUDA_SUCCESS) {
        cu.GetErrorString(s, (const char **) &str);
        return static_free(EAR_ERROR, str);
    }
    if ((s = cu.DeviceGetCount((int *) &devs_count)) != CUDA_SUCCESS) {
        cu.GetErrorString(s, (const char **) &str);
        return static_free(EAR_ERROR, str);
    }
    if (((int) devs_count) <= 0) {
        return static_free(EAR_ERROR, Generr.gpus_not);
    }
    debug("detected devices: %u", devs_count);
    return EAR_SUCCESS;
}

state_t cupti_open(cupti_t *cupti_in, cuda_t *cu_in)
{
    state_t s = EAR_SUCCESS;
    #ifndef CUPTI_BASE
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
    if (ok && cupti_in != NULL) {
        memcpy(cupti_in, &cupti, sizeof(cupti_t));
    }
    if (ok && cu_in != NULL) {
        memcpy(cu_in, &cu, sizeof(cuda_t));
    }
    return s;
}

void cupti_count_devices(uint *devs_count_in)
{
    *devs_count_in = devs_count;
}

state_t cupti_close()
{
    return EAR_SUCCESS;
}
