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

#include <common/math_operations.h>
#include <common/output/debug.h>
#include <metrics/bandwidth/archs/amd17.h>
#include <metrics/common/msr.h>
#include <stdlib.h>

static topology_t tp;
static uint       devs_count;
static off_t      ctl; // Address
static off_t      ctr; // Address
static ullong     cmd; // Value

BWIDTH_F_LOAD(bwidth_amd17_load)
{
    if (tpo->vendor != VENDOR_AMD || tpo->family < FAMILY_ZEN) {
        return_msg(, Generr.api_incompatible);
    }
    if (state_fail(msr_test(tpo, MSR_WR))) {
        return;
    }
    // We are selecting CCX by selecting a chunk of L3.
    if (state_fail(topology_select(tpo, &tp, TPSelect.l3, TPGroup.merge, 0))) {
        return;
    }
    // ZEN3/4/5. In theory is compatible because the small bit changes between
    // ZEN3/4 and ZEN5 are not used.
    if (tpo->family >= FAMILY_ZEN3) {
        //  cmd = 0x0300c0000040ff04;
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
    devs_count = tp.cpu_count;
    //
    apis_put(ops->get_info, bwidth_amd17_get_info);
    apis_put(ops->init,     bwidth_amd17_init);
    apis_put(ops->dispose,  bwidth_amd17_dispose);
    apis_put(ops->read,     bwidth_amd17_read);
    debug("Loaded AMD17");
}

BWIDTH_F_GET_INFO(bwidth_amd17_get_info)
{
    info->api         = API_AMD17;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_L3_SLICE;
    info->devs_count  = devs_count + 1;
}

BWIDTH_F_INIT(bwidth_amd17_init)
{
    state_t s;
    int i;
    for (i = 0; i < tp.cpu_count; ++i) {
        if (state_fail(s = msr_open(tp.cpus[i].id, MSR_WR))) {
            return s;
        }
        msr_write(tp.cpus[i].id, (void *) &cmd, sizeof(ullong), ctl);
    }
    return EAR_SUCCESS;
}

BWIDTH_F_DISPOSE(bwidth_amd17_dispose)
{
    return EAR_SUCCESS;
}

BWIDTH_F_READ(bwidth_amd17_read)
{
    int cpu;
    timestamp_get(&bws[devs_count].time);
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        msr_read(tp.cpus[cpu].id, &bws[cpu].cas, sizeof(ullong), ctr);
        // 48 bits for all counters from ZEN to ZEN3 (bit 48 is overflow)
        bws[cpu].cas = bws[cpu].cas & MAXBITS48;
        debug("CPU%03d: %014llu cas (REG 0x%lx)", tp.cpus[cpu].id, bws[cpu].cas, ctr);
    }
    return EAR_SUCCESS;
}