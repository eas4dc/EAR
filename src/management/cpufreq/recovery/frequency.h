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

#ifndef EAR_CONTROL_FREQUENCY_H
#define EAR_CONTROL_FREQUENCY_H

#include <linux/version.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>
#ifndef EAR_CPUPOWER
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#include <cpupower.h>
#else
#include <cpufreq.h>
#endif
#else
#include <management/cpufreq/recovery/cpupower.h>
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
ulong frequency_get_cpufreq_list(uint cpus,ulong *cpuf);
// NEW
ulong frequency_set_cpu(ulong freq_khz, uint cpu);
ulong frequency_set_all_cpus(ulong freq);
ulong frequency_set_with_mask(cpu_set_t *mask,ulong freq);
ulong frequency_set_with_list(uint cpus,ulong *cpuf);
ulong frequency_pstate_to_freq(uint pstate);
ulong frequency_pstate_to_freq_list(uint pstate,ulong *flist,uint np);
uint frequency_freq_to_pstate(ulong freq);
uint frequency_freq_to_pstate_list(ulong freq,ulong *flist,uint np);
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
ulong frequency_closest_high_freq(ulong freq,int minps);


#ifndef EAR_CPUPOWER
void get_governor(struct cpufreq_policy *governor);
void set_governor(struct cpufreq_policy *governor);
#else
void get_governor(governor_t *governor);
void set_governor(governor_t *governor);
#endif

#endif //EAR_CONTROL_FREQUENCY_H
