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

#include <daemon/local_api/eard_api.h>
#include <management/cpufreq/cpufreq_user.h>

extern uint mgt_cpu_count;

state_t mgt_cpufreq_user_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	ulong list_khz[4096]; // I hope there are no nodes with more than 4096 CPUs
	uint cpu;
	// Cleaning
	memset(pstate_list, 0, sizeof(pstate_t)*mgt_cpu_count);
	// If 0 then an error ocurred
	if (eards_get_freq_list(mgt_cpu_count, list_khz)) {
		return_msg(EAR_ERROR, "error while contacting daemon");
	}
	// Getting also P_STATE
	for (cpu = 0; cpu < mgt_cpu_count; ++cpu) {
		pstate_list[cpu].khz = (ullong) list_khz[cpu];
		if (state_fail(mgt_cpufreq_get_index(c, pstate_list[cpu].khz, &pstate_list[cpu].idx, 0))) {
		}
	}

	return EAR_SUCCESS;
}
