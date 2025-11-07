/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <metrics/cpufreq/archs/dummy.h>
#include <metrics/cpufreq/cpufreq_base.h>
#include <stdlib.h>

static uint cpu_count;
static cfb_t bf;

state_t cpufreq_dummy_status(topology_t *tp, cpufreq_ops_t *ops)
{
    cpu_count = tp->cpu_count;
    // Getting base frequency and boost
    cpufreq_base_init(tp, &bf);
    debug("cpufreq_dummy_status base freq %lu", bf.frequency);

    replace_ops(ops->init, cpufreq_dummy_init);
    replace_ops(ops->dispose, cpufreq_dummy_dispose);
    replace_ops(ops->count_devices, cpufreq_dummy_count_devices);
    replace_ops(ops->read, cpufreq_dummy_read);
    replace_ops(ops->data_diff, cpufreq_dummy_data_diff);
    return EAR_SUCCESS;
}

state_t cpufreq_dummy_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t cpufreq_dummy_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t cpufreq_dummy_count_devices(ctx_t *c, uint *cpu_count_in)
{
    *cpu_count_in = cpu_count;
    return EAR_SUCCESS;
}

state_t cpufreq_dummy_read(ctx_t *c, cpufreq_t *f)
{
    int cpu;
    debug("cpufreq_dummy_readi cpus=%d", cpu_count);
    for (cpu = 0; cpu < cpu_count; ++cpu) {
        f[cpu].freq_aperf = bf.frequency;
        f[cpu].freq_mperf = bf.frequency;
        f[cpu].error      = 0;
    }
    return EAR_SUCCESS;
}

state_t cpufreq_dummy_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
    uint c;
    if (freqs != NULL) {
        memset(freqs, 0, cpu_count * sizeof(ulong));
        for (c = 0; c < cpu_count; c++) {
            freqs[c] = bf.frequency;
        }
    }
    if (average != NULL) {
        *average = bf.frequency;
    }
    debug("Average cpufreq %lu", *average);
    return EAR_SUCCESS;
}
