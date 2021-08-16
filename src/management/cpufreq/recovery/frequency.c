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
#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/version.h>
#include <math.h>
//#define SHOW_DEBUGS 0
#include <common/states.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/output/verbose.h>
#include <common/hardware/hardware_info.h>
#include <management/cpufreq/recovery/frequency.h>

static governor_t previous_cpu0_policy;
static ulong previous_cpu0_freq;
static int saved_previous_policy;
static int saved_previous_freq;

static ulong *freq_list_rank; // List of frequencies of the whole rank (KHz)
static ulong *freq_list_cpu; // List of frequencies of each CPU (KHz)
static ulong freq_nom; // Nominal frequency (assuming CPU 0)
static ulong num_freqs;
static uint num_cpus;
static uint is_turbo_enabled;

//
static ulong *get_frequencies_cpu()
{
	ulong *freqs;
	int status, i;

	freqs = (ulong *) malloc(sizeof(ulong) * num_cpus);

	if (freqs == NULL) {
		error( "ERROR while allocating memory, exiting");
		exit(1);
	}

	for (i = 0; i < num_cpus; i++)
	{
		#if 0
		#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
		// Returns:
		// X -> if not
		// 0 -> if the specified CPU is present
		// status = cpucpufreq_exists(i);
		status = 0;
		#else
		// Returns:
		// 1 -> if CPU is online
		// 0 -> if CPU is offline
		status = !cpupower_is_cpu_online(cpu);
		#endif
		#endif
		freqs[i] = 0;

		// This is now hardcoded. It must be checked
		status=0;

		if (status == 0) {
			freqs[i] = CPUfreq_get(i);

			if (freqs[i] == 0) {
				error( "ERROR, CPU %d is online but the returned freq is 0", i);
			}
		} else {
			error( "ERROR, CPU %d is offline", i); // 4
		}
	}

	return freqs;
}

//
static ulong *get_frequencies_rank()
{
	unsigned long *list;
	ulong *pointer;
	int i;

	// Kernel alloc
	list=CPUfreq_get_available_frequencies(0,&num_freqs);

	debug("%lu frequencies available ", num_freqs); // 2

	//
	pointer = (ulong *) malloc(sizeof(ulong) * num_freqs);

	if (pointer == NULL) {
		error("ERROR while allocating memory, exiting");
		exit(1);
	}

	//
	memcpy(pointer,list,sizeof(ulong)*num_freqs);
	CPUfreq_put_available_frequencies(list);
	return pointer;
}

// ear_cpufreq_init
int frequency_init(unsigned int _num_cpus)
{
	num_cpus = _num_cpus;
	char driver[128],my_gov[128];
	CPUfreq_get_cpufreq_driver(0,driver);
	debug("Current cpufreq driver is %s",driver);
	if (strcmp(driver,"acpi-cpufreq")){
		error("Invalid cpufreq driver %s, please, install %s",driver,"acpi-cpufreq");
		exit(0);
	}

	CPUfreq_get_cpufreq_governor(0,my_gov);
	debug("Current cpufreq governor %s",my_gov);
	//
	freq_list_cpu = get_frequencies_cpu();

	//
	freq_list_rank = get_frequencies_rank();

	// Saving nominal freq = 1, because 0 is the turbo mode
	is_turbo_enabled=is_cpu_boost_enabled();	

	if (is_turbo_enabled)	freq_nom = freq_list_rank[1];
	else freq_nom = freq_list_rank[0];
	debug("nominal frequency is %lu (KHz)", freq_nom);

	return EAR_SUCCESS;
}

// ear_cpufreq_end
void frequency_dispose()
{
	free(freq_list_rank);
	free(freq_list_cpu);
}

int is_valid_frequency(ulong freq)
{
	int i = 0;

	while((i < num_freqs) && (freq_list_rank[i] != freq)) {
		i++;
	}

	if (i < num_freqs) return 1;
	else return 0;
}

int frequency_is_valid_frequency(ulong freq)
{
	return is_valid_frequency(freq);
}

int frequency_is_valid_pstate(uint pstate)
{
	if (pstate<num_freqs) return 1;
	else return 0;
}

uint frequency_get_num_online_cpus()
{
	return num_cpus;
}

// Privileged function
ulong frequency_set_cpu(ulong freq_khz, uint cpu)
{
	return 0LU;
}

ulong frequency_set_all_cpus(ulong freq)
{
	int result, i = 0;

	debug("setting all cpus to %lu KHz", freq);
	if (is_valid_frequency(freq))
	{

		for (i = 0; i < num_cpus; i++)
		{
			freq_list_cpu[i] = freq;

			// This is a privileged function
			result=CPUfreq_set_frequency(i,freq);

			if (result < 0) {
				//verbose(0, "ERROR while switching cpu %d frequency to %lu (%s)", i, freq, strerror(-result));
				error( "ERROR while switching cpu %d frequency to %lu ", i, freq);
			}
		}

		return freq;
	}
	return freq_list_cpu[0];
}

ulong frequency_set_with_mask(cpu_set_t *mask,ulong freq)
{
  int result, i = 0;
  
  if (is_valid_frequency(freq))
  { 
    
    for (i = 0; i < num_cpus; i++)
    { 
			if (CPU_ISSET(i,mask)){
				debug("setting cpu %d to freq %lu",i,freq);
      	freq_list_cpu[i] = freq;
      	// This is a privileged function
      	result=CPUfreq_set_frequency(i,freq);
     		if (result < 0) {
     		 	error( "ERROR while switching cpu %d frequency to %lu ", i, freq);
     		}
			}
    }
  	return freq;
  }
  return freq_list_cpu[0];
     
}

/* Returns 0 in case of some error and the last CPUF set when success */
ulong frequency_set_with_list(uint cpus,ulong *cpuf)
{
	int result=0, i = 0;
	if (cpuf == NULL) return 0;
	if (cpus > num_cpus) return 0;
	for (i = 0; i < cpus; i++)
  {
		if (cpuf[i] != 0){
		if (is_valid_frequency(cpuf[i])){
			debug("setting cpu %d to freq %lu",i,cpuf[i]);
			freq_list_cpu[i] = cpuf[i];
			result = CPUfreq_set_frequency(i,cpuf[i]);
			if (result < 0 ){
				error("ERROR while switching cpu %d frequency to %lu ", i,cpuf[i]);
			}
		}
		}
	}
	if (result < 0 ) return 0;
	return (ulong)result;
}
//
uint frequency_get_num_freqs()
{
	return num_freqs;
}

// ear_get_num_p_states
uint frequency_get_num_pstates()
{
	return num_freqs;
}

// ear_cpufreq_get
ulong frequency_get_cpu_freq(uint cpu)
{
	if (cpu > num_cpus) {
		return 0;
	}

	// Kernel asking (not hardware)
	freq_list_cpu[cpu] = CPUfreq_get(cpu);

	return freq_list_cpu[cpu];
}

// ear_get_nominal_frequency
ulong frequency_get_nominal_freq()
{
	return freq_nom;
}

// return the nominal pstate
ulong frequency_get_nominal_pstate()
{
	if (is_turbo_enabled) return 1;
	else return 0;
}

// ear_get_pstate_list
ulong *frequency_get_freq_rank_list()
{
	return freq_list_rank;
}

ulong frequency_get_cpufreq_list(uint cpus,ulong *cpuf)
{
	int i;
	memset(cpuf,0,sizeof(ulong)*cpus);
  if (cpus > num_cpus) {
    return 0;
  }
	for (i=0;i< cpus;i++) cpuf[i] = CPUfreq_get(i);
	return 0;
}


// ear_get_freq
ulong frequency_pstate_to_freq(uint pstate)
{
	if (pstate >= num_freqs) {
		debug("higher P_STATE (%u) than the maximum (%lu), returning nominal", pstate, num_freqs);
		return freq_list_rank[1];
	}
	return freq_list_rank[pstate];
}
ulong frequency_pstate_to_freq_list(uint pstate,ulong *flist,uint np)
{
	debug("Looking for pstate %u",pstate);
  if (pstate >= np) {
    return flist[1];
  }
  return flist[pstate];
}


// ear_get_pstate
uint frequency_freq_to_pstate(ulong freq)
{
	int i = 0, found = 0;

	while ((i < num_freqs) && (found == 0))
	{
		if (freq_list_rank[i] != freq) i++;
		else found = 1;
	}

	return i;
}

uint frequency_freq_to_pstate_list(ulong freq,ulong *flist,uint np)
{
  int i = 0, found = 0;
	debug("Looking for %lu, nump %u",freq,np);
  while ((i < np) && (found == 0))
  {
		debug("pstate %u is %lu",i,flist[i]);	
    if (flist[i] != freq) i++;
    else found = 1;
  }
  return i;
}


uint freq_to_ps(ulong freq,uint *ps)
{
	uint p=frequency_freq_to_pstate(freq);
	if (frequency_is_valid_pstate(p)){
		*ps=p;
		return EAR_SUCCESS;
	}else{
		return EAR_ERROR;
	}
}

uint close_ps_to_freq(ulong freq,uint *ps)
{
  int i=0;
	if ((freq<freq_list_rank[0]) && (freq>freq_list_rank[num_freqs-1])){
		i=0;
  	while (i < num_freqs)
  	{
  	  if (freq_list_rank[i] > freq){ i++;
  	  }else{
				debug("selecting closest frequency to %lu=%lu",freq,freq_list_rank[i-1]);
				*ps=i-1;
				return EAR_SUCCESS;	
			}
  	}
	}else{
		debug("Frequency %lu out of range for this architecture",freq);
		*ps=1;
		return EAR_ERROR;
	}
	return EAR_SUCCESS;

}

ulong frequency_closest_frequency(ulong freq)
{
	int i=0;
	if (freq>freq_list_rank[0]) return freq_list_rank[1];
	if (freq<freq_list_rank[num_freqs-1]) return freq_list_rank[num_freqs-1];
	for (i=0;i<num_freqs;i++){
		if (freq>=freq_list_rank[i]) return freq_list_rank[i];
	}
	return freq_list_rank[num_freqs-1];
	
}

ulong frequency_closest_high_freq(ulong freq,int minps)
{
  int i=0;
	ulong newf;
	float ff;
	ff=roundf((float)freq/100000.0);
	newf=(ulong)((ff/10.0)*1000000);
  if (newf>freq_list_rank[minps]) return freq_list_rank[minps];
	return frequency_closest_frequency(newf);
}


uint frequency_closest_pstate(ulong freq)
{
	uint ps;
	if (freq_to_ps(freq,&ps)==EAR_SUCCESS){
		return ps;
	}else{
			debug("Invalid frequency %lu, looking for closest frequency",freq);
			close_ps_to_freq(freq,&ps);
			return ps;
	}
}

// Privileged function
void frequency_set_performance_governor_all_cpus()
{
	int i;

	for (i = 0; i < num_cpus; i++) {
		CPUfreq_set_cpufreq_governor(i,"performance");
	}
}

// Privileged function
void frequency_set_ondemand_governor_all_cpus()
{
    int i;

    for (i = 0; i < num_cpus; i++) {
				CPUfreq_set_cpufreq_governor(i,"ondemand");
    }
}


// Privileged function
void frequency_set_userspace_governor_all_cpus()
{
	int i;

	for (i = 0; i < num_cpus; i++) {
		CPUfreq_set_cpufreq_governor(i,"userspace");
	}
}

void frequency_save_previous_frequency()
{
	// Saving previous policy data
	previous_cpu0_freq = freq_list_cpu[0];
	
	debug("previous frequency was set to %lu (KHz)", previous_cpu0_freq);
	
	saved_previous_freq = 1;
}

// Privileged function
void frequency_recover_previous_frequency()
{
	if (!saved_previous_freq) {
		debug("previous frequency not saved");
		return;
	}

	frequency_set_all_cpus(previous_cpu0_freq);
	saved_previous_freq = 0;
}

void frequency_save_previous_policy()
{
	governor_t policy;
	CPUfreq_get_policy(0,&policy);
	memcpy(&previous_cpu0_policy,&policy,sizeof(governor_t));
	debug("previous policy governor was %s", policy.name);


	// Kernel dealloc
	saved_previous_policy = 1;
}

void get_governor(governor_t *gov)
{
	CPUfreq_get_policy(0,gov);
}


void set_governor(governor_t *gov)
{
	int i;
	for (i = 0; i < num_cpus; i++){
		CPUfreq_set_policy(i,gov);
	}
}
void frequency_recover_previous_policy()
{
	int status, i;

	if (!saved_previous_policy) {
		debug("previous policy not saved");
		return;
	}

	for (i = 0; i < num_cpus; i++)
	{
		status=CPUfreq_set_policy(i,&previous_cpu0_policy);

		if (status < 0) {
			//verbose(0, "ERROR while switching policy for cpu %d (%s)", i, strerror(-status));
			error("ERROR while switching policy for cpu %d ", i);
		}
	}
	saved_previous_policy = 0;
}
