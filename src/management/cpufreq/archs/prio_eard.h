/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_PRIORITY_EARD_H
#define MANAGEMENT_PRIORITY_EARD_H

#include <management/cpufreq/priority.h>

void mgt_prio_eard_load(topology_t *tp, mgt_prio_ops_t *ops, int eard);

void mgt_prio_eard_get_api(uint *api);

state_t mgt_prio_eard_init();

state_t mgt_prio_eard_dispose();

state_t mgt_prio_eard_enable();

state_t mgt_prio_eard_disable();

int mgt_prio_eard_is_enabled();

state_t mgt_prio_eard_get_available_list(cpuprio_t *prio_list);

state_t mgt_prio_eard_set_available_list(cpuprio_t *prio_list);

state_t mgt_prio_eard_get_current_list(uint *idx_list);

state_t mgt_prio_eard_set_current_list(uint *idx_list);

state_t mgt_prio_eard_set_current(uint idx, int cpu);

void mgt_prio_eard_data_count(uint *prios_count, uint *idxs_count);

#endif // MANAGEMENT_PRIORITY_EARD_H
