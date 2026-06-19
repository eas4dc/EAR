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
#include <common/sizes.h>
#include <errno.h>
#include <fcntl.h>
#include <metrics/common/msr.h>
#include <metrics/cpufreq/archs/intel63.h>
#include <metrics/cpufreq/cpufreq_base.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MSR_IA32_APERF 0x000000E8
#define MSR_IA32_MPERF 0x000000E7

static topology_t tp_own;

static void close_msrs(int max_cpu)
{
    int cpu;
    for (cpu = 0; cpu < max_cpu; ++cpu) {
        msr_close(tp_own.cpus[cpu].id);
    }
}

CPUFREQ_F_LOAD(intel63)
{
    int cpu;

    if ((tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) ||
        (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN)) {
        // Compatible
    } else {
        return;
    }
    // Testing if MSR can be read
    if (state_fail(msr_test(tp, MSR_RD))) {
        return;
    }
    // Opening MSRs.
    for (cpu = 0; cpu < tp->cpu_count; ++cpu) {
        if (state_fail(msr_open(tp->cpus[cpu].id, MSR_RD))) {
            close_msrs(cpu);
            return;
        }
    }
    topology_copy(&tp_own, tp);
    // If reading MSR is allowed
    apis_put(ops->unload, cpufreq_intel63_unload);
    apis_put(ops->get_info, cpufreq_intel63_get_info);
    apis_put(ops->read, cpufreq_intel63_read);
    debug("loaded intel63");
}

CPUFREQ_F_UNLOAD(intel63)
{
    close_msrs(tp_own.cpu_count);
    topology_close(&tp_own);
}

CPUFREQ_F_GET_INFO(intel63)
{
    info->api         = API_INTEL63;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_CPU;
    info->devs_count  = tp_own.cpu_count;
}

CPUFREQ_F_READ(intel63)
{
    state_t s1, s2;
    int cpu;

    debug("cpufreq_intel63_read");
    for (cpu = 0; cpu < tp_own.cpu_count; ++cpu) {
        s1           = msr_read(tp_own.cpus[cpu].id, &f[cpu].freq_mperf, sizeof(ulong), MSR_IA32_MPERF);
        s2           = msr_read(tp_own.cpus[cpu].id, &f[cpu].freq_aperf, sizeof(ulong), MSR_IA32_APERF);
        f[cpu].state = (state_fail(s1) || state_fail(s2)); // Reading error, set 0
    }
    return EAR_SUCCESS;
}