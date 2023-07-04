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
//  | ARM v8.2a       | ??? | v     | Process     | Too many variances         |
//  ---------------------------------------------------------------------------|
// Props:
//  - Thread safe: yes
//  - Requires root: no

typedef struct cache_s {
    ullong lbw_misses;
    ullong l1d_misses;
    ullong l2_misses;
    ullong l3_misses;
    ullong ll_misses;
    timestamp_t time;
} cache_t;

typedef struct coche_ops_s {
    void    (*get_info)        (apinfo_t *info);
    state_t (*init)            ();
	state_t (*dispose)         ();
	state_t (*read)            (cache_t *ca);
    void    (*data_diff)       (cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);
    void    (*details_tostr)      (char *buffer, int length);
} cache_ops_t;

void cache_load(topology_t *tp, int eard);

void cache_get_info(apinfo_t *info);

state_t cache_init(ctx_t *c);

state_t cache_dispose(ctx_t *c);

state_t cache_read(ctx_t *c, cache_t *ca);

state_t cache_read_diff(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

state_t cache_read_copy(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

void cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

void cache_data_copy(cache_t *dst, cache_t *src);

void cache_data_print(cache_t *caD, double gbs, int fd);

void cache_data_tostr(cache_t *caD, double gbs, char *buffer, size_t length);

void cache_details_print(int fd);

void cache_details_tostr(char *buffer, int length);

#endif //METRICS_CACHE_H