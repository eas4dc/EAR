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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <metrics/common/msr.h>
#include <metrics/frequency/cpu/intel63.h>

#define MSR_IA32_APERF 0x000000E8
#define MSR_IA32_MPERF 0x000000E7

#define debug(...) \
	fprintf(stderr, __VA_ARGS__);

static uint cpu_count;
static topology_t tp;

typedef struct aperf_intel63_s
{
	ulong freq_aperf;
	ulong freq_mperf;
	uint snapped;
	uint error;
} aperf_intel63_t;

state_t freq_intel63_init(topology_t *_tp)
{
	state_t s;
	int cpu;
	
	// Static data
	if (tp.cpus == NULL) {
		if(xtate_fail(s, topology_select(_tp, &tp, TPSelect.core, TPGroup.merge, 0))) {
			return s;
		}
	}
	if (cpu_count == 0) {
		cpu_count = tp.cpu_count;
	}

	// Opening MSR
	for (cpu = 0; cpu < cpu_count; ++cpu) {
		if (xtate_fail(s, msr_open(tp.cpus[cpu].id))) {
			return s;
		}
	}

	return EAR_SUCCESS;
}

state_t freq_intel63_dispose()
{
	state_t s1, s2 = EAR_SUCCESS;
	int cpu;

	if (cpu_count == 0) {
		return_msg(EAR_NOT_INITIALIZED, Generr.api_uninitialized);
	}
	for (cpu = 0; cpu < cpu_count; ++cpu) {
		s1 = msr_close(tp.cpus[cpu].id);
		if (state_fail(s1)) s2 = s1;
	}

	return s2;
}

state_t freq_intel63_read(freq_cpu_t *ef)
{
	aperf_intel63_t *a = ef->data;
	state_t s1, s2;
	int cpu;

	if (cpu_count == 0) {
		return_msg(EAR_NOT_INITIALIZED, Generr.api_uninitialized);
	}

	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		s1 = msr_read(tp.cpus[cpu].id, &a[cpu].freq_mperf, sizeof(ulong), MSR_IA32_MPERF);
		s2 = msr_read(tp.cpus[cpu].id, &a[cpu].freq_aperf, sizeof(ulong), MSR_IA32_APERF);
		a[cpu].snapped = !(state_fail(s1) || state_fail(s2));
		a[cpu].error   = !(a[cpu].snapped);
	}

	return EAR_SUCCESS;
}

state_t freq_intel63_read_diff(freq_cpu_t *ef2, freq_cpu_t *ef1, ulong *freqs, ulong *average)
{
	state_t s;

	if (xtate_fail(s, freq_intel63_read(ef2))) {
		return s;
	}

	return freq_intel63_data_diff(ef2, ef1, freqs, average);
}

state_t freq_intel63_read_copy(freq_cpu_t *ef2, freq_cpu_t *ef1, ulong *freqs, ulong *average)
{
	state_t s;

	if (xtate_fail(s, freq_intel63_read_diff(ef2, ef1, freqs, average))) {
		return s;
	}

	return freq_intel63_data_copy(ef1, ef2);
}

state_t freq_intel63_data_alloc(freq_cpu_t *ef, ulong *freqs[], ulong *freqs_count)
{
	size_t size = sizeof(aperf_intel63_t);
	aperf_intel63_t *a;
	state_t s;
	int cpu;

	if (freqs_count != NULL) {
		*freqs_count = 0;
	}
	if (cpu_count == 0) {
		return_msg(EAR_NOT_INITIALIZED, Generr.api_uninitialized);
	}

	// Allocating space
	if (posix_memalign((void **) &ef->data, 64, size * cpu_count) != 0) {
		return_msg(EAR_ERROR, Generr.alloc_error);
	}

	if (freqs != NULL) {
		if (posix_memalign((void **) freqs, 64, sizeof(ulong) * cpu_count) != 0) {
			return_msg(EAR_ERROR, Generr.alloc_error);
		}
		if (freqs_count != NULL) {
			*freqs_count = cpu_count;
		}
	}

	// Initialization
	a = (aperf_intel63_t *) ef->data;

	return EAR_SUCCESS;
}

state_t freq_intel63_data_count(uint *count)
{
	*count = cpu_count;
	return EAR_SUCCESS;
}

state_t freq_intel63_data_copy(freq_cpu_t *ef_dst, freq_cpu_t *ef_src)
{
	if (cpu_count == 0) {
		return_msg(EAR_NOT_INITIALIZED, Generr.api_uninitialized);
	}
	if (ef_dst == NULL || ef_src == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (ef_dst->data == NULL || ef_src->data == NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}

	memcpy(ef_dst->data, ef_src->data, sizeof(aperf_intel63_t) * cpu_count);

	return EAR_SUCCESS;
}

state_t freq_intel63_data_free(freq_cpu_t *ef, ulong *freqs[])
{
	if (ef == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (ef->data != NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}
	if (freqs != NULL && *freqs != NULL) {
		free(*freqs);
		*freqs = NULL;
	}
	free(ef->data);
	ef = NULL;

	return EAR_SUCCESS;
}

state_t freq_intel63_data_diff(freq_cpu_t *ef2, freq_cpu_t *ef1, ulong *freqs, ulong *average)
{
	ulong mperf_diff, aperf_diff, aperf_pcnt;
	aperf_intel63_t *d2;
	aperf_intel63_t *d1;
	ulong valid_count = 0;
	ulong freq;
	int cpu;

	if (ef2 == NULL || ef1 == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (ef2->data == NULL || ef1->data == NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}
	d2 = ef2->data;
	d1 = ef1->data;

	//
	if (average != NULL) {
		*average = 0LU;
	}

	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		if (freqs != NULL) {
			freqs[cpu] = 0LU;
		}
		if (d2[cpu].error || d1[cpu].error) {
			continue;
		}
		//
		valid_count += 1LU;
		//
		mperf_diff = d2[cpu].freq_mperf - d1[cpu].freq_mperf;
		aperf_diff = d2[cpu].freq_aperf - d1[cpu].freq_aperf;
		//
		if (((ulong)(-1LU) / 100LU) < aperf_diff) {
			aperf_diff >>= 7;
			mperf_diff >>= 7;
		}
		aperf_pcnt = (aperf_diff * 100LU) / mperf_diff;
		//
		freq = (tp.cpus[cpu].freq_base * aperf_pcnt) / 100LU;
		if (freqs != NULL) {
			freqs[cpu] = freq;
		}
		if (average != NULL) {
			*average += freq;
		}
	}

	if (average != NULL) {
		*average = *average / valid_count;
	}

	return EAR_SUCCESS;
}

state_t freq_intel63_data_print(ulong *freqs, ulong *average)
{
    double freq_ghz;
	double freq_div;
	int cpu_half;
    int s, i, j;

	//
	cpu_half = cpu_count / 2;
    printf("CPU:");

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
