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

#define _GNU_SOURCE
#include <math.h>
#include <sched.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <management/cpufreq/frequency.h>

static topology_t topo;
static ctx_t      c;
static pstate_t  *available_list;
static pstate_t  *current_list;
static uint      *index_list;
static uint       pstate_count;
static uint       pstate_nominal;
static uint       init;

static state_t static_init(ullong *freq_list, uint freq_count)
{
	state_t s;
	
	if (init) {
		return_msg(EAR_INITIALIZED, Generr.api_initialized);
	}
	if (xtate_fail(s, topology_init(&topo))) {
		return s;
	}
	if (xtate_fail(s, mgt_cpufreq_load(&topo))) {
		return s;
	}
	if (freq_list == NULL) {
		if (xtate_fail(s, mgt_cpufreq_init(&c))) {
			return s;
		}
	} else {
		if (xtate_fail(s, mgt_cpufreq_init_user(&c, freq_list, freq_count))) {
			return s;
		}
	}
	if (xtate_fail(s, mgt_cpufreq_count_available(&c, &pstate_count))) {
		return s;
	}
	//alloc
	if ((available_list = calloc(pstate_count, sizeof(pstate_t))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
	if ((current_list = calloc(topo.cpu_count, sizeof(pstate_t))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
	if ((index_list = calloc(topo.cpu_count, sizeof(uint))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
	//
	if (xtate_fail(s, mgt_cpufreq_get_available_list(&c, available_list))) {
		return s;
	}
	if (xtate_fail(s, mgt_cpufreq_get_nominal(&c, &pstate_nominal))) {
		return s;
	}
	init = 1;
	
	return EAR_SUCCESS;
}

state_t frequency_init_user(ulong *freq_list, uint freq_count)
{
	return static_init((ullong *) freq_list, freq_count);
}

state_t frequency_init(uint x)
{
	return static_init(NULL, 0);
}

state_t frequency_dispose()
{
	state_t s;

	if (!init) {
		return_msg(EAR_NOT_INITIALIZED, Generr.api_uninitialized);
	}
	if (xtate_fail(s, mgt_cpufreq_dispose(&c))) {
		return s;
	}
	if (xtate_fail(s, topology_close(&topo))) {
		return s;
	}
	free(available_list);
	free(current_list);
	free(index_list);
	init = 0;

	return EAR_SUCCESS;
}

uint frequency_get_num_pstates()
{
	if (!init) {
		return 0;
	}
	// Returns the number of P_STATEs.
	return pstate_count;
}

ulong frequency_get_cpu_freq(uint cpu)
{
	if (!init) {
		return 0LU;
	}
	if (cpu >= topo.cpu_count) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_get_current_list(&c, current_list))) {
		return 0LU;
	}
	// Returns the current frequency given a CPU in KHz.
	return (ulong) current_list[cpu].khz;
}

ulong frequency_get_cpufreq_list(uint cpu_count, ulong *freq_list)
{
	int i;
	memset(freq_list, 0, sizeof(ulong)*cpu_count);
	if (cpu_count > topo.cpu_count) {
		return 0;
	}
	if (state_fail(mgt_cpufreq_get_current_list(&c, current_list))) {
		return 0LU;
	}
	for (i = 0; i < cpu_count; ++i) {
		freq_list[i] = (ulong) current_list[i].khz;
	}
	return 0;
}

ulong frequency_get_nominal_freq()
{
	if (!init) {
		return 0LU;
	}
	// Returns the nominal frequency in KHz.
	return (ulong) available_list[pstate_nominal].khz;
}

ulong frequency_get_nominal_pstate()
{
	if (!init) {
		return 0LU;
	}
	// Returns the nominal P_STATE.
	return (ulong) pstate_nominal;
}

ulong *frequency_get_freq_rank_list()
{
	static ulong *list_khz = NULL;
	int i;
	if (!init) {
		return NULL;
	}
	if (list_khz == NULL) {
		list_khz = malloc(sizeof(ulong)*pstate_count);
	}
	for (i = 0; i < pstate_count; ++i) {
		list_khz[i] = (ulong) available_list[i].khz;
	}
	// Returns a list of KHz of each CPU.
	return list_khz;
}

ulong frequency_set_cpu(ulong freq_khz, uint cpu)
{
	uint pstate_index;
	// Sets a frequency in KHz in all CPUs.
	if (!init) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 0))) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_set_current(&c, pstate_index, cpu))) {
		return 0LU;
	}
	// Returns written P_STATE.
	return freq_khz;
}

ulong frequency_set_all_cpus(ulong freq_khz)
{
	uint pstate_index;
	// Sets a frequency in KHz in all CPUs.
	if (!init) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 0))) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_set_current(&c, pstate_index, all_cpus))) {
		return 0LU;
	}
	// Returns written P_STATE.
	return freq_khz;
}

ulong frequency_set_with_mask(cpu_set_t *mask, ulong freq_khz)
{
	uint pstate_index;
	int i = 0;
  
	if (!init) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 0))) {
  		return current_list[0].khz;
	}
    for (i = 0; i < topo.cpu_count; i++)
    {
    	if (CPU_ISSET(i, mask)) {
			debug("setting CPU%d to P_STATE %u", i, pstate_index);
			if (state_fail(mgt_cpufreq_set_current(&c, pstate_index, i))) {
				error("while setting frequency of CPU%d to %lu", i, freq_khz);
			}
		}
	}
  	return freq_khz;
}

ulong frequency_set_with_list(uint x, ulong *list)
{
	uint pstate_index;
	state_t s1, s2;
	int cpu;

	if (list == NULL) {
		return 0LU;
	}
	for (cpu = 0; cpu < topo.cpu_count; ++cpu) {
		if (list[cpu] > 0){
			if (xtate_ok(s1, mgt_cpufreq_get_index(&c, (ullong) list[cpu], &pstate_index, 0))) {
				if (xtate_ok(s2, mgt_cpufreq_set_current(&c, pstate_index, cpu))) {
    	            if (cpu == 0)
    	              debug("frequency set to pstate %u", pstate_index); 
				}
			}
		}
	}
	if (state_fail(s1) || state_fail(s2)) {
		return 0LU;
	}
	return list[topo.cpu_count-1];
}

ulong frequency_pstate_to_freq(uint pstate_index)
{
	// Converts index to frequency.
	if (!init) {
		return 0LU;
	}
	if (pstate_index >= pstate_count) {
		return (ulong) available_list[pstate_nominal].khz;
	}
	// Returns the nominal frequency.
	return (ulong) available_list[pstate_index].khz;
}

uint frequency_freq_to_pstate(ulong freq_khz)
{
	uint pstate_index;
	if (!init) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 0))) {
		return pstate_count;
	}
	// Given a frequency in KHz returns a P_STATE index.
	return pstate_index;
}

ulong frequency_pstate_to_freq_list(uint pstate_index, ulong *list, uint pstate_count)
{
	// Given a frequency list and a P_STATE index, returns a frequency.
	if (pstate_index >= pstate_count) {
		return list[pstate_nominal];
	}
	return list[pstate_index];
}

uint frequency_freq_to_pstate_list(ulong freq_khz, ulong *list, uint pstate_count)
{
	int i = 0, found = 0;
	for (i = found = 0; i < pstate_count && !found; ++i) {
		found = (list[i] == freq_khz);
	}
	return i-found;
}

void frequency_set_performance_governor_all_cpus()
{
	mgt_cpufreq_set_governor(&c, Governor.performance);
}

void frequency_set_userspace_governor_all_cpus()
{
	mgt_cpufreq_set_governor(&c, Governor.userspace);
}

int frequency_is_valid_frequency(ulong freq_khz)
{
	uint pstate_index;
	if (!init) {
		return 0;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 0))) {
		return 0;
	}
	return 1;
}

int frequency_is_valid_pstate(uint pstate)
{
	if (!init) {
		return 0;
	}
	return pstate < pstate_count;
}

uint frequency_closest_pstate(ulong freq_khz)
{
	uint pstate_index;
	//fprintf(stderr, "frequency_closest_pstate, init %d, freq_khz %lu ...", init, freq_khz);
	if (!init) {
		return 1;
	}
	if (freq_khz < available_list[pstate_count-1].khz) {
		//fprintf(stderr, "leaving because is less than %lu\n", available_list[pstate_count-1].khz);
		return pstate_nominal;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 1))) {
		//fprintf(stderr, "leaving because mgt_cpufreq_get_index failed\n");
		return pstate_nominal;
	}
		//fprintf(stderr, "leaving because is ok, indes %d\n", pstate_index);
	return pstate_index;
}

ulong frequency_closest_frequency(ulong freq_khz)
{
	uint pstate_index;
	if (!init) {
		return 0LU;
	}
	if (state_fail(mgt_cpufreq_get_index(&c, (ullong) freq_khz, &pstate_index, 1))) {
		return available_list[pstate_count-1].khz;
	}
	return available_list[pstate_index].khz;
}

ulong frequency_closest_high_freq(ulong freq_khz, int pstate_min)
{
	ulong newf;
	float ff;

	ff = roundf((float) freq_khz / 100000.0);
	newf = (ulong) ((ff / 10.0) * 1000000);

	if (newf > (ulong) available_list[pstate_min].khz)
		return (ulong) available_list[pstate_min].khz;
	return frequency_closest_frequency(newf);
}

void get_governor(governor_t *_governor)
{
	uint governor;
	// Clear
	_governor->max_f = 0LU;
	_governor->min_f = 0LU;
	sprintf(_governor->name, Goverstr.other);
	//
	if (!init) {
		return;
	}
	if (state_fail(mgt_cpufreq_get_governor(&c, &governor))) {
		return;
	}
	if (state_fail(mgt_governor_tostr(governor, _governor->name))) {
		return;
	}
	_governor->max_f = available_list[0].khz;
	_governor->min_f = available_list[pstate_count-1].khz;
}

void set_governor(governor_t *_governor)
{
	uint governor;
	//
	if (!init) {
		return;
	}
	if (state_fail(mgt_governor_toint(_governor->name, &governor))) {
		return;
	}
	if (state_fail(mgt_cpufreq_set_governor(&c, governor))) {
		return;
	}
}
