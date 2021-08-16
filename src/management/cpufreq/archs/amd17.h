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

#ifndef MANAGEMENT_CPUFREQ_ARCHS_AMD17
#define MANAGEMENT_CPUFREQ_ARCHS_AMD17

#include <management/cpufreq/cpufreq.h>

// AMD Frequency limitations
//	- Although you can change the frequency by MSR, when doing it, all socket MSR Px register
//	  change together. This means that is not possible to set different range of frequencies
//	  accross the same socket. You can set different frequencies by using different MSR Px
//	  registers, but currently we are using just the MSR P1.
//	- This means that is not possible to use different frequencies per thread/core. Just P1
//	  and boost.

state_t cpufreq_amd17_status(topology_t *_tp);

state_t cpufreq_amd17_init(ctx_t *c, mgt_ps_driver_ops_t *ops_driver);

state_t cpufreq_amd17_init_user(ctx_t *c, mgt_ps_driver_ops_t *ops_driver, const ullong *freq_list, uint freq_count);

state_t cpufreq_amd17_dispose(ctx_t *c);

// Data
state_t cpufreq_amd17_count_available(ctx_t *c, uint *pstate_count);

// Getters
state_t cpufreq_amd17_get_available_list(ctx_t *c, pstate_t *pstate_list);

state_t cpufreq_amd17_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t cpufreq_amd17_get_nominal(ctx_t *c, uint *pstate_index);

state_t cpufreq_amd17_get_governor(ctx_t *c, uint *governor);

state_t cpufreq_amd17_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

// Setters
state_t cpufreq_amd17_set_current_list(ctx_t *c, uint *pstate_index);

state_t cpufreq_amd17_set_current(ctx_t *c, uint pstate_index, int cpu);

state_t cpufreq_amd17_set_governor(ctx_t *c, uint governor);

#endif //MANAGEMENT_CPUFREQ_ARCHS_AMD17
