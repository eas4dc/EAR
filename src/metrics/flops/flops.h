/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_FLOPS_H
#define METRICS_FLOPS_H
// clang-format off

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

// Old style, deprecated and extinguish as soon as possible
#define FLOPS_EVENTS 8
#define INDEX_256F   2
#define INDEX_256D   6
#define INDEX_512F   3
#define INDEX_512D   7
#ifdef __ARCH_ARM
#define WEIGHT_256F 1
#define WEIGHT_256D 1
#define WEIGHT_512F 1
#define WEIGHT_512D 1
#else
#define WEIGHT_256F 8
#define WEIGHT_256D 4
#define WEIGHT_512F 16
#define WEIGHT_512D 8
#endif

typedef struct flops_s {
    pid_t  pid;
    ullong f64;
    ullong d64;
    ullong f128;
    ullong d128;
    ullong f256;
    ullong d256;
    ullong f512;   // 512 field is also used as MAX VECTOR LENGTH flops,
    ullong d512;   // i.e. the ARM's SVE. But in the future maybe union could be.
    double gflops; //

    union {
        timestamp_t time;
        double secs;
    };
} flops_t;

typedef struct flops_ops_s {
    void    (*unload)    ();
    state_t (*update)    (uint option, void *value);
    void    (*get_info)  (apinfo_t *info);
    state_t (*read)      (flops_t *fl);
    void    (*data_diff) (flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);
    void    (*internals_tostr) (char *buffer, int length);
} flops_ops_t;

// API building scheme
#define FLOPS_F_LOAD(name)       void flops_##name##_load(topology_t *tp, flops_ops_t *ops, int options)
#define FLOPS_F_UNLOAD(name)     void flops_##name##_unload()
#define FLOPS_F_UPDATE(name)     state_t flops_##name##_update(uint option, void *value)
#define FLOPS_F_GET_INFO(name)   void flops_##name##_get_info(apinfo_t *info)
#define FLOPS_F_READ(name)       state_t flops_##name##_read(flops_t *fl)
#define FLOPS_F_DATA_DIFF(name)  void flops_##name##_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
#define FLOPS_F_INTERNALS_TOSTR(name) void flops_##name##_internals_tostr(char *buffer, int length)

#define FLOPS_DEFINES(name)  \
    FLOPS_F_LOAD(name);      \
    FLOPS_F_UNLOAD(name);    \
    FLOPS_F_UPDATE(name);    \
    FLOPS_F_GET_INFO(name);  \
    FLOPS_F_READ(name);      \
    FLOPS_F_DATA_DIFF(name); \
    FLOPS_F_INTERNALS_TOSTR(name);

void flops_load(topology_t *tp, int options);

void flops_unload();

state_t flops_update(uint option, void *value);

void flops_get_info(apinfo_t *info);

state_t flops_read(flops_t *fl);

state_t flops_read_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gflops_tot);

state_t flops_read_copy(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gflops_tot);

/* It returns the difference (flD) and the total gflops of all devices. */
void flops_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gflops_tot);

void flops_data_alloc(flops_t **fl);

void flops_data_free(flops_t **fl);

void flops_data_copy(flops_t *dst, flops_t *src);

void flops_data_print(flops_t *flD, double gfs, int fd);

char *flops_data_tostr(flops_t *flD, double gfs, char *buffer, size_t length);

/* Accumulates flops differences and return its data in GFLOPs (accepts NULL). */
void flops_data_accum(flops_t *fA, flops_t *fD, double *gflops_tot);

void flops_internals_print(int fd);

void flops_internals_tostr(char *buffer, int length);

/* Converts a flops difference into old FLOPs array system. */
ullong *flops_help_toold(flops_t *flD, ullong *flops);

// clang-format on
#endif
