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

#ifndef MANAGEMENT_CPUFREQ_ARCHS_EARD_H
#define MANAGEMENT_CPUFREQ_ARCHS_EARD_H

#include <management/cpufreq/cpufreq.h>

state_t mgt_cpufreq_eard_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver, int eard);

state_t mgt_cpufreq_eard_init(ctx_t *c);

state_t mgt_cpufreq_eard_dispose(ctx_t *c);

state_t mgt_cpufreq_eard_count_available(ctx_t *c, uint *pstate_count);

state_t mgt_cpufreq_eard_get_available_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_eard_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_eard_get_nominal(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_eard_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

state_t mgt_cpufreq_eard_set_current_list(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_eard_set_current(ctx_t *c, uint pstate_index, int cpu);

// Governors
state_t mgt_cpufreq_eard_governor_get(ctx_t *c, uint *governor);

state_t mgt_cpufreq_eard_governor_get_list(ctx_t *c, uint *governors);

state_t mgt_cpufreq_eard_governor_set(ctx_t *c, uint governor);

state_t mgt_cpufreq_eard_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask);

state_t mgt_cpufreq_eard_governor_set_list(ctx_t *c, uint *governors);

#endif //MANAGEMENT_CPUFREQ_ARCHS_EARD_H
