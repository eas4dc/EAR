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

#include <metrics/frequency/imc.h>
#include <metrics/frequency/imc/intel63.h>

#define opreturn(call, ...) \
	if (call == NULL) { \
		return_msg(EAR_UNDEFINED, Generr.api_undefined); \
	} \
	return call (__VA_ARGS__);

static struct imc_freq_ops
{
	state_t (*init) (topology_t *tp);
	state_t (*dispose) ();
	state_t (*read) (freq_imc_t *ef);
	state_t (*read_diff) (freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);
	state_t (*read_copy) (freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);
	state_t (*data_alloc) (freq_imc_t *ef, ulong *freqs[], ulong *freqs_count);
	state_t (*data_count) (uint *count);
	state_t (*data_copy) (freq_imc_t *ef_dst, freq_imc_t *ef_src);
	state_t (*data_free) (freq_imc_t *ef, ulong *freqs[]);
	state_t (*data_diff) (freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);
	state_t (*data_print) (ulong *freqs, ulong *average);
} ops;

state_t freq_imc_init(topology_t *tp)
{
	if (tp->vendor == VENDOR_INTEL &&
		tp->model  >= MODEL_HASWELL_X)
	{
		ops.init		= ifreq_intel63_init;
		ops.dispose		= ifreq_intel63_dispose;
		ops.read		= ifreq_intel63_read;
		ops.read_diff	= ifreq_intel63_read_diff;
		ops.read_copy	= ifreq_intel63_read_copy;
		ops.data_alloc	= ifreq_intel63_data_alloc;
		ops.data_count	= ifreq_intel63_data_count;
		ops.data_copy	= ifreq_intel63_data_copy;
		ops.data_free   = ifreq_intel63_data_free;
		ops.data_diff   = ifreq_intel63_data_diff;
		ops.data_print  = ifreq_intel63_data_print;
		return ops.init(tp);
	} else {
		return_msg(EAR_INCOMPATIBLE, Generr.api_incompatible);
	}
}

state_t freq_imc_dispose()
{
	opreturn(ops.dispose,);
}

state_t freq_imc_read(freq_imc_t *ef)
{
	opreturn(ops.read, ef);
}

state_t freq_imc_read_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average)
{
	opreturn(ops.read_diff, ef2, ef1, freqs, average);
}

state_t freq_imc_read_copy(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average)
{
	opreturn(ops.read_copy, ef2, ef1, freqs, average);
}

state_t freq_imc_data_alloc(freq_imc_t *ef, ulong *freqs[], ulong *freqs_count)
{
	opreturn(ops.data_alloc, ef, freqs, freqs_count);
}

state_t freq_imc_data_count(uint *count)
{
	opreturn(ops.data_count, count);
}

state_t freq_imc_data_copy(freq_imc_t *ef_dst, freq_imc_t *ef_src)
{
	opreturn(ops.data_copy, ef_dst, ef_src);
}

state_t freq_imc_data_free(freq_imc_t *ef, ulong *freqs[])
{
	opreturn(ops.data_free, ef, freqs);
}

state_t freq_imc_data_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average)
{
	opreturn(ops.data_diff,  ef2, ef1, freqs, average);
}

state_t freq_imc_data_print(ulong *freqs, ulong *average)
{
	opreturn(ops.data_print, freqs, average);
}
