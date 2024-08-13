/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_PRIORITY_DUMMY_H
#define MANAGEMENT_PRIORITY_DUMMY_H

#include <management/cpufreq/priority.h>

void mgt_prio_dummy_load(topology_t *tp, mgt_prio_ops_t *ops);

void mgt_prio_dummy_get_api(uint *api);

state_t mgt_prio_dummy_init();

state_t mgt_prio_dummy_dispose();

state_t mgt_prio_dummy_enable();

state_t mgt_prio_dummy_disable();

int mgt_prio_dummy_is_enabled();

state_t mgt_prio_dummy_get_available_list(cpuprio_t *prio_list);

state_t mgt_prio_dummy_set_available_list(cpuprio_t *prio_list);

state_t mgt_prio_dummy_get_current_list(uint *idx_list);

state_t mgt_prio_dummy_set_current_list(uint *idx_list);

state_t mgt_prio_dummy_set_current(uint idx, int cpu);

void mgt_prio_dummy_data_count(uint *prios_count, uint *idxs_count);

#endif //MANAGEMENT_PRIORITY_DUMMY_H
