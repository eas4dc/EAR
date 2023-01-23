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

//#define SHOW_DEBUGS 1
#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/cpufreq/archs/dummy.h>

static uint cpu_count;
static topology_t my_topo;

state_t cpufreq_dummy_status(topology_t *tp, cpufreq_ops_t *ops)
{
	replace_ops(ops->init,      cpufreq_dummy_init);
	replace_ops(ops->dispose,   cpufreq_dummy_dispose);
	replace_ops(ops->count_devices, cpufreq_dummy_count_devices);
	replace_ops(ops->read,      cpufreq_dummy_read);
	replace_ops(ops->data_diff, cpufreq_dummy_data_diff);
    	cpu_count = tp->cpu_count;
	topology_copy(&my_topo, tp);
	return EAR_SUCCESS;
}

state_t cpufreq_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cpufreq_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cpufreq_dummy_count_devices(ctx_t *c, uint *cpu_count_in)
{
	*cpu_count_in = cpu_count;
	return EAR_SUCCESS;
}

state_t cpufreq_dummy_read(ctx_t *c, cpufreq_t *f)
{
	int cpu;
	debug("cpufreq_dummy_readi cpus=%d", cpu_count);
	for (cpu = 0; cpu < cpu_count; ++cpu) {
		f[cpu].freq_aperf = my_topo.base_freq;
		f[cpu].freq_mperf = my_topo.base_freq;
		f[cpu].error = 0;
	}
	return EAR_SUCCESS;
}

state_t cpufreq_dummy_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
    uint c;

	if ((freqs == NULL) || (average == NULL)){
		if (freqs == NULL) {debug("freqs are null");}
		if (average == NULL) {debug("average is null");}
	}
	if (freqs != NULL) {
		memset(freqs, 0, cpu_count * sizeof(ulong));
		for (c = 0; c < cpu_count; c++){
			freqs[c] = my_topo.base_freq;
		}
	}
	if (average != NULL) {
		*average = my_topo.base_freq;
	}
	debug("Average cpufreq %lu", *average);
	return EAR_SUCCESS;
}
