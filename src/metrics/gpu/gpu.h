/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_H
#define METRICS_GPU_H
/* clang-format off */

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <metrics/common/apis.h>

typedef struct gpu_devs_s {
    uint   index;
    int    index_device; // Reference to the main device if this is a subdevice
    uint   is_readable; // Metrics can be read
    uint   is_subdevice;
    uint   has_subdevices;
    uint   subdevices_count;
    void  *handler; // Internal use only
    char   name[128];
    char   uuid[128];
    ullong serial;
    uint   cores_count;
    ullong core_freq; // KHz
    ullong core_freq_boost; // KHz
    uint   mem_total; // MiB's
    char   __reserved[256];
} gpu_devs_t; // 584 bytes

typedef struct gpu_topology_s {
    gpu_devs_t *devs;
    uint devs_count;
    char __reserved[128]; // For future widening
} gpu_topology_t; // 144 bytes

#define GPU_TP_SEL_MAIN     1 // Do not include sub-devices
#define GPU_TP_SEL_READABLE 2 // Include only readable devices

typedef struct gpu_s {
    timestamp_t time;
    ulong samples;
    ulong freq_gpu; // khz
    ulong freq_mem; // khz
    ulong util_gpu; // percent
    ulong util_mem; // percent
    ulong temp_gpu; // celsius
    ulong temp_mem; // celsius
    double energy_j;
    double power_w;
    uint working;
    uint correct;
} gpu_t;

typedef struct gpu_ops_s {
    void    (*unload)       ();
    state_t (*update)       (uint option, void *value);
    void    (*get_info)     (apinfo_t *info);
    void    (*topology_get) (gpu_topology_t *tp);
    void    (*set_monitoring_mode) (int mode);
    state_t (*read)         (gpu_t *data);
    double  (*read_raw)     (gpu_t *data);
    void    (*data_diff)    (gpu_t *data2, gpu_t *data1, gpu_t *data_diff);
} gpu_ops_t;

// API building scheme
#define GPU_F_LOAD(name)         void gpu_##name##_load(gpu_ops_t *ops, int options)
#define GPU_F_UNLOAD(name)       void gpu_##name##_unload()
#define GPU_F_UPDATE(name)       state_t gpu_##name##_update(uint option, void *value)
#define GPU_F_GET_INFO(name)     void gpu_##name##_get_info(apinfo_t *info)
#define GPU_F_TOPOLOGY_GET(name) void gpu_##name##_topology_get(gpu_topology_t *tp)
#define GPU_F_SET_MONITORING_MODE(name) void gpu_##name##_set_monitoring_mode(int mode)
#define GPU_F_READ(name)         state_t gpu_##name##_read(gpu_t *d)
#define GPU_F_READ_RAW(name)     state_t gpu_##name##_read_raw(gpu_t *d)
#define GPU_F_DATA_DIFF(name)    void gpu_##name##_data_diff(gpu_t *d2, gpu_t *d1, gpu_t *dD)

#define GPU_DEFINES(name)  \
    GPU_F_LOAD(name);      \
    GPU_F_UNLOAD(name);    \
    GPU_F_UPDATE(name);    \
    GPU_F_GET_INFO(name);  \
    GPU_F_TOPOLOGY_GET(name); \
    GPU_F_SET_MONITORING_MODE(name); \
    GPU_F_READ(name);      \
    GPU_F_READ_RAW(name);  \
    GPU_F_DATA_DIFF(name);

// Discovers the low level API.
void gpu_load(int options);

void gpu_unload();

state_t gpu_update(uint option, void *value);

// Returns all available static information
void gpu_get_info(apinfo_t *info);

void gpu_topology_get(gpu_topology_t *tp);

void gpu_topology_free(gpu_topology_t *tp);

void gpu_topology_print(gpu_topology_t *tp, int fd);

void gpu_topology_select(gpu_topology_t *tp, gpu_topology_t *tp_new, uint type);

// Information about devices that return metrics. They can be devices and sub-
// devices (like MIG Slices). Freeing devs allocation is your responsibility.
void gpu_get_devices(gpu_devs_t **devs, uint *devs_count);

// You can use it to increase the monitoring rate.
void gpu_set_monitoring_mode(int mode);

// Reads the GPU device data and stores it in the gpu_t array data (1 per device).
state_t gpu_read(gpu_t *data);

// Reads the GPU device data directly from the hardware (not pooled).
state_t gpu_read_raw(gpu_t *data);

// Performs a gpu_read() over data2, a gpu_data_diff() and copies data2 in data1.
state_t gpu_read_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

// Performs a gpu_read() over data2, a gpu_data_diff() and copies data2 in data1.
state_t gpu_read_copy(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

/** Helpers */
// Substracts the elements of the gpu_t array (data_diff = data2 - data1).
void gpu_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

// Makes an average of all the elements of the data_diff array.
void gpu_data_merge(gpu_t *data_diff, gpu_t *data_merge);

// Allocates an array of gpu_t (1 per device).
void gpu_data_alloc(gpu_t **data);

// Frees an array of gpu_t.
void gpu_data_free(gpu_t **data);

// Sets to 0 an array of gpu_t.
void gpu_data_null(gpu_t *data);

// Copies an array of gpu_t.
void gpu_data_copy(gpu_t *data_dst, gpu_t *data_src);

// Prints an array of gpu_t in the channel fd.
void gpu_data_print(gpu_t *data, int fd);

// Copies the printing string in a char buffer.
char *gpu_data_tostr(gpu_t *data, char *buffer, int length);

int gpu_is_supported();

/* clang-format on */
#endif