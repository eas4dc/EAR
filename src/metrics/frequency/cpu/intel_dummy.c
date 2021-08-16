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
#include <metrics/frequency/cpu/intel_dummy.h>
#include <metrics/frequency/cpufreq_base.h>


static topology_t tp;

typedef struct aperf_intel_dummy_s
{
        ulong freq_aperf;
        ulong freq_mperf;
        uint snapped;
        uint error;
} aperf_intel_dummy_t;


state_t cpufreq_intel_dummy_status(topology_t *t)
{

	// Thread control required in this small section
	if (tp.cpus == NULL) {
		topology_copy(&tp,t);
	}
	debug("CPUfreq intel dummy status compatible");
	return EAR_SUCCESS;
}

static state_t static_init_test()
{
	return EAR_SUCCESS;
}


state_t cpufreq_intel_dummy_init()
{
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_dispose()
{
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_read(cpufreq_t *f)
{
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_read_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	*average = 0;
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_read_copy(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	*average = 0;
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_data_alloc(cpufreq_t *f, ulong *freqs[])
{
	uint cpufreq_size;
	uint freqs_count;
	state_t s;
	//
	if (xtate_fail(s, cpufreq_intel_dummy_data_count(&cpufreq_size, &freqs_count))) {
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

state_t cpufreq_intel_dummy_data_count(uint *cpufreq_size, uint *freqs_count)
{
	if (cpufreq_size != NULL) {
		*cpufreq_size = sizeof(aperf_intel_dummy_t) * tp.cpu_count;
	}
	if (freqs_count != NULL) {
		*freqs_count = tp.cpu_count;
	}
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_data_copy(cpufreq_t *f_dst, cpufreq_t *f_src)
{
	if (f_dst == NULL || f_src == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (f_dst->context == NULL || f_src->context == NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}
	memcpy(f_dst->context, f_src->context, sizeof(aperf_intel_dummy_t) * tp.cpu_count);
	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_data_free(cpufreq_t *f, ulong *freqs[])
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

state_t cpufreq_intel_dummy_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	//aperf_intel_dummy_t *d2;
	//aperf_intel_dummy_t *d1;
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
	//d2 = f2->context;
	//d1 = f1->context;
	// If average not null, then clean.
	if (average != NULL) {
		*average = 0LU;
	}
	for (cpu = 0;cpu < tp.cpu_count; cpu++) freqs[cpu] = 0;

	return EAR_SUCCESS;
}

state_t cpufreq_intel_dummy_data_print(ulong *freqs, ulong *average)
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
