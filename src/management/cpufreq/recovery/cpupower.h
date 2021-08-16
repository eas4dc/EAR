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

#ifndef _CPUPOWER_H_
#define _CPUPOWER_H_

#define GOVERNOR_MAX_NAME_LEN 128

typedef struct governor{
	char name[GOVERNOR_MAX_NAME_LEN];
	unsigned long max_f,min_f;
}governor_t;

/** */
unsigned long CPUfreq_get_cpufreq_driver(int cpu,char *src);
unsigned long CPUfreq_get_cpufreq_governor(int cpu,char *src);
unsigned long CPUfreq_get_cpufreq_maxf(int cpu);
unsigned long CPUfreq_get_cpufreq_minf(int cpu);
void CPUfreq_set_cpufreq_governor(int cpu,char *name);
unsigned long CPUfreq_set_cpufreq_minf(int cpu,unsigned long minf);
unsigned long CPUfreq_set_cpufreq_maxf(int cpu,unsigned long maxf);
/** Memory is allocated to save the list,CPUfreq_put_available_governors must be used to release the memory,num_freqs es the num of pstates */
unsigned long *CPUfreq_get_available_frequencies(int cpu,unsigned long *num_freqs);
void CPUfreq_put_available_frequencies(unsigned long *f);
unsigned long CPUfreq_get_num_pstates(int cpu);
/** Memory is allocated to save the list of governos, CPUfreq_put_available_governors must be used to release the memory */
char **CPUfreq_get_available_governors(int cpu,unsigned long *num_governors);
void CPUfreq_put_available_governors(char **glist);
/** */
void CPUfreq_get_policy(int cpu,governor_t *g);
int CPUfreq_set_policy(int cpu,governor_t *g);
/** */
unsigned long CPUfreq_get(int cpu_num);
unsigned long CPUfreq_set_frequency(unsigned int cpu, unsigned long f);

#endif
