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

#ifndef METRICS_CACHE_H
#define METRICS_CACHE_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

// Compatibility:
//  ----------------------------------------------------------------------------
//  | Architecture    | F/M | Comp. | Granularity | Comments                   |
//  ----------------------------------------------------------------------------
//  | Intel HASWELL   | 63  | v     | Process     | L1 and L2 no writes        |
//  | Intel BROADWELL | 79  | v     | Process     | L1 and L2 no writes        |
//  | Intel SKYLAKE   | 85  | v     | Process     | L1 and L2 no writes        |
//  | AMD ZEN+/2      | 17h | v     | Process     | L1 no writes, no L2        |
//  | AMD ZEN3        | 19h | v     | Process     | L1 no writes, no L2        |
//  ---------------------------------------------------------------------------|
// Props:
//  - Thread safe: yes
//  - Requires root: no

typedef struct cache_s {
	timestamp_t time;
	ullong l1d_misses;
	ullong l2_misses;
    ullong l3r_misses;
    ullong l3w_misses;
    ullong l3_misses;
} cache_t;

typedef struct coche_ops_s {
	state_t (*init)            (ctx_t *c);
	state_t (*dispose)         (ctx_t *c);
	state_t (*read)            (ctx_t *c, cache_t *ca);
	state_t (*data_diff)       (cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);
	state_t (*data_copy)       (cache_t *dst, cache_t *src);
	state_t (*data_print)      (cache_t *ca, double gbs, int fd);
	state_t (*data_tostr)      (cache_t *ca, double gbs, char *buffer, size_t length);
} cache_ops_t;

state_t cache_load(topology_t *tp, int eard);

state_t cache_get_api(uint *api);

state_t cache_init(ctx_t *c);

state_t cache_dispose(ctx_t *c);

state_t cache_read(ctx_t *c, cache_t *ca);

// Helpers
state_t cache_read_diff(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

state_t cache_read_copy(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

state_t cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

state_t cache_data_copy(cache_t *dst, cache_t *src);

state_t cache_data_print(cache_t *caD, double gbs, int fd);

state_t cache_data_tostr(cache_t *caD, double gbs, char *buffer, size_t length);

#endif //METRICS_CACHE_H
