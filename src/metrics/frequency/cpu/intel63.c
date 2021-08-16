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
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <common/hardware/cpuid.h>
#include <metrics/common/msr.h>
#include <metrics/frequency/cpu/intel63.h>
#include <metrics/frequency/cpufreq_base.h>

#define MSR_IA32_APERF 0x000000E8
#define MSR_IA32_MPERF 0x000000E7

static topology_t tp;
static ulong freq_base;
static uint user_mode;
static uint init = 0;

typedef struct aperf_intel63_s
{
	ulong freq_aperf;
	ulong freq_mperf;
	uint snapped;
	uint error;
} aperf_intel63_t;

state_t cpufreq_intel63_status(topology_t *t)
{
	int compat1;
	int compat2;

	compat1 = (t->vendor == VENDOR_AMD   && t->family >= FAMILY_ZEN);
	compat2 = (t->vendor == VENDOR_INTEL && t->model  >= MODEL_HASWELL_X);
	if (!compat1 && !compat2) {
		return EAR_ERROR;
	}
	// Thread control required in this small section
	if (tp.cpus == NULL) {
		topology_copy(&tp,t);
	}
	debug("CPUfreq intel status compatible");
	return EAR_SUCCESS;
}

static state_t static_init_test()
{
	if (!init) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	return EAR_SUCCESS;
}

static state_t static_dispose(uint close_msr_up_to, state_t s, char *msg)
{
	int cpu;
	for (cpu = 0; cpu < close_msr_up_to; ++cpu) {
		msr_close(tp.cpus[cpu].id);
	}
	init = 0;
	return_msg(s, msg);
}

state_t cpufreq_intel63_init()
{
	state_t s;
	int cpu;
	// This is a patch to allow multiple inits without control, but it has to be fixed.
	if (xtate_ok(s, static_init_test())) {
		return EAR_SUCCESS;
	}
	// Opening MSRs.
	for (cpu = 0; cpu < tp.cpu_count && !user_mode; ++cpu) {
		if (xtate_fail(s, msr_open(tp.cpus[cpu].id))) {
			// If fails, enters in user mode.
			if (state_is(s, EAR_NO_PERMISSIONS)) {
				user_mode = 1;
			} else {
				return static_dispose(cpu, s, state_msg);
			}
		}
	}
	// Getting base frequency
	cpufreq_get_base(&tp, &freq_base);
	debug("Freq base %lu",freq_base);
	// Computing base frequency
	init = 1;
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_dispose()
{
	state_t s;
	if (xtate_fail(s, static_init_test())) {
		return s;
	}
	return static_dispose(tp.cpu_count, EAR_SUCCESS, NULL);
}

state_t cpufreq_intel63_read(cpufreq_t *f)
{
	aperf_intel63_t *a = f->context;
	state_t s1, s2;
	int cpu;

	if (xtate_fail(s1, static_init_test())) {
		return s1;
	}
	if (user_mode) {
		return_msg(EAR_NO_PERMISSIONS, Generr.no_permissions);
	}
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		s1 = msr_read(tp.cpus[cpu].id, &a[cpu].freq_mperf, sizeof(ulong), MSR_IA32_MPERF);
		s2 = msr_read(tp.cpus[cpu].id, &a[cpu].freq_aperf, sizeof(ulong), MSR_IA32_APERF);
		a[cpu].snapped = !(state_fail(s1) || state_fail(s2));
		a[cpu].error   = !(a[cpu].snapped);
	}
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_read_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	state_t s;
	if (xtate_fail(s, cpufreq_intel63_read(f2))) {
		return s;
	}
	return cpufreq_intel63_data_diff(f2, f1, freqs, average);
}

state_t cpufreq_intel63_read_copy(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	state_t s;
	if (xtate_fail(s, cpufreq_intel63_read_diff(f2, f1, freqs, average))) {
		return s;
	}
	return cpufreq_intel63_data_copy(f1, f2);
}

state_t cpufreq_intel63_data_alloc(cpufreq_t *f, ulong *freqs[])
{
	uint cpufreq_size;
	uint freqs_count;
	state_t s;
	//
	if (xtate_fail(s, static_init_test())) {
		return s;
	}	
	if (xtate_fail(s, cpufreq_intel63_data_count(&cpufreq_size, &freqs_count))) {
		return s;
	}
	// Allocating space for cpufreq_t
	if (posix_memalign((void **) &f->context, 64, cpufreq_size) != 0) {
		return_msg(EAR_ERROR, Generr.alloc_error);
	}
	f->size = cpufreq_size;
	debug("Allocating %u frequencies of size %u", freqs_count, cpufreq_size);
	// Allocating space for freqs[]
	if (freqs != NULL) {
		if (posix_memalign((void **) freqs, 64, sizeof(ulong) * freqs_count) != 0) {
			return_msg(EAR_ERROR, Generr.alloc_error);
		}
	}

	return EAR_SUCCESS;
}

state_t cpufreq_intel63_data_count(uint *cpufreq_size, uint *freqs_count)
{
	state_t s;
	if (xtate_fail(s, static_init_test())) {
		return s;
	}
	if (cpufreq_size != NULL) {
		*cpufreq_size = sizeof(aperf_intel63_t) * tp.cpu_count;
	}
	if (freqs_count != NULL) {
		*freqs_count = tp.cpu_count;
	}
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_data_copy(cpufreq_t *f_dst, cpufreq_t *f_src)
{
	state_t s;
	if (xtate_fail(s, static_init_test())) {
		return s;
	}
	if (f_dst == NULL || f_src == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (f_dst->context == NULL || f_src->context == NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}
	memcpy(f_dst->context, f_src->context, sizeof(aperf_intel63_t) * tp.cpu_count);
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_data_free(cpufreq_t *f, ulong *freqs[])
{
	if (f == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (f->context != NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}
	if (freqs != NULL && *freqs != NULL) {
		free(*freqs);
		*freqs = NULL;
	}
	free(f->context);
	f = NULL;
	return EAR_SUCCESS;
}

state_t cpufreq_intel63_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	ulong mperf_diff, aperf_diff, aperf_pcnt;
	aperf_intel63_t *d2;
	aperf_intel63_t *d1;
	ulong valid_count = 0;
	ulong freq_aux;
	state_t s;
	int cpu;

	if (xtate_fail(s, static_init_test())) {
		return s;
	}
	// If null return.
	if (f2 == NULL || f1 == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	// If empty return.
	if (f2->context == NULL || f1->context == NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}
	d2 = f2->context;
	d1 = f1->context;
	// If average not null, then clean.
	if (average != NULL) {
		*average = 0LU;
	}
	// Iterating over all CPU cores.
	for (cpu = 0; cpu < tp.cpu_count; ++cpu)
	{
		// Also frequency clean.
		if (freqs != NULL) {
			freqs[cpu] = 0LU;
		}
		// If there is no error in the lectures.
		if (d2[cpu].error || d1[cpu].error) {
			continue;
		}
		// Counting valid samples to perform good average.
		valid_count += 1LU;
		// Differences between APERF/MPERF counters between both samples.
		if (d2[cpu].freq_mperf < d1[cpu].freq_mperf){
			debug("Warning, potential overflow in CPUfreq computation");
			mperf_diff = ULONG_MAX - d1[cpu].freq_mperf + d2[cpu].freq_mperf;
		}else{
			mperf_diff = d2[cpu].freq_mperf - d1[cpu].freq_mperf;
		}
		if (d2[cpu].freq_aperf < d1[cpu].freq_aperf){
			debug("Warning, potential overflow in CPUfreq computation");
			aperf_diff = ULONG_MAX - d1[cpu].freq_aperf + d2[cpu].freq_aperf;
		}else{
			aperf_diff = d2[cpu].freq_aperf - d1[cpu].freq_aperf;
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
			debug("CPU%d: %lu = %lu * %lu", cpu, freq_aux, freq_base, aperf_pcnt);
			*average += freq_aux;
		}
	}
	if (average != NULL) {
		*average = *average / valid_count;
	}

	return EAR_SUCCESS;
}

state_t cpufreq_intel63_data_print(ulong *freqs, ulong *average)
{
	double freq_ghz;
	int cpu_half;
	int s, i, j;

	if (xtate_fail(s, static_init_test())) {
		return s;
	}
	//
	cpu_half = tp.cpu_count / 2;
	printf("CPU:");
	//
	for (s = 0, j = 0; freqs != NULL && s < tp.socket_count; ++s)
	{
		for (i = 0; i < cpu_half; ++i, ++j) {
        		freq_ghz = ((double) freqs[j]) / 1000000.0;
        		printf(" %0.1lf", freq_ghz);
    		}
		if ((s+1) < tp.socket_count) printf("\nCPU:");
	}

	if (freqs != NULL) {
		printf(",");
	}
    if (average != NULL) {
		freq_ghz = ((double) *average) / 1000000.0;
        printf(" avg %0.1lf", freq_ghz);
    }   
    printf(" (GHz)\n");

    return EAR_SUCCESS;
}
