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

#ifndef _APP_API_H
#define _APP_API_H

#define _GNU_SOURCE
#include <sched.h>

#define EAR_SUCCESS 0
#define EAR_ERROR -1
#define EAR_TIMEOUT -21
/** returns the accumulated energy (mili joules) and time (miliseconds) */
int ear_energy(unsigned long *energy_mj,unsigned long *time_ms);
/** Given two measures of energy and time computes the difference */
void ear_energy_diff(unsigned long ebegin,unsigned long eend, unsigned long *ediff, unsigned long tbegin, unsigned long tend, unsigned long *tdiff);

int ear_debug_energy(ulong *energy_j,ulong *energy_mj,ulong *time_sec,ulong *time_ms,ulong *os_time_sec,ulong *os_time_ms);
/* Sets cpufreq in the CPU sets in mask */
int ear_set_cpufreq(cpu_set_t *mask,unsigned long cpufreq);
/* Sets gpufreq in a given gpu*/
int ear_set_gpufreq(int gpu_id,unsigned long gpufreq);
/* Sets gpufreq in all the GPUs */
int ear_set_gpufreq_list(int num_gpus,unsigned long *gpufreqlist);

int ear_connect();
void ear_disconnect();

#else
#endif
