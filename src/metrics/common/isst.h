/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_ISST_H
#define METRICS_COMMON_ISST_H

#include <common/hardware/topology.h>
#include <common/states.h>
#include <common/types.h>

#define ISST_MAX_FREQ  25500000
#define ISST_SAME_PRIO UINT_MAX

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
