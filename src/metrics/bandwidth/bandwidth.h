/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_BANDWIDTH_H
#define METRICS_BANDWIDTH_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/types/generic.h>
#include <metrics/common/apis.h>

// The last device is used as a timer for computing the GB/s
typedef struct bwidth_s {
    union {
        timestamp_t time;
        double secs;
        ullong cas;
    };
} bwidth_t;

typedef struct bwidth_ops_s {
    void (*unload)();
    void (*get_info)(apinfo_t *info);
    state_t (*read)(bwidth_t *b);
    double (*castob)(double cas);
} bwidth_ops_t;

// API building scheme
#define BWIDTH_F_LOAD(name)     void bwidth_##name##_load(topology_t *tp, bwidth_ops_t *ops, int options)
#define BWIDTH_F_UNLOAD(name)   void bwidth_##name##_unload()
#define BWIDTH_F_GET_INFO(name) void bwidth_##name##_get_info(apinfo_t *info)
#define BWIDTH_F_READ(name)     state_t bwidth_##name##_read(bwidth_t *b)
#define BWIDTH_F_CASTOB(name)   double bwidth_##name##_castob(double cas)

#define BWIDTH_DEFINES(name)                                                                                           \
    BWIDTH_F_LOAD(name);                                                                                               \
    BWIDTH_F_UNLOAD(name);                                                                                             \
    BWIDTH_F_GET_INFO(name);                                                                                           \
    BWIDTH_F_READ(name);                                                                                               \
    BWIDTH_F_CASTOB(name);

void bwidth_load(topology_t *tp, int options);

void bwidth_unload();

void bwidth_get_info(apinfo_t *api);

state_t bwidth_read(bwidth_t *b);

/* CAS and GBS are just one value. */
state_t bwidth_read_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

state_t bwidth_read_copy(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

/* Returns the total node CAS and total node GBs. */
void bwidth_data_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

/* Accumulates bandwidth differences and return its data in CAS and/or GB/s (accepts NULL). */
void bwidth_data_accum(bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs);

void bwidth_data_alloc(bwidth_t **b);

void bwidth_data_free(bwidth_t **b);

void bwidth_data_null(bwidth_t *bws);

void bwidth_data_copy(bwidth_t *dst, bwidth_t *src);

void bwidth_data_print(ullong cas, double gbs, int fd);

char *bwidth_data_tostr(ullong cas, double gbs, char *buffer, size_t length);

// Helpers
/* Converts CAS to GBS given a time in seconds. */
double bwidth_help_castogbs(ullong cas, double secs);
/* Converts CAS to TPI given a number of instructions. */
double bwidth_help_castotpi(ullong cas, ullong instructions);

#endif
