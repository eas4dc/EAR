/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_H
#define METRICS_H
/* clang-format off */

#include <metrics/io/io.h>
#include <metrics/cpi/cpi.h>
#include <metrics/gpu/gpu.h>
#include <metrics/proc/proc.h>
#include <metrics/cache/cache.h>
#include <metrics/flops/flops.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/energy/energy_node.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/energy_cpu/energy_cpu.h>
#include <metrics/temperature/temperature.h>

typedef struct metrics_info_s {
    apinfo_t cpufreq; // cpu frequency
    apinfo_t imcfreq; // imc/uncore frequency
    apinfo_t bwidth;  // ram bandwidth
    apinfo_t cache;   // cache hits, misses and accesses
    apinfo_t flops;   // floating point operations
    apinfo_t temp;    // temperature
    apinfo_t cpupow;  // cpu energy/power consumption
    apinfo_t cpi;     // cycles per instructions
    apinfo_t gpu;     // gpu
    apinfo_t nodepow; // node energ/power consumption
    apinfo_t net;     // network traffic
    apinfo_t io ;     // disk I/O
    apinfo_t proc;    // process stats
} metrics_info_t;

typedef struct metrics_read_s {
    ullong      samples;
    timestamp_t time;
    cpufreq_t  *cpufreq;
    imcfreq_t  *imcfreq;
    bwidth_t   *bwidth;
    cache_t    *cache;
    flops_t    *flops;
    llong      *temp;
    ullong     *cpupow;
    cpi_t      *cpi;
    gpu_t      *gpu;
    char       *nodepow;
    io_t       *io;
    proc_t     *proc;
} metrics_read_t;

typedef struct metrics_diff_s {
    ullong   samples;
    double   time;          // Seconds
    ulong   *cpufreq_diff;  // KHz (per device)
    ulong    cpufreq_avrg;  // KHz
    ulong   *imcfreq_diff;  // KHz (per device)
    ulong    imcfreq_avrg;  // KHz
    ullong   bwidth_diff;   // CAS
    double   bwidth_avrg;   // GB/s
    cache_t *cache_diff;    // Misses
    double   cache_avrg;    // GB/s
    flops_t *flops_diff;    // Float instructions
    double   flops_avrg;    // Giga float operations per second
    llong   *temp_diff;     // Celsius
    llong    temp_avrg;     // Celsius
    ullong  *cpupow_diff;   // DRAM and PACKAGE power per device
    ullong  *cpupow_dram;   // DRAM power per device
    ullong  *cpupow_pack;   // PACKAGE power per device
    ullong  cpupow_tot_dram;
    ullong  cpupow_tot_pack;
    cpi_t   *cpi_diff;
    double   cpi_avrg;      // CPI
    gpu_t   *gpu_diff;      // GPU
    ulong    nodepow_avrg;  // Total node energy
    io_t    *io_diff;       // MB/s
    double   io_avrg;       // MB/s
    proc_t  *proc_diff;
} metrics_diff_t;

#define MET_OPT_CPUFREQ 0
#define MET_OPT_IMCFREQ 1
#define MET_OPT_BWIDTH  2
#define MET_OPT_FLOPS   3
#define MET_OPT_CACHE   4
#define MET_OPT_TEMP    5
#define MET_OPT_CPI     6
#define MET_OPT_GPU     7
#define MET_OPT_CPUPOW  8
#define MET_OPT_NODEPOW 9
#define MET_OPT_IO      10
#define MET_OPT_PROC    11
#define MET_OPT_MAX     12

// This function contains load() and init() functions of the different metrics.
void metrics_load(metrics_info_t *m, topology_t *tp, char *nodepow_path, uint *options);

// You can see the list of update options in apis.h (UPD_ prefix). The value can
// be of whatever type, a pointer (including NULL), an integer, etc.
void metrics_update(uint option, void *value);

// This function is composed by get_info() functions of the different metrics.
void metrics_info_get(metrics_info_t *m);

char *metrics_info_tostr(metrics_info_t *m, char *buffer);

void metrics_info_print(metrics_info_t *m, int fd);

void metrics_read(metrics_read_t *mr);

void metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_data_alloc(metrics_read_t *mr1, metrics_read_t *mr2, metrics_diff_t *mrD);

void metrics_data_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_data_copy(metrics_read_t *mrD, metrics_read_t *mrS);

void metrics_data_print(metrics_diff_t *mrD, int fd);

char *metrics_data_tostr(metrics_diff_t *mrD);

// Helper function to convert an environment variable to array of options.
uint *metrics_envtoops(char *var_name, uint options_expected);

/* clang-format on */
#endif // METRICS_H
