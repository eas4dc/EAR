/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/version.h>
// #define SHOW_DEBUGS 1
#include <common/config.h>

#ifdef EAR_CPUPOWER
#include <common/hardware/cpupower.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#include <cpupower.h>
#else
#include <cpufreq.h>
#endif
#endif

#include <common/states.h>
#include <common/types/generic.h>
#include <common/output/verbose.h>
#include <common/hardware/frequency.h>
#include <common/hardware/hardware_info.h>

#ifndef EAR_CPUPOWER
static struct cpufreq_policy previous_cpu0_policy;
#else
static governor_t previous_cpu0_policy;
#endif
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
		// status = cpufreq_cpu_exists(i);
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
			#ifdef EAR_CPUPOWER
			freqs[i] = CPUfreq_get(i);
			#else
			freqs[i] = cpufreq_get(i);
			#endif

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
	#ifndef EAR_CPUPOWER
	struct cpufreq_available_frequencies *list, *first;
	#else
	unsigned long *list;
	#endif
	ulong *pointer;
	int i;

	// Kernel alloc
	#ifndef EAR_CPUPOWER	
	list = cpufreq_get_available_frequencies(0);
	first = list;

	if (list == NULL) {
		error("unable to find an available frequencies list");
	}

	while(list != NULL)
	{
		list = list->next;
		num_freqs++;
	}
	#else
	list=CPUfreq_get_available_frequencies(0,&num_freqs);
	#endif

	verbose(VMETRICS, "%lu frequencies available ", num_freqs); // 2

	//
	pointer = (ulong *) malloc(sizeof(ulong) * num_freqs);

	if (pointer == NULL) {
		error("ERROR while allocating memory, exiting");
		exit(1);
	}

	//
	#ifndef EAR_CPUPOWER
	list = first;
	i = 0;

	while(list!=NULL)
	{
		pointer[i] = list->frequency;
		list = list->next;
		i++;
	}

	// Kernel dealloc
	cpufreq_put_available_frequencies(first);
	#else
	memcpy(pointer,list,sizeof(ulong)*num_freqs);
	CPUfreq_put_available_frequencies(list);
	#endif
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
	verbose(VMETRICS, "nominal frequency is %lu (KHz)", freq_nom);

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
ulong frequency_set_all_cpus(ulong freq)
{
	int result, i = 0;

	verbose(VMETRICS, "setting all cpus to %lu KHz", freq);
	if (is_valid_frequency(freq))
	{

		for (i = 0; i < num_cpus; i++)
		{
			freq_list_cpu[i] = freq;

			// This is a privileged function
			#ifndef EAR_CPUPOWER
			result = cpufreq_set_frequency(i, freq);
			#else
			result=CPUfreq_set_frequency(i,freq);
			#endif

			if (result < 0) {
				//verbose(0, "ERROR while switching cpu %d frequency to %lu (%s)", i, freq, strerror(-result));
				error( "ERROR while switching cpu %d frequency to %lu ", i, freq);
			}
		}

		return freq;
	}
	return freq_list_cpu[0];
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
	#ifndef EAR_CPUPOWER
	freq_list_cpu[cpu] = cpufreq_get(cpu);
	#else
	freq_list_cpu[cpu] = CPUfreq_get(cpu);
	#endif

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

// ear_get_freq
ulong frequency_pstate_to_freq(uint pstate)
{
	if (pstate >= num_freqs) {
		error("higher P_STATE (%u) than the maximum (%lu), returning nominal", pstate, num_freqs);
		return freq_list_rank[1];
	}
	return freq_list_rank[pstate];
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

	verbose(VMETRICS, "the P_STATE of the frequency %lu is %d", freq, i);

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
				verbose(1,"selecting closest frequency to %lu=%lu",freq,freq_list_rank[i-1]);
				*ps=i-1;
				return EAR_SUCCESS;	
			}
  	}
	}else{
		error("Frequency %lu out of range for this architecture",freq);
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
		if (freq>freq_list_rank[i]) return freq_list_rank[i];
	}
	return freq_list_rank[num_freqs-1];
	
}

uint frequency_closest_pstate(ulong freq)
{
	uint ps;
	if (freq_to_ps(freq,&ps)==EAR_SUCCESS){
		return ps;
	}else{
			error("Invalid frequency %lu, looking for closest frequency",freq);
			close_ps_to_freq(freq,&ps);
			return ps;
	}
}

// Privileged function
void frequency_set_performance_governor_all_cpus()
{
	int i;

	for (i = 0; i < num_cpus; i++) {
		#ifndef EAR_CPUPOWER
		cpufreq_modify_policy_governor(i, "performance");
		#else
		CPUfreq_set_cpufreq_governor(i,"performance");
		#endif
	}
}

// Privileged function
void frequency_set_ondemand_governor_all_cpus()
{
    int i;

    for (i = 0; i < num_cpus; i++) {
			#ifndef EAR_CPUPOWER
        cpufreq_modify_policy_governor(i, "ondemand");
			#else
				CPUfreq_set_cpufreq_governor(i,"ondemand");
			#endif
    }
}


// Privileged function
void frequency_set_userspace_governor_all_cpus()
{
	int i;

	for (i = 0; i < num_cpus; i++) {
	#ifndef EAR_CPUPOWER
		cpufreq_modify_policy_governor(i, "userspace");
	#else
		CPUfreq_set_cpufreq_governor(i,"userspace");
	#endif
	}
}

void frequency_save_previous_frequency()
{
	// Saving previous policy data
	previous_cpu0_freq = freq_list_cpu[0];
	
	verbose(VMETRICS, "previous frequency was set to %lu (KHz)", previous_cpu0_freq);
	
	saved_previous_freq = 1;
}

// Privileged function
void frequency_recover_previous_frequency()
{
	if (!saved_previous_freq) {
		verbose(VMETRICS, "previous frequency not saved");
		return;
	}

	frequency_set_all_cpus(previous_cpu0_freq);
	saved_previous_freq = 0;
}

void frequency_save_previous_policy()
{
	#ifndef EAR_CPUPOWER
	struct cpufreq_policy *policy;
	
	// Kernel alloc
	policy = cpufreq_get_policy(0);

	previous_cpu0_policy.min = policy->min;
	previous_cpu0_policy.max = policy->max;

	previous_cpu0_policy.governor = (char *) malloc(strlen(policy->governor) + 1);
	strcpy(previous_cpu0_policy.governor, policy->governor);
	verbose(VMETRICS, "previous policy governor was %s", policy->governor);
	#else
	governor_t policy;
	CPUfreq_get_policy(0,&policy);
	memcpy(&previous_cpu0_policy,&policy,sizeof(governor_t));
	verbose(VMETRICS, "previous policy governor was %s", policy.name);
	#endif


	// Kernel dealloc
	#ifndef EAR_CPUPOWER
	cpufreq_put_policy(policy);
	#endif
	saved_previous_policy = 1;
}

#ifndef EAR_CPUPOWER
void get_governor(struct cpufreq_policy *governor)
{
	struct cpufreq_policy *policy;	
    policy = cpufreq_get_policy(0);

    governor->min = policy->min;
    governor->max = policy->max;

    governor->governor = (char *) malloc(strlen(policy->governor) + 1);
    strcpy(governor->governor, policy->governor);
    cpufreq_put_policy(policy);
	
}
#else
void get_governor(governor_t *gov)
{
	CPUfreq_get_policy(0,gov);
}

#endif

#ifndef EAR_CPUPOWER
void set_governor(struct cpufreq_policy *governor)
{
	int status,i;
	for (i = 0; i < num_cpus; i++)
    {
        status = cpufreq_set_policy(i, governor);

        if (status < 0) {
			error( "ERROR while switching policy for cpu %d ", i);
		}
	}		
}
#else
void set_governor(governor_t *gov)
{
	int i;
	for (i = 0; i < num_cpus; i++){
		CPUfreq_set_policy(i,gov);
	}
}
#endif
void frequency_recover_previous_policy()
{
	int status, i;

	if (!saved_previous_policy) {
		verbose(VMETRICS, "previous policy not saved");
		return;
	}

	for (i = 0; i < num_cpus; i++)
	{
		#ifndef EAR_CPUPOWER
		status = cpufreq_set_policy(i, &previous_cpu0_policy);
		#else
		status=CPUfreq_set_policy(i,&previous_cpu0_policy);
		#endif	

		if (status < 0) {
			//verbose(0, "ERROR while switching policy for cpu %d (%s)", i, strerror(-status));
			error("ERROR while switching policy for cpu %d ", i);
		}
	}
	#ifndef EAR_CPUPOWER
	free(previous_cpu0_policy.governor);
	#endif
	saved_previous_policy = 0;
}
