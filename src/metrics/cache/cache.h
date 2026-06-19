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
/* clang-format off */

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

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
    pid_t          pid;
    cache_level_t  l1d;
    cache_level_t  l2;
    cache_level_t  l3;
    cache_level_t *ll;
    cache_level_t *lbw; // Level used to calc RAM bandwidth
    double         bw_gbs; // RAM bandwidth in GB/s
    double         bw_ratio; // Fraction of bandwidth occupied by this dev from the total
    timestamp_t    time;
} cache_t;

typedef struct coche_ops_s {
    void    (*unload)          ();
    state_t (*update)          (uint option, void *value);
    void    (*get_info)        (apinfo_t *info);
    state_t (*read)            (cache_t *ca);
    void    (*data_diff)       (cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);
    void    (*internals_tostr) (char *buffer, int length);
} cache_ops_t;

// API building scheme
#define CACHE_F_LOAD(name)      void cache_##name##_load(topology_t *tp, cache_ops_t *ops, int options)
#define CACHE_F_UNLOAD(name)    void cache_##name##_unload()
#define CACHE_F_UPDATE(name)    state_t cache_##name##_update(uint option, void *value)
#define CACHE_F_GET_INFO(name)  void cache_##name##_get_info(apinfo_t *info)
#define CACHE_F_READ(name)      state_t cache_##name##_read(cache_t *ca)
#define CACHE_F_DATA_DIFF(name) void cache_##name##_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
#define CACHE_F_INTERNALS(name) void cache_##name##_internals_tostr(char *buffer, int length)

#define CACHE_DEFINES(name)                                       \
    CACHE_F_LOAD(name);                                           \
    CACHE_F_UNLOAD(name);                                         \
    CACHE_F_UPDATE(name);                                         \
    CACHE_F_GET_INFO(name);                                       \
    CACHE_F_READ(name);                                           \
    CACHE_F_DATA_DIFF(name);                                      \
    CACHE_F_INTERNALS(name)

void cache_load(topology_t *tp, int options);

void cache_unload();

state_t cache_update(uint option, void *value);

void cache_get_info(apinfo_t *info);

state_t cache_read(cache_t *ca);

state_t cache_read_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

state_t cache_read_copy(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

void cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

void cache_data_alloc(cache_t **ca);

void cache_data_free(cache_t **ca);

void cache_data_copy(cache_t *dst, cache_t *src);

void cache_data_print(cache_t *caD, double gbs, int fd);

char *cache_data_tostr(cache_t *caD, double gbs, char *buffer, size_t length);

void cache_internals_print(int fd);

void cache_internals_tostr(char *buffer, int length);

/* clang-format on */
#endif // METRICS_CACHE_H