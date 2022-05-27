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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/cpufreq/archs/intel63.h>
#include <metrics/cpufreq/cpufreq_base.h>

#define MSR_IA32_APERF 0x000000E8
#define MSR_IA32_MPERF 0x000000E7

static topology_t tp;
static ulong freq_base;

// aperf
state_t cpufreq_intel63_status(topology_t *tp_in, cpufreq_ops_t *ops)
{
	int compat1;
	int compat2;
	state_t s;

	// Compatibility test
	compat1 = (tp_in->vendor == VENDOR_AMD   && tp_in->family >= FAMILY_ZEN);
	compat2 = (tp_in->vendor == VENDOR_INTEL && tp_in->model  >= MODEL_HASWELL_X);
	if (!compat1 && !compat2) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	// Copying the topology
	topology_copy(&tp, tp_in);
	// Getting base frequency
	cpufreq_get_base(&tp, &freq_base);
	// If it is compatible these functions takes the control
	replace_ops(ops->count_devices, cpufreq_intel63_count_devices);
	replace_ops(ops->data_diff,     cpufreq_intel63_data_diff);
	// Permissions test
	if (state_fail(s = msr_test(tp_in, MSR_RD))) {
		debug("msr_test failed");
		return EAR_ERROR;
	}
	//debug("INTEL63 full permissions? %d", can_read);
	debug("INTEL63 cpufreq ok");
	// If reading MSR is allowed
	replace_ops(ops->init,    cpufreq_intel63_init);
	replace_ops(ops->dispose, cpufreq_intel63_dispose);
	replace_ops(ops->read,    cpufreq_intel63_read);
	
	return EAR_SUCCESS;
}

static state_t static_dispose(ctx_t *c, int close_msr_up_to, state_t s)
{
	int cpu;
	for (cpu = 0; cpu < close_msr_up_to; ++cpu) {
		msr_close(tp.cpus[cpu].id);
	}
	return s;
}

state_t cpufreq_intel63_init(ctx_t *c)
{
	state_t s;
	int cpu;
	// Opening MSRs.
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (state_fail(s = msr_open(tp.cpus[cpu].id, MSR_RD))) {
			return static_dispose(c, cpu, s);
		}
	}
	debug("cpufreq_intel63_init ready");
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_dispose(ctx_t *c)
{
	return static_dispose(c, tp.cpu_count, EAR_SUCCESS);
}

state_t cpufreq_intel63_count_devices(ctx_t *c, uint *cpu_count)
{
	*cpu_count = tp.cpu_count;
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_read(ctx_t *c, cpufreq_t *f)
{
	state_t s1, s2;
	int cpu;

	debug("cpufreq_intel63_read");
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		s1 = msr_read(tp.cpus[cpu].id, &f[cpu].freq_mperf, sizeof(ulong), MSR_IA32_MPERF);
		s2 = msr_read(tp.cpus[cpu].id, &f[cpu].freq_aperf, sizeof(ulong), MSR_IA32_APERF);
		f[cpu].error = (state_fail(s1) || state_fail(s2));
	}
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	ulong mperf_diff, aperf_diff, aperf_pcnt;
	ulong valid_count = 0;
	ulong freq_aux;
	int cpu;

	debug("cpufreq_intel63_data diff");
	// If null return.
	if (f2 == NULL || f1 == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	// If average not null, then clean.
	if (average != NULL) {
		*average = 0LU;
	}
	// Iterating over all CPU cores.
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		// Also frequency clean.
		if (freqs != NULL) {
			freqs[cpu] = 0LU;
		}
		// If there is no error in the lectures.
		if (f2[cpu].error || f1[cpu].error) {
			continue;
		}
		// Counting valid samples to perform good average.
		valid_count += 1LU;
		// Differences between APERF/MPERF counters between both samples.
		if (f2[cpu].freq_mperf < f1[cpu].freq_mperf){
			debug("Warning, potential overflow in MPERF computation");
			mperf_diff = ULONG_MAX - f1[cpu].freq_mperf + f2[cpu].freq_mperf;
		} else {
			mperf_diff = f2[cpu].freq_mperf - f1[cpu].freq_mperf;
		}
		if (f2[cpu].freq_aperf < f1[cpu].freq_aperf){
			debug("Warning, potential overflow in APERF computation");
			aperf_diff = ULONG_MAX - f1[cpu].freq_aperf + f2[cpu].freq_aperf;
		} else {
			aperf_diff = f2[cpu].freq_aperf - f1[cpu].freq_aperf;
		}
		#if 0
		debug("CPU %d: APERF %lu - %lu = , MPERF %lu - %lu = %lu", tp.cpus[cpu].id,
			f2[cpu].freq_aperf, f1[cpu].freq_aperf, aperf_diff,
			f2[cpu].freq_mperf, f1[cpu].freq_mperf, mperf_diff);
		#endif
		// Preventing floating point exception
		if (aperf_diff == 0 || mperf_diff == 0) {
			continue;
		}
		// The aperf percentage function includes a multiplication per 100. The
		// only way to overflow that counter is measuring if the aperf
		// difference is smaller than the maximum ulong value ((ulong) -1LU)
		// divided by 100. In case it is, removing 7 bits prevents that problem.
		// But it is an incorrect solution because it would take a lot of bits
		// and does not control when the first APERF/MPERF is greater the
		// second. It has to be fixed.
		if (((ulong)(-1LU) / 100LU) < aperf_diff) {
			aperf_diff >>= 7;
			mperf_diff >>= 7;
		}
		// With the percentage applied to the base frequency, finally can be
		// computed the average frequency of a specific CPU.
		aperf_pcnt = (aperf_diff * 100LU) / mperf_diff;
		freq_aux   = (freq_base * aperf_pcnt) / 100LU;
		//
		if (freqs != NULL) {
			freqs[cpu] = freq_aux;
		}
		if (average != NULL) {
			*average += freq_aux;
		}
	}
	if (average != NULL && valid_count > 0LU) {
		*average = *average / valid_count;
	}

	return EAR_SUCCESS;
}
