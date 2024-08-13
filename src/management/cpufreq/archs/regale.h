/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_CPUFREQ_ARCHS_REGALE_H
#define MANAGEMENT_CPUFREQ_ARCHS_REGALE_H

#include <management/cpufreq/cpufreq.h>

state_t mgt_cpufreq_regale_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver, int regale);

state_t mgt_cpufreq_regale_init(ctx_t *c);

state_t mgt_cpufreq_regale_dispose(ctx_t *c);

state_t mgt_cpufreq_regale_count_available(ctx_t *c, uint *pstate_count);

state_t mgt_cpufreq_regale_get_available_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_regale_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_regale_get_nominal(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_regale_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

state_t mgt_cpufreq_regale_set_current_list(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_regale_set_current(ctx_t *c, uint pstate_index, int cpu);

// Governors
state_t mgt_cpufreq_regale_governor_get(ctx_t *c, uint *governor);

state_t mgt_cpufreq_regale_governor_get_list(ctx_t *c, uint *governors);

state_t mgt_cpufreq_regale_governor_set(ctx_t *c, uint governor);

state_t mgt_cpufreq_regale_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask);

state_t mgt_cpufreq_regale_governor_set_list(ctx_t *c, uint *governors);

#endif //MANAGEMENT_CPUFREQ_ARCHS_REGALE_H
