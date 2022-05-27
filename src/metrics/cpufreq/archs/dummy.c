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

#include <stdlib.h>
#include <metrics/cpufreq/archs/dummy.h>

static uint cpu_count;

state_t cpufreq_dummy_status(topology_t *tp, cpufreq_ops_t *ops)
{
	replace_ops(ops->init,      cpufreq_dummy_init);
	replace_ops(ops->dispose,   cpufreq_dummy_dispose);
	replace_ops(ops->count_devices, cpufreq_dummy_count_devices);
	replace_ops(ops->read,      cpufreq_dummy_read);
	replace_ops(ops->data_diff, cpufreq_dummy_data_diff);
    cpu_count = tp->cpu_count;
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
	for (cpu = 0; cpu < cpu_count; ++cpu) {
		f[cpu].freq_aperf = 0LU;
		f[cpu].freq_mperf = 0LU;
		f[cpu].error = 1;
	}
	return EAR_SUCCESS;
}

state_t cpufreq_dummy_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	if (freqs != NULL) {
		memset(freqs, 0, cpu_count * sizeof(ulong));
	}
	if (average != NULL) {
		*average = 0;
	}
	return EAR_SUCCESS;
}
