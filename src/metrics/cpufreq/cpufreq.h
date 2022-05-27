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

#ifndef METRICS_CPUFREQ_H
#define METRICS_CPUFREQ_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

typedef struct cpufreq_s
{
	ulong freq_aperf;
	ulong freq_mperf;
	uint error;
} cpufreq_t;

typedef struct cpufreq_ops_s {
	state_t (*init)          (ctx_t *c);
	state_t (*dispose)       (ctx_t *c);
	state_t (*count_devices) (ctx_t *c, uint *cpu_count);
	state_t (*read)          (ctx_t *c, cpufreq_t *f);
	state_t (*data_diff)     (cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);
} cpufreq_ops_t;

state_t cpufreq_load(topology_t *tp, int eard);

state_t cpufreq_get_api(uint *api);

state_t cpufreq_init(ctx_t *c);

state_t cpufreq_dispose(ctx_t *c);

state_t cpufreq_count_devices(ctx_t *c, uint *dev_count);

state_t cpufreq_read(ctx_t *c, cpufreq_t *ef);

state_t cpufreq_read_diff(ctx_t *c, cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

state_t cpufreq_read_copy(ctx_t *c, cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

// Helpers
state_t cpufreq_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

state_t cpufreq_data_alloc(cpufreq_t **f, ulong **freqs);

state_t cpufreq_data_copy(cpufreq_t *dst, cpufreq_t *src);

state_t cpufreq_data_free(cpufreq_t **f, ulong **freqs);

state_t cpufreq_data_print(ulong *freqs, ulong average, int fd);

#endif //METRICS_CPUFREQ_H
