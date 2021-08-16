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

#ifndef METRICS_FREQUENCY_CPU_INTEL63_H
#define METRICS_FREQUENCY_CPU_INTEL63_H

#include <metrics/frequency/cpu.h>

state_t cpufreq_intel63_status(topology_t *tp);

state_t cpufreq_intel63_init();

state_t cpufreq_intel63_dispose();

state_t cpufreq_intel63_read(cpufreq_t *f);

state_t cpufreq_intel63_read_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

state_t cpufreq_intel63_read_copy(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

state_t cpufreq_intel63_data_alloc(cpufreq_t *f, ulong *freqs[]);

state_t cpufreq_intel63_data_count(uint *cpufreq_size, uint *freqs_count);

state_t cpufreq_intel63_data_copy(cpufreq_t *f_dst, cpufreq_t *f_src);

state_t cpufreq_intel63_data_free(cpufreq_t *f, ulong *freqs[]);

state_t cpufreq_intel63_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

state_t cpufreq_intel63_data_print(ulong *freqs, ulong *average);

#endif //METRICS_FREQUENCY_CPU_INTEL63_H
