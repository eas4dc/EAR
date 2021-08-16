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

#ifndef MANAGEMENT_CPUFREQ_ARCHS_DEFAULT
#define MANAGEMENT_CPUFREQ_ARCHS_DEFAULT

#include <management/cpufreq/cpufreq.h>

// Dummy/default/generic API:
//
// This API is designed to nothing more than rely on the driver. It just sorts
// some of the driver raw information.

state_t cpufreq_default_status(topology_t *_tp);

state_t cpufreq_default_init(ctx_t *c, mgt_ps_driver_ops_t *ops_driver);

state_t cpufreq_default_init_user(ctx_t *c, mgt_ps_driver_ops_t *ops_driver, const ullong *freq_list, uint freq_count);

state_t cpufreq_default_dispose(ctx_t *c);

// Data
state_t cpufreq_default_count_available(ctx_t *c, uint *pstate_count);

// Getters
state_t cpufreq_default_get_available_list(ctx_t *c, pstate_t *pstate_list);

state_t cpufreq_default_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t cpufreq_default_get_nominal(ctx_t *c, uint *pstate_index);

state_t cpufreq_default_get_governor(ctx_t *c, uint *governor);

state_t cpufreq_default_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

// Setters
state_t cpufreq_default_set_current_list(ctx_t *c, uint *pstate_index);

state_t cpufreq_default_set_current(ctx_t *c, uint pstate_index, int cpu);

state_t cpufreq_default_set_governor(ctx_t *c, uint governor);

#endif //MANAGEMENT_CPUFREQ_ARCHS_DEFAULT
