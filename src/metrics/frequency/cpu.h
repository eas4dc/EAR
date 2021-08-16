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

#ifndef METRICS_FREQUENCY_CPU_H
#define METRICS_FREQUENCY_CPU_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

typedef ctx_t cpufreq_t;

state_t cpufreq_load(topology_t *tp);

state_t cpufreq_init();

state_t cpufreq_dispose();

// Privileged function.
state_t cpufreq_read(cpufreq_t *ef);

// Privileged function.
state_t cpufreq_read_diff(cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average);

// Privileged function.
state_t cpufreq_read_copy(cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average);

state_t cpufreq_data_alloc(cpufreq_t *ef, ulong *freqs[]);

state_t cpufreq_data_count(uint *cpufreq_size, uint *freqs_count);

state_t cpufreq_data_copy(cpufreq_t *ef_dst, cpufreq_t *ef_src);

state_t cpufreq_data_free(cpufreq_t *ef, ulong **freqs);

state_t cpufreq_data_diff(cpufreq_t *ef2, cpufreq_t *ef1, ulong *freqs, ulong *average);

state_t cpufreq_data_print(ulong *freqs, ulong *average);

#endif //EAR_PRIVATE_CACHE_H
