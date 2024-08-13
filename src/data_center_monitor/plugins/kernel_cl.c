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

#include <CL/cl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/utils/string.h>
#include <common/system/plugin_manager.h>

static pthread_t         thread;
static uint             *devs_running;
static uint              devs_count;
static cl_device_id     *devices;
static uint              is_running;
static uint              lay_time;
static cl_command_queue *command_queue;

declr_up_get_tag()
{
    *tag = "kernel_cl";
    *tags_deps = NULL;
}

const char *CL_KERNEL =
"__kernel void cl_kernel(__global int *C)\n"
"{\n"
    "int id = get_global_id(0);\n"
    "unsigned long long i;\n"
"\n"
    "C[id] = id;\n"
    "for (i = 0; i < 1000000000LLU; ++i) {\n"
        "C[id] += id * ((int) i % 9);\n"
    "\n}"
"\n}";

int kcl_count_devices()
{
    static int init = 0;    
    cl_platform_id *platforms;
    uint plats_count;
    int p;

    if (init) {
        goto retdevs;
    }

    clGetPlatformIDs(0, NULL, &plats_count);
    platforms = (cl_platform_id *) calloc(plats_count, sizeof(cl_platform_id));
    clGetPlatformIDs(plats_count, platforms, NULL);

    for (p = 0; p < plats_count; ++p) {
        clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 0, NULL, &devs_count);
        if (devs_count == 0) {
            continue;
        }
        devices = (cl_device_id *) calloc(devs_count, sizeof(cl_device_id));
        clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, devs_count, devices, NULL);
        break; 
    }
    if (devs_count == 0) {
        goto retdevs;
    }
    command_queue = (cl_command_queue *) calloc(devs_count, sizeof(cl_command_queue));
    init = 1; 
retdevs:
    return devs_count;
}

static void *static_kernel_execute(void *x)
{
    size_t            global_size;
    size_t            local_size;
    cl_int            err;
    cl_context        context;
    cl_program        program;
    cl_kernel         kernel;
    cl_mem            r_mem_obj;
    int              *result;   
    int               d; 

    #define LIST_SIZE         1024
    #define MAX_SOURCE_SIZE  (0x100000)
    
    // Context -> Program -> Kernel -> CommandQueue
    result        = (int *) malloc(sizeof(int) * LIST_SIZE);
    global_size   = LIST_SIZE;
    local_size    = 64;
  
    // Main program 
    while (is_running)
    {
        context       = clCreateContext(NULL, devs_count, devices, NULL, NULL, &err);
        r_mem_obj     = clCreateBuffer(context, CL_MEM_WRITE_ONLY, LIST_SIZE * sizeof(int), NULL, &err);
        program       = clCreateProgramWithSource(context, 1, &CL_KERNEL, NULL, &err);
        err           = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
        kernel        = clCreateKernel(program, "cl_kernel", &err);
        err           = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &r_mem_obj);

        for (d = 0; d < devs_count; ++d) {
            if (!devs_running[d]) {
                continue;
            }
            command_queue[d] = clCreateCommandQueue(context, devices[d], 0, &err);
            err = clEnqueueNDRangeKernel(command_queue[d], kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            err = clEnqueueReadBuffer(command_queue[d], r_mem_obj, CL_TRUE, 0, LIST_SIZE * sizeof(int), result, 0, NULL, NULL);
            err = clFlush(command_queue[d]);
            err = clFinish(command_queue[d]);
            err = clReleaseCommandQueue(command_queue[d]);
            err = clReleaseKernel(kernel);
            err = clReleaseProgram(program);
            err = clReleaseMemObject(r_mem_obj);
            err = clReleaseContext(context);
            #if 1 
            int i;
            for(i = 0; i < LIST_SIZE; i++) {
                printf("R[%d] = %d\n", i, result[i]);
            }
            #endif
        } 
        sleep(lay_time);
    }
    // Clean up
    free(result);

    return NULL;
}

declr_up_action_init(_kernel_cl)
{
    char **args = (char **) data;
    if ((devs_count = kcl_count_devices()) == 0) {
        return "[D] No devices detected";
    }
    devs_running = calloc(devs_count, sizeof(uint));
    // Running devices
    if (args == NULL || !rantoa(args[0], devs_running, devs_count)) {
        // If the list is empty, we are selecting the GPU0.
        devs_running[0] = 1;
    }
    if (ARG(args, 1)) {
        lay_time = (uint) atoi(args[1]);
    }
    is_running = 1;
    pthread_create(&thread, NULL, static_kernel_execute, NULL);
    return NULL;
}
