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

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/system/time.h>

// The API
//
// This API is designed to get the floating point operations per second of the system.
//
// Props:
//  - Thread safe: yes.
//  - Requires root: no.
//	- Daemon bypass: no.
//  - Dummy API: yes.
//
// Compatibility:
//  ----------------------------------------------------------------------------
//  | Architecture    | F/M | Comp. | Granularity | Comments                   |
//  ----------------------------------------------------------------------------
//  | Intel HASWELL   | 63  | v     | Process     |                            |
//  | Intel BROADWELL | 79  | v     | Process     |                            |
//  | Intel SKYLAKE   | 85  | v     | Process     |                            |
//  | Intel ICELAKE   | 106 | v     | Process     | It is supposed             |
//  | AMD ZEN+/2      | 17h | v     | Process     | Counts 256 flops.          |
//  | AMD ZEN3        | 19h | ?     | Process     | Test required.             |
//  ---------------------------------------------------------------------------|
//

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
    ullong f64;
    ullong d64;
    ullong f128;
    ullong d128;
    ullong f256;
    ullong d256;
    ullong f512; // 512 field is also used as MAX VECTOR LENGTH flops,
    ullong d512; // i.e. the ARM's SVE. But in the future maybe union could be.
    double gflops;

    union {
        timestamp_t time;
        double secs;
    };
} flops_t;

typedef struct flops_ops_s {
    void (*get_info)(apinfo_t *info);
    state_t (*init)();
    state_t (*dispose)();
    state_t (*read)(flops_t *fl);
    void (*data_diff)(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);
    void (*internals_tostr)(char *buffer, int length);
} flops_ops_t;

void flops_load(topology_t *tp, int force_api);

void flops_get_info(apinfo_t *info);

state_t flops_init(ctx_t *c);

state_t flops_dispose(ctx_t *c);

state_t flops_read(ctx_t *c, flops_t *fl);

state_t flops_read_diff(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

state_t flops_read_copy(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

void flops_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

void flops_data_copy(flops_t *dst, flops_t *src);

void flops_data_print(flops_t *flD, double gfs, int fd);

void flops_data_tostr(flops_t *flD, double gfs, char *buffer, size_t length);

/* Accumulates flops differences and return its data in GFLOPs (accepts NULL). */
void flops_data_accum(flops_t *fA, flops_t *fD, double *gfs);

void flops_internals_print(int fd);

void flops_internals_tostr(char *buffer, int length);

// Helpers
/* Converts a flops difference into old FLOPs array system. */
ullong *flops_help_toold(flops_t *flD, ullong *flops);

#endif
