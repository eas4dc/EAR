/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_CPUFREQ_ARCHS_DUMMY_H
#define MANAGEMENT_CPUFREQ_ARCHS_DUMMY_H

#include <management/cpufreq/cpufreq.h>

state_t mgt_cpufreq_dummy_load(topology_t *tp_in, mgt_ps_ops_t *ops);

state_t mgt_cpufreq_dummy_init(ctx_t *c);

state_t mgt_cpufreq_dummy_dispose(ctx_t *c);

void mgt_cpufreq_dummy_get_info(apinfo_t *info);

void mgt_cpufreq_dummy_get_freq_details(freq_details_t *details);

state_t mgt_cpufreq_dummy_count_available(ctx_t *c, uint *pstate_count);

state_t mgt_cpufreq_dummy_get_available_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_dummy_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_dummy_get_nominal(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_dummy_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

state_t mgt_cpufreq_dummy_set_current_list(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_dummy_set_current(ctx_t *c, uint pstate_index, int cpu);

state_t mgt_cpufreq_dummy_reset(ctx_t *c);

// Governors
state_t mgt_cpufreq_dummy_governor_get(ctx_t *c, uint *governor);

state_t mgt_cpufreq_dummy_governor_get_list(ctx_t *c, uint *governors);

state_t mgt_cpufreq_dummy_governor_set(ctx_t *c, uint governor);

state_t mgt_cpufreq_dummy_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask);

state_t mgt_cpufreq_dummy_governor_set_list(ctx_t *c, uint *governors);

#endif //MANAGEMENT_CPUFREQ_ARCHS_DUMMY_H