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

#ifndef METRICS_COMMON_ISST_H
#define METRICS_COMMON_ISST_H

#include <common/types.h>
#include <common/states.h>
#include <common/hardware/topology.h>

#define ISST_MAX_FREQ    25500000
#define ISST_SAME_PRIO   UINT_MAX

typedef struct clos_s {
    ullong max_khz;
    ullong min_khz;
    uint high;
    uint low;
    uint idx;
} clos_t;

state_t isst_init(topology_t *tp);
// Resets the SST registers to the original values and frees the data
state_t isst_dispose();

state_t isst_enable();

state_t isst_disable();

int isst_is_enabled();
// Resets the SST registers to the original values
state_t isst_reset();
/* CLOS */
state_t isst_clos_count(uint *count);

state_t isst_clos_alloc_list(clos_t **clos_list);

state_t isst_clos_get_list(clos_t *clos_list);

state_t isst_clos_set_list(clos_t *clos_list);
/* Assoc */
state_t isst_assoc_count(uint *count);

state_t isst_assoc_alloc_list(uint **idx_list);

state_t isst_assoc_get_list(uint *idx_list);

state_t isst_assoc_set_list(uint *idx_list);

state_t isst_assoc_set(uint idx, int cpu);

#endif
