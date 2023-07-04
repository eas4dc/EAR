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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef METRICS_FLOPS_H
#define METRICS_FLOPS_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

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
#define WEIGHT_256F  1
#define WEIGHT_256D  1
#define WEIGHT_512F  1
#define WEIGHT_512D  1
#else
#define WEIGHT_256F  8
#define WEIGHT_256D  4
#define WEIGHT_512F 16
#define WEIGHT_512D  8
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
    union {
        timestamp_t time;
        double secs;
    };
} flops_t;

typedef struct flops_ops_s {
    void    (*get_api)         (uint *api);
    void    (*get_granularity) (uint *granularity);
    void    (*get_weights)     (ullong **weights);
	state_t (*init)            (ctx_t *c);
	state_t (*dispose)         (ctx_t *c);
	state_t (*read)            (ctx_t *c, flops_t *fl);
} flops_ops_t;

void flops_load(topology_t *tp, int eard);

void flops_get_api(uint *api);

void flops_get_granularity(uint *granularity);

state_t flops_init(ctx_t *c);

state_t flops_dispose(ctx_t *c);

state_t flops_read(ctx_t *c, flops_t *fl);

state_t flops_read_diff(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

state_t flops_read_copy(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

void flops_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

/* Accumulates flops differences and return its data in GFLOPs (accepts NULL). */
void flops_data_accum(flops_t *fA, flops_t *fD, double *gfs);

void flops_data_copy(flops_t *dst, flops_t *src);

void flops_data_print(flops_t *flD, double gfs, int fd);

void flops_data_tostr(flops_t *flD, double gfs, char *buffer, size_t length);

// Helpers
/* Converts a flops difference into old FLOPs array system. */
ullong *flops_help_toold(flops_t *flD, ullong *flops);

#endif
