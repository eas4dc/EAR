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

// The last device is used as a timer for computing the GB/s
typedef struct bwidth_s {
	union {
		timestamp_t time;
        double secs;
		ullong cas;
	};
} bwidth_t;

// API building scheme
#define BWIDTH_F_LOAD(name)      void name (topology_t *tpo, bwidth_ops_t *ops)
#define BWIDTH_F_GET_INFO(name)  void name (apinfo_t *info)
#define BWIDTH_F_INIT(name)      state_t name (ctx_t *c)
#define BWIDTH_F_DISPOSE(name)   state_t name (ctx_t *c)
#define BWIDTH_F_READ(name)      state_t name (ctx_t *c, bwidth_t *bws)

#define BWIDTH_DEFINES(name) \
BWIDTH_F_LOAD     (bwidth_ ##name ##_load); \
BWIDTH_F_GET_INFO (bwidth_ ##name ##_get_info); \
BWIDTH_F_INIT     (bwidth_ ##name ##_init); \
BWIDTH_F_DISPOSE  (bwidth_ ##name ##_dispose); \
BWIDTH_F_READ     (bwidth_ ##name ##_read);

typedef struct bwidth_ops_s {
    state_t (*count_devices)   (ctx_t *c, uint *dev_count); // Obsolete
    BWIDTH_F_GET_INFO((*get_info));
    BWIDTH_F_INIT    ((*init));
    BWIDTH_F_DISPOSE ((*dispose));
    BWIDTH_F_READ    ((*read));
} bwidth_ops_t;

void bwidth_load(topology_t *tp, int eard);

void bwidth_get_info(apinfo_t *api);
// Obsolete (use bwidth_get_info)
void bwidth_get_api(uint *api);
// Obsolete (use bwidth_get_info)
void bwidth_count_devices(ctx_t *c, uint *dev_count);

state_t bwidth_init(ctx_t *c);

state_t bwidth_dispose(ctx_t *c);

state_t bwidth_read(ctx_t *c, bwidth_t *b);

/* CAS and GBS are just one value. */
state_t bwidth_read_diff(ctx_t *c, bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

state_t bwidth_read_copy(ctx_t *c, bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

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
