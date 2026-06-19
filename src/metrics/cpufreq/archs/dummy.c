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

static uint cpus_count;

CPUFREQ_F_LOAD(dummy)
{
    cpus_count = tp->cpu_count;
    apis_put(ops->unload, cpufreq_dummy_unload);
    apis_put(ops->get_info, cpufreq_dummy_get_info);
    apis_put(ops->read, cpufreq_dummy_read);
    debug("loaded dummy");
}

CPUFREQ_F_UNLOAD(dummy)
{
}

CPUFREQ_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_CPU;
    info->devs_count  = cpus_count;
}

CPUFREQ_F_READ(dummy)
{
    int cpu;
    for (cpu = 0; cpu < cpus_count; ++cpu) {
        f[cpu].freq_aperf = 0;
        f[cpu].freq_mperf = 0;
        f[cpu].state      = 2; // DUMMY state, set base frequency
    }
    return EAR_SUCCESS;
}
