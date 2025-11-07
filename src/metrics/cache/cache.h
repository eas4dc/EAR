/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_CACHE_H
#define METRICS_CACHE_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/system/time.h>

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

typedef struct cache_level_s {
    ullong accesses;
    ullong misses;
    ullong hits;
    ullong lines_in;
    ullong lines_out;
    double miss_rate;
    double hit_rate;
} cache_level_t;

typedef struct cache_s {
    cache_level_t l1d;
    cache_level_t l2;
    cache_level_t l3;
    cache_level_t *ll;
    cache_level_t *lbw; // Level used to calc bandwidth
    timestamp_t time;
} cache_t;

typedef struct coche_ops_s {
    void (*get_info)(apinfo_t *info);
    state_t (*init)();
    state_t (*dispose)();
    state_t (*read)(cache_t *ca);
    void (*data_diff)(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);
    void (*internals_tostr)(char *buffer, int length);
} cache_ops_t;

void cache_load(topology_t *tp, int force_api);

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

void cache_internals_print(int fd);

void cache_internals_tostr(char *buffer, int length);

#endif // METRICS_CACHE_H