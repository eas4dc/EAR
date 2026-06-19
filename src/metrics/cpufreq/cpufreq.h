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

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/types.h>
#include <metrics/cpufreq/cpufreq_base.h>

typedef struct cpufreq_s {
    ulong freq_aperf;
    ulong freq_mperf;
    uint state;
} cpufreq_t;

typedef struct cpufreq_ops_s {
    void (*unload)();
    void (*get_info)(apinfo_t *info);
    state_t (*read)(cpufreq_t *f);
} cpufreq_ops_t;

// API building scheme
#define CPUFREQ_F_LOAD(name)     void cpufreq_##name##_load(topology_t *tp, cpufreq_ops_t *ops, int options)
#define CPUFREQ_F_UNLOAD(name)   void cpufreq_##name##_unload()
#define CPUFREQ_F_GET_INFO(name) void cpufreq_##name##_get_info(apinfo_t *info)
#define CPUFREQ_F_READ(name)     state_t cpufreq_##name##_read(cpufreq_t *f)

#define CPUFREQ_DEFINES(name)                                                                                          \
    CPUFREQ_F_LOAD(name);                                                                                              \
    CPUFREQ_F_UNLOAD(name);                                                                                            \
    CPUFREQ_F_GET_INFO(name);                                                                                          \
    CPUFREQ_F_READ(name);

void cpufreq_load(topology_t *tp, int options);

void cpufreq_unload();

void cpufreq_get_info(apinfo_t *info);

state_t cpufreq_read(cpufreq_t *ef);

state_t cpufreq_read_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

state_t cpufreq_read_copy(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

// Helpers
void cpufreq_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

void cpufreq_data_alloc(cpufreq_t **f, ulong **freqs);

void cpufreq_data_copy(cpufreq_t *dst, cpufreq_t *src);

void cpufreq_data_free(cpufreq_t **f, ulong **freqs);

void cpufreq_data_print(ulong *freqs, ulong average, int fd);

char *cpufreq_data_tostr(ulong *freqs, ulong average, char *buffer, size_t length);

#endif // METRICS_CPUFREQ_H