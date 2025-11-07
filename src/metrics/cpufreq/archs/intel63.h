/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_CPUFREQ_PERF_H
#define METRICS_CPUFREQ_PERF_H

#include <metrics/cpufreq/cpufreq.h>

state_t cpufreq_intel63_status(topology_t *tp, cpufreq_ops_t *ops);

state_t cpufreq_intel63_init(ctx_t *c);

state_t cpufreq_intel63_dispose(ctx_t *c);

state_t cpufreq_intel63_count_devices(ctx_t *c, uint *cpu_count);

state_t cpufreq_intel63_read(ctx_t *c, cpufreq_t *f);

state_t cpufreq_intel63_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

#endif // METRICS_CPUFREQ_PERF_H
