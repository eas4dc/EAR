/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

extern "C"
{
//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <cuda_runtime.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/system/plugin_manager.h>
 
static pthread_t  thread;
static int        is_running;
static uint      *devs_running;
static uint       devs_count;
static ullong     cycles;
static ullong     run_time;
static uint       lay_time;

__global__ void kcuda_kernel(ullong cycles)
{
    long long start = clock64();
    while (clock64() < (start + cycles));
}

int kcuda_is_running()
{
    return is_running;
}

int kcuda_count_devices()
{
    int devs_count = 0;
    cudaGetDeviceCount(&devs_count);
    return devs_count;
}

static void *static_kcuda_execute(void *x)
{
    static struct cudaDeviceProp prop;
    int d;  
 
    while (1)
    {
        is_running = 1;

        for (d = 0; d < devs_count; ++d) {
            debug("Running in GPU%d? %u", d, devs_running[d]);
            if (!devs_running[d]) {
                continue;
            }
            if (cudaSetDevice(d)) {
                printf("GPU%d error: %s\n", d, cudaGetErrorString(cudaGetLastError()));
                devs_running[d] = 0;
                continue;
            }
            if (cudaGetDeviceProperties(&prop, d) != cudaSuccess) {
                printf("GPU%d error: %s\n", d, cudaGetErrorString(cudaGetLastError()));
                devs_running[d] = 0;
                continue;
            }
            cycles = prop.clockRate * 1000LLU * run_time; 
            printf("Running GPU%d: %llu\n", d, cycles);
            kcuda_kernel<<<prop.multiProcessorCount, 64>>>(cycles);
        }
        if (cudaDeviceSynchronize() != cudaSuccess) {
            printf("CUDA error: %s\n", cudaGetErrorString(cudaGetLastError()));
            return NULL;
        }
        cudaDeviceReset();
        is_running = 0;
        sleep(lay_time);
    }

    return NULL;
}

int kcuda_execute(char **conf)
{
    run_time     = 10LLU;
    lay_time     =  2LLU; 
    devs_count   = kcuda_count_devices();
    devs_running = (uint *) calloc(devs_count, sizeof(uint));
    // Running devices
    if (conf == NULL || !rantoa(conf[0], devs_running, devs_count)) {
        // If the list is empty, we are selecting the GPU0.
        devs_running[0] = 1;
    }
    if (ARG(conf, 1)) run_time = (ullong) atoi(conf[1]);
    if (ARG(conf, 2)) lay_time = (uint)   atoi(conf[2]);
    pthread_create(&thread, NULL, static_kcuda_execute, NULL);    
    return 1;
}
}
