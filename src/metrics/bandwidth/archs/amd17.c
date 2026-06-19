/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/
/* clang-format off */

// #define SHOW_DEBUGS 1

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/msr.h>
#include <metrics/bandwidth/archs/amd17.h>

static topology_t tp_own;
static uint       devs_count;
static off_t      ctl; // Address
static off_t      ctr; // Address
static ullong     cmd; // Value

static void close_all(int max_cpu)
{
    int cpu;
    for (cpu = 0; cpu < max_cpu; ++cpu) {
        msr_close(tp_own.cpus[cpu].id);
    }
    topology_close(&tp_own);
}

BWIDTH_F_LOAD(amd17)
{
    int i;

    if (api_already_loaded(ops)) {
        return;
    }
    if (tp->vendor != VENDOR_AMD || tp->family < FAMILY_ZEN) {
        return_msg(, Generr.api_incompatible);
    }
    if (state_fail(msr_test(tp, MSR_WR))) {
        return;
    }
    // We are selecting CCX by selecting a chunk of L3.
    if (state_fail(topology_select(tp, &tp_own, TPSelect.l3, TPGroup.merge, 0))) {
        return;
    }
    // ZEN3/4/5. In theory is compatible because the small bit changes between
    // ZEN3/4 and ZEN5 are not used.
    if (tp_own.family >= FAMILY_ZEN3) {
        // cmd = 0x0300c0000040ff04;
        cmd = 0x0300c00000400104;
        ctl = 0xc0010230;
        ctr = 0xc0010231;
    } else { // ZEN+ZEN2
        cmd = 0xff0f000000400104;
        ctl = 0xc0010230;
        ctr = 0xc0010231;
    }
    // It seems that in ZEN2 there are two L3's chunks per CCD, one per CCX.
    // But in case of ZEN3, there is just one L3 chunk per CCD.
    for (i = 0; i < tp_own.cpu_count; ++i) {
        if (state_fail(msr_open(tp_own.cpus[i].id, MSR_WR))) {
            close_all(i);
            return;
        }
        msr_write(tp_own.cpus[i].id, (void *) &cmd, sizeof(ullong), ctl);
    }
    apis_put(ops->unload  , bwidth_amd17_unload);
    apis_put(ops->get_info, bwidth_amd17_get_info);
    apis_put(ops->read    , bwidth_amd17_read);
    debug("Loaded AMD17");
}

BWIDTH_F_UNLOAD(amd17)
{
    close_all(tp_own.cpu_count);
}

BWIDTH_F_GET_INFO(amd17)
{
    info->api         = API_AMD17;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_L3_SLICE;
    info->devs_count  = tp_own.cpu_count + 1;
}

BWIDTH_F_READ(amd17)
{
    int cpu;
    timestamp_get(&b[devs_count].time);
    for (cpu = 0; cpu < tp_own.cpu_count; ++cpu) {
        msr_read(tp_own.cpus[cpu].id, &b[cpu].cas, sizeof(ullong), ctr);
        // 48 bits for all counters from ZEN to ZEN3 (bit 48 is overflow)
        b[cpu].cas = b[cpu].cas & MAXBITS48;
        debug("CPU%03d: %014llu cas (REG 0x%lx)", tp.cpus[cpu].id, b[cpu].cas, ctr);
    }
    return EAR_SUCCESS;
}
