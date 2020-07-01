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

#ifndef EAR_CONTROL_FREQUENCY_H
#define EAR_CONTROL_FREQUENCY_H
#include <linux/version.h>

#ifndef EAR_CPUPOWER
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#include <cpupower.h>
#else
#include <cpufreq.h>
#endif
#else
#include <common/hardware/cpupower.h>
#endif


int frequency_init(uint cpus);
void frequency_dispose();
uint frequency_get_num_freqs();
uint frequency_get_num_pstates();
uint frequency_get_num_online_cpus();
ulong frequency_get_cpu_freq(uint cpu);
ulong frequency_get_nominal_freq();
ulong frequency_get_nominal_pstate();
ulong *frequency_get_freq_rank_list();
ulong frequency_set_all_cpus(ulong freq);
ulong frequency_pstate_to_freq(uint pstate);
uint frequency_freq_to_pstate(ulong freq);
void frequency_set_performance_governor_all_cpus();
void frequency_set_ondemand_governor_all_cpus();
void frequency_set_userspace_governor_all_cpus();
void frequency_save_previous_frequency();
void frequency_save_previous_configuration();
void frequency_save_previous_policy();
void frequency_recover_previous_frequency();
void frequency_recover_previous_configuration();
void frequency_recover_previous_policy();
int frequency_is_valid_frequency(ulong freq);
int frequency_is_valid_pstate(uint pstate);
uint frequency_closest_pstate(ulong freq);
ulong frequency_closest_frequency(ulong freq);

#ifndef EAR_CPUPOWER
void get_governor(struct cpufreq_policy *governor);
void set_governor(struct cpufreq_policy *governor);
#else
void get_governor(governor_t *governor);
void set_governor(governor_t *governor);
#endif

#endif //EAR_CONTROL_FREQUENCY_H
