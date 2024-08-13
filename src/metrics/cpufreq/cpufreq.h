/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_CPUFREQ_H
#define METRICS_CPUFREQ_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>
#include <metrics/cpufreq/cpufreq_base.h>

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

state_t cpufreq_load(topology_t *tp, int force_api);

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

void cpufreq_data_print(ulong *freqs, ulong average, int fd);

char *cpufreq_data_tostr(ulong *freqs, ulong average, char *buffer, size_t length);

#endif //METRICS_CPUFREQ_H