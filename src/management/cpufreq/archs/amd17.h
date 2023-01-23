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

#ifndef MANAGEMENT_CPUFREQ_ARCHS_AMD17
#define MANAGEMENT_CPUFREQ_ARCHS_AMD17

#include <management/cpufreq/cpufreq.h>

// AMD Frequency limitations when using Virtual P_STATEs mode
//	- Although you can change the frequency by MSR, when doing it, all socket MSR Px register
//	  change together. This means that is not possible to set different range of frequencies
//	  accross the same socket. You can set different frequencies by using different MSR Px
//	  registers, but currently we are using just the MSR P1.
//	- This means that is not possible to use different frequencies per thread/core. Just P1
//	  and boost.

state_t mgt_cpufreq_amd17_load(topology_t *tp, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver);

state_t mgt_cpufreq_amd17_init(ctx_t *c);

state_t mgt_cpufreq_amd17_dispose(ctx_t *c);

state_t mgt_cpufreq_amd17_count_available(ctx_t *c, uint *pstate_count);

state_t mgt_cpufreq_amd17_get_available_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_amd17_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_cpufreq_amd17_get_nominal(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_amd17_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

state_t mgt_cpufreq_amd17_set_current_list(ctx_t *c, uint *pstate_index);

state_t mgt_cpufreq_amd17_set_current(ctx_t *c, uint pstate_index, int cpu);

state_t mgt_cpufreq_amd17_reset(ctx_t *c);

//Governors
state_t mgt_cpufreq_amd17_governor_get(ctx_t *c, uint *governor);

state_t mgt_cpufreq_amd17_governor_get_list(ctx_t *c, uint *governors);

state_t mgt_cpufreq_amd17_governor_set(ctx_t *c, uint governor);

state_t mgt_cpufreq_amd17_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask);

state_t mgt_cpufreq_amd17_governor_set_list(ctx_t *c, uint *governors);

#endif //MANAGEMENT_CPUFREQ_ARCHS_AMD17
