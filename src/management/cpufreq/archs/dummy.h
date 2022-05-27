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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef MANAGEMENT_CPUFREQ_ARCHS_DUMMY_H
#define MANAGEMENT_CPUFREQ_ARCHS_DUMMY_H

#include <management/cpufreq/cpufreq.h>

state_t mgt_cpufreq_dummy_load(topology_t *tp_in, mgt_ps_ops_t *ops);

state_t mgt_cpufreq_dummy_init(ctx_t *c);

state_t mgt_cpufreq_dummy_dispose(ctx_t *c);

// Data
state_t mgt_cpufreq_dummy_count_available(ctx_t *c, uint *pstate_count);

// Getters
state_t mgt_cpufreq_dummy_get_available_list(ctx_t *c, pstate_t *pstate_list);
/** */
state_t mgt_cpufreq_dummy_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_dummy_get_nominal(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_dummy_get_governor(ctx_t *c, uint *governor);

state_t mgt_cpufreq_dummy_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

// Setters
/** */
state_t mgt_cpufreq_dummy_set_current_list(ctx_t *c, uint *pstate_index);
/** */
state_t mgt_cpufreq_dummy_set_current(ctx_t *c, uint pstate_index, int cpu);
/** */
state_t mgt_cpufreq_dummy_set_governor(ctx_t *c, uint governor);

#endif //MANAGEMENT_CPUFREQ_ARCHS_DUMMY_H
