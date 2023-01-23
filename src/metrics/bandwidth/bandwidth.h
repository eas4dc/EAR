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

#ifndef METRICS_BANDWIDTH_H
#define METRICS_BANDWIDTH_H

#include <common/sizes.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/types/generic.h>
#include <common/hardware/topology.h>
#include <metrics/common/apis.h>

// The API
//
// This API is designed to get the bandwidth from CPU caches to main memory.
//
// Props:
// 	- Thread safe: yes.
//	- Daemon API: yes.
//  - Dummy API: yes.
//  - Requires root: yes.
//
// Compatibility:
//  -------------------------------------------------------------------------
//  | Architecture    | F/M | Comp. | Granularity | System                  |
//  -------------------------------------------------------------------------
//  | Intel HASWELL   | 63  | v     | Node        | PCI                     |
//  | Intel BROADWELL | 79  | v     | Node        | PCI                     |
//  | Intel SKYLAKE   | 85  | v     | Node        | PCI                     |
//  | Intel ICELAKE   | 106 | x     | Node        | MMIO (test required)    |
//  | AMD ZEN+/2      | 17h | v     | Node        | L3 misses (future HSMP) |
//  | AMD ZEN3        | 19h | v     | Node        | L3 misses (future HSMP) |
//  | Likwid          | -   | v     | Node        | Used for ICELAKE        |
//  | Cache bypass    | -   | v     | Cache API   | Normally is procf       |
//  -------------------------------------------------------------------------

typedef struct bwidth_s {
	union {
		timestamp_t time;
        double secs;
		ullong cas;
	};
} bwidth_t;

typedef struct bwidth_ops_s {
	state_t (*init)            (ctx_t *c);
	state_t (*dispose)         (ctx_t *c);
	state_t (*count_devices)   (ctx_t *c, uint *dev_count);
	state_t (*get_granularity) (ctx_t *c, uint *granularity);
	state_t (*read)            (ctx_t *c, bwidth_t *b);
    state_t (*data_diff)       (bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);
    state_t (*data_accum)      (bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs);
	state_t (*data_alloc)      (bwidth_t **b);
	state_t (*data_free)       (bwidth_t **b);
	state_t (*data_copy)       (bwidth_t *dst, bwidth_t *src);
	state_t (*data_print)      (ullong cas, double gbs, int fd);
	state_t (*data_tostr)      (ullong cas, double gbs, char *buffer, size_t length);
} bwidth_ops_t;

state_t bwidth_load(topology_t *tp, int eard);

state_t bwidth_get_api(uint *api);

//
state_t bwidth_init(ctx_t *c);

state_t bwidth_dispose(ctx_t *c);

state_t bwidth_count_devices(ctx_t *c, uint *dev_count);

state_t bwidth_get_granularity(ctx_t *c, uint *granularity);

state_t bwidth_read(ctx_t *c, bwidth_t *b);

/* CAS and GBS are just one value. */
state_t bwidth_read_diff(ctx_t *c, bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

state_t bwidth_read_copy(ctx_t *c, bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

/* Returns the total node CAS and total node GBs. */
state_t bwidth_data_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

/* Accumulates bandwidth differences and return its data in CAS and/or GB/s (accepts NULL). */
state_t bwidth_data_accum(bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs);

state_t bwidth_data_alloc(bwidth_t **b);

state_t bwidth_data_free(bwidth_t **b);

state_t bwidth_data_copy(bwidth_t *dst, bwidth_t *src);

state_t bwidth_data_print(ullong cas, double gbs, int fd);

state_t bwidth_data_tostr(ullong cas, double gbs, char *buffer, size_t length);

// Helpers

/* Converts CAS to GBS given a time in seconds. */
double bwidth_help_castogbs(ullong cas, double secs);
/* Converts CAS to TPI given a number of instructions. */
double bwidth_help_castotpi(ullong cas, ullong instructions);

#endif
