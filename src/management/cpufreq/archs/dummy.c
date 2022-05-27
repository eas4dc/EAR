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

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/cpufreq/cpufreq_base.h>
#include <management/cpufreq/archs/dummy.h>

static ullong base_freq;
static uint cpu_count;

state_t mgt_cpufreq_dummy_load(topology_t *tp_in, mgt_ps_ops_t *ops)
{
	cpu_count = tp_in->cpu_count;
	// Base freq
	cpufreq_get_base(tp_in, (ulong *) &base_freq);
	if (base_freq == 0LLU) {
		base_freq = 1000000LLU;
	}
	//
	replace_ops(ops->init,             mgt_cpufreq_dummy_init);
	replace_ops(ops->dispose,          mgt_cpufreq_dummy_dispose);
	replace_ops(ops->count_available,  mgt_cpufreq_dummy_count_available);
	replace_ops(ops->get_available_list, mgt_cpufreq_dummy_get_available_list);
	replace_ops(ops->get_current_list, mgt_cpufreq_dummy_get_current_list);
	replace_ops(ops->get_nominal,      mgt_cpufreq_dummy_get_nominal);
	replace_ops(ops->get_governor,     mgt_cpufreq_dummy_get_governor);
	replace_ops(ops->get_index,        mgt_cpufreq_dummy_get_index);
	replace_ops(ops->set_current_list, mgt_cpufreq_dummy_set_current_list);
	replace_ops(ops->set_current,      mgt_cpufreq_dummy_set_current);
	replace_ops(ops->set_governor,     mgt_cpufreq_dummy_set_governor);

	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_count_available(ctx_t *c, uint *pstate_count)
{
	*pstate_count = 1;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
	pstate_list[0].idx = 0;
	pstate_list[0].khz = base_freq;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	debug("mgt_cpufreq_dummy_get_current_list");
	int i;
	for (i = 0; i < cpu_count; ++i) {
		pstate_list[i].idx = i;
		pstate_list[i].idx = base_freq;
	}
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_nominal(ctx_t *c, uint *pstate_index)
{
	*pstate_index = 0;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_governor(ctx_t *c, uint *governor)
{
	*governor = Governor.userspace;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
	*pstate_index = 0;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_set_current_list(ctx_t *c, uint *pstate_index)
{
	debug("mgt_cpufreq_dummy_set_current_list");
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_set_current(ctx_t *c, uint pstate_index, int cpu)
{
	debug("mgt_cpufreq_dummy_set_current");
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_set_governor(ctx_t *c, uint governor)
{
	debug("mgt_cpufreq_dummy_set_governor");
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_data_print(pstate_t *pstate_list, uint pstate_count, int fd)
{
    int i;
    for (i = 0; i < pstate_count; ++i) {
        dprintf(fd, "CPU PS%d: id%d, %llu KHz\n", i, pstate_list[i].idx, pstate_list[i].khz);
    }
    return EAR_SUCCESS;
}
