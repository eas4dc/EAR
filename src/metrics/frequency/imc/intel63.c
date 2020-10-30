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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/frequency/imc/intel63.h>

#define UBOX_CTR_OFS	0x000704
#define UBOX_CTL_OFS	0x000703
#define UBOX_CMD_STA	0x400000
#define UBOX_CMD_STO	0x000000

typedef struct imc_intel63_s
{
	timestamp_t time;
	ulong freq;
	uint error;
} imc_intel63_t;

// In this capacity, the UBox acts as the central unit for a variety of functions.
// Means just one register per socket?
// See 'Additional IMC Performance Monitoring' in 'Intel Xeon Processor Scalable
// Memory Family Uncore Performance Monitoring Reference Manual'

static uint cpu_count;
static topology_t tp;

state_t ifreq_intel63_init(topology_t *_tp)
{
	state_t s;
	int cpu;

	// Static data
	if (tp.cpus == NULL) {
		if(xtate_fail(s, topology_select(_tp, &tp, TPSelect.socket, TPGroup.merge, 0))) {
			return s;
		}
	}

	if (cpu_count == 0) {
		cpu_count = tp.cpu_count;
	}

	// Opening MSR
	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		if (xtate_fail(s, msr_open(tp.cpus[cpu].id))) {
			return s;
		}
	}

	return EAR_SUCCESS;
}

state_t ifreq_intel63_dispose()
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

static state_t ifreq_intel63_is_enabled(int *enabled)
{
	ulong command;
	state_t s;
	int cpu;

	*enabled = 1;

	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		if (xtate_fail(s, msr_read(tp.cpus[cpu].id, &command, sizeof(ulong), UBOX_CTL_OFS))) {
			*enabled = 0;
			return s;
		}
		if (command != UBOX_CMD_STA) {
			*enabled = 0;
			return s;
		}
	}
	return EAR_SUCCESS;
}

static state_t ifreq_intel63_enable()
{
	ulong command = UBOX_CMD_STA;
	state_t s;
	int cpu;

	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		if (xtate_fail(s, msr_write(tp.cpus[cpu].id, &command, sizeof(ulong), UBOX_CTL_OFS))) {
			return s;
		}
	}

	return EAR_SUCCESS;
}

state_t ifreq_intel63_read(freq_imc_t *ef)
{
	imc_intel63_t *a = ef->data;
	int enabled;
	state_t s;
	int cpu;

	if (xtate_fail(s, ifreq_intel63_is_enabled(&enabled))) {
		return s;
	}
	if (!enabled) {
		if (xtate_fail(s, ifreq_intel63_enable())) {
			return s;
		}
	}

	timestamp_getfast(&a[0].time);

	for (cpu = 0; cpu < cpu_count; ++cpu) {
		if ((s = msr_read(tp.cpus[cpu].id, &a[cpu].freq, sizeof(ulong), UBOX_CTR_OFS)) != EAR_SUCCESS) {
			return s;
		}
		debug(0, "msr_read returned %lu\n", a[cpu].freq);
		a[cpu].error = state_fail(s);
	}

	return EAR_SUCCESS;
}

state_t ifreq_intel63_read_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average)
{
	state_t s;

	if (xtate_fail(s, ifreq_intel63_read(ef2))) {
		return s;
	}

	return ifreq_intel63_data_diff(ef2, ef1, freqs, average);
}

state_t ifreq_intel63_read_copy(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average)
{
	state_t s;

	if (xtate_fail(s, ifreq_intel63_read_diff(ef2, ef1, freqs, average))) {
		return s;
	}

	return ifreq_intel63_data_copy(ef1, ef2);
}

state_t ifreq_intel63_data_alloc(freq_imc_t *ef, ulong *freqs[], ulong *freqs_count)
{
	size_t size = sizeof(imc_intel63_t);
	imc_intel63_t *a;
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
	a = (imc_intel63_t *) ef->data;

	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		if (tp.cpus[cpu].freq_base == 0) {
			tp.cpus[cpu].freq_base = 100LU;
		}
		a[cpu].error = 1;		
	}

	return EAR_SUCCESS;
}

state_t ifreq_intel63_data_count(uint *count)
{
	*count = cpu_count;
	return EAR_SUCCESS;
}

state_t ifreq_intel63_data_copy(freq_imc_t *ef_dst, freq_imc_t *ef_src)
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
	memcpy(ef_dst->data, ef_src->data, sizeof(imc_intel63_t) * cpu_count);

	return EAR_SUCCESS;
}

state_t ifreq_intel63_data_free(freq_imc_t *ef, ulong *freqs[])
{
	imc_intel63_t *d = ef->data;

	if (ef == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (freqs != NULL && *freqs != NULL) {
		free(*freqs);
		*freqs = NULL;
	}
	free(d);
	ef = NULL;

	return EAR_SUCCESS;
}

state_t ifreq_intel63_data_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average)
{
	imc_intel63_t *d2 = ef2->data;
	imc_intel63_t *d1 = ef1->data;
	ulong valid_count = 0;
	ulong time;
	ulong freq;
	int cpu;

	if (ef2 == NULL || ef1 == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (ef2->data == NULL || ef1->data == NULL) {
		return_msg(EAR_NOT_INITIALIZED, Generr.input_uninitialized);
	}

	//
	if (average != NULL) {
		*average = 0LU;
	}

	//
	time = (ulong) timestamp_diff(&d2[0].time, &d1[0].time, TIME_MSECS);

	if (time < 1LU) {
		
	}

	//
	for (cpu = 0; cpu < cpu_count; ++cpu)
	{
		if (freqs != NULL) {
			freqs[cpu] = 0LU;
		}
		if (d2[cpu].error || d1[cpu].error) {
			continue;
		}
		//
		valid_count += 1;
		//
		freq = (d2[cpu].freq - d1[cpu].freq) / time;
		//
		if (freqs != NULL) {
			freqs[cpu] = freq;
		}
		if (average != NULL) {
			*average += freq;
		}
	}
	printf("\n");

	if (average != NULL) {
		*average = *average / valid_count;
	}

	return EAR_SUCCESS;
}

state_t ifreq_intel63_data_print(ulong *freqs, ulong *average)
{
	double freq_ghz;
	int i;

	printf("RAM:");
	for (i = 0; freqs != NULL && i < cpu_count; ++i) {
		freq_ghz = ((double) freqs[i]) / 1000000.0;
		printf(" %0.1lf", freq_ghz);
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

#if 0
#define U_MSR_UNCORE_RATIO_LIMIT	0x000620
#define U_MSR_UNCORE_RL_MASK_MAX	0x00007F
#define U_MSR_UNCORE_RL_MASK_MIN	0x007F00

state_t frequency_uncore_set_limits(uint32_t *buffer)
{
	uint64_t set0 = 0;
	uint64_t set1 = 0;
	state_t r;
	int i, j;

	if (!_init) {
		return EAR_NOT_INITIALIZED;
	}

	for (i = 0, j = 0; i < _cpus_num; ++i, j += 2)
	{
		set0 = (buffer[j+0] << 8) & U_MSR_UNCORE_RL_MASK_MIN;
		set1 = (buffer[j+1] << 0) & U_MSR_UNCORE_RL_MASK_MAX;
		set0 = set0 | set1;

		if ((r = msr_write(&_fds[i], &set0, sizeof(uint64_t), _offurl)) != EAR_SUCCESS) {
			return r;
		}
	}

	return EAR_SUCCESS;
}

state_t frequency_uncore_get_limits(uint32_t *buffer)
{
	uint64_t result0 = 0;
	uint64_t result1 = 0;
	state_t r;
	int i, j;

	if (!_init) {
		return EAR_NOT_INITIALIZED;
	}

	for (i = 0, j = 0; i < _cpus_num; ++i, j += 2)
	{
		buffer[j+0] = 0;
		buffer[j+1] = 0;

		// Read
		if ((r = msr_read(&_fds[i], &result0, sizeof(uint64_t), _offurl)) != EAR_SUCCESS) {
			return r;
		}

		result1 = (result0 & U_MSR_UNCORE_RL_MASK_MAX) >> 0;
		result0 = (result0 & U_MSR_UNCORE_RL_MASK_MIN) >> 8;

		buffer[j+0] = (uint32_t) result0;
		buffer[j+1] = (uint32_t) result1;
		result0 = 0;
		result1 = 0;
	}

	return EAR_SUCCESS;
}
#endif
