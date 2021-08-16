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
//#define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <metrics/frequency/cpu.h>
#include <metrics/frequency/cpu/intel63.h>
#include <metrics/frequency/cpu/intel_dummy.h>
static struct cpu_freq_ops
{
	state_t (*init)       ();
	state_t (*dispose)    ();
	state_t (*read)       (cpufreq_t *ef);
	state_t (*read_diff)  (cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average);
	state_t (*read_copy)  (cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average);
	state_t (*data_alloc) (cpufreq_t *ef, ulong *freqs[]);
	state_t (*data_count) (uint *cpufreq_size, uint *freqs_count);
	state_t (*data_copy)  (cpufreq_t *ef_dst, cpufreq_t *ef_src);
	state_t (*data_free)  (cpufreq_t *ef, ulong *freqs[]);
	state_t (*data_diff)  (cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average);
	state_t (*data_print) (ulong *freqs, ulong *average);
} ops;

state_t cpufreq_load(topology_t *tp)
{
	debug("Testing CPU freq API");
	if (state_ok(cpufreq_intel63_status(tp)))
	{
		ops.init		= cpufreq_intel63_init;
		ops.dispose		= cpufreq_intel63_dispose;
		ops.read		= cpufreq_intel63_read;
		ops.read_diff	= cpufreq_intel63_read_diff;
		ops.read_copy	= cpufreq_intel63_read_copy;
		ops.data_alloc	= cpufreq_intel63_data_alloc;
		ops.data_count  = cpufreq_intel63_data_count;
		ops.data_copy	= cpufreq_intel63_data_copy;
		ops.data_free   = cpufreq_intel63_data_free;
		ops.data_diff   = cpufreq_intel63_data_diff;
		ops.data_print  = cpufreq_intel63_data_print;
	} else {
		debug("CPUfreq dummy loaded ");
		ops.init                = cpufreq_intel_dummy_init;
                ops.dispose             = cpufreq_intel_dummy_dispose;
                ops.read                = cpufreq_intel_dummy_read;
                ops.read_diff   = cpufreq_intel_dummy_read_diff;
                ops.read_copy   = cpufreq_intel_dummy_read_copy;
                ops.data_alloc  = cpufreq_intel_dummy_data_alloc;
                ops.data_count  = cpufreq_intel_dummy_data_count;
                ops.data_copy   = cpufreq_intel_dummy_data_copy;
                ops.data_free   = cpufreq_intel_dummy_data_free;
                ops.data_diff   = cpufreq_intel_dummy_data_diff;
                ops.data_print  = cpufreq_intel_dummy_data_print;

	}
	return EAR_SUCCESS;
}

state_t cpufreq_init()
{
	preturn(ops.init,);
}

state_t cpufreq_dispose()
{
	preturn(ops.dispose,);
}

state_t cpufreq_read(cpufreq_t *ef)
{
	preturn(ops.read, ef);
}

state_t cpufreq_read_diff(cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average)
{
	preturn(ops.read_diff, ef2, ef1, freqs, average);
}

state_t cpufreq_read_copy(cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average)
{
	preturn(ops.read_copy, ef2, ef1, freqs, average);
}

state_t cpufreq_data_alloc(cpufreq_t *ef, ulong **freqs)
{
	preturn(ops.data_alloc, ef, freqs);
}

state_t cpufreq_data_count(uint *cpufreq_size, uint *freqs_count)
{
	preturn(ops.data_count, cpufreq_size, freqs_count);
}

state_t cpufreq_data_copy(cpufreq_t *ef_dst, cpufreq_t *ef_src)
{
	preturn(ops.data_copy, ef_dst, ef_src);
}

state_t cpufreq_data_free(cpufreq_t *ef, ulong **freqs)
{
	preturn(ops.data_free, ef, freqs);
}

state_t cpufreq_data_diff(cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average)
{
	preturn(ops.data_diff,  ef2, ef1, freqs, average);
}

state_t cpufreq_data_print(ulong *freqs, ulong *average)
{
    preturn(ops.data_print, freqs, average);
}
