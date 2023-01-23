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
