/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _APP_API_H
#define _APP_API_H

#define _GNU_SOURCE
#include <sched.h>

#define EAR_SUCCESS 0
#define EAR_ERROR   -1
#define EAR_TIMEOUT -21
/** returns the accumulated energy (mili joules) and time (miliseconds) */
int ear_energy(unsigned long *energy_mj, unsigned long *time_ms);
/** Given two measures of energy and time computes the difference */
void ear_energy_diff(unsigned long ebegin, unsigned long eend, unsigned long *ediff, unsigned long tbegin,
                     unsigned long tend, unsigned long *tdiff);

int ear_debug_energy(unsigned long *energy_j, unsigned long *energy_mj, unsigned long *time_sec, unsigned long *time_ms,
                     unsigned long *os_time_sec, unsigned long *os_time_ms);
/* Sets cpufreq in the CPU sets in mask */
int ear_set_cpufreq(cpu_set_t *mask, unsigned long cpufreq);
/* Sets gpufreq in a given gpu*/
int ear_set_gpufreq(int gpu_id, unsigned long gpufreq);
/* Sets gpufreq in all the GPUs */
int ear_set_gpufreq_list(int num_gpus, unsigned long *gpufreqlist);

int ear_connect();
void ear_disconnect();

#else
#endif
