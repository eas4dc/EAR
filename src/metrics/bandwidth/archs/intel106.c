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
#include <fcntl.h>
#include <metrics/bandwidth/archs/intel106.h>
#include <metrics/common/pci.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO: have an ICE LAKE node and test it.
static ushort ice_ids[2] = {0x3451, 0x00};
static char *ice_dfs[2]  = {"00.1", NULL};
// static uint   ice_cmds_init[2] = { 0x030000, 0x423F04 };
// static uint   ice_ids_count    = 1;
// static uint   ice_dfs_count    = 1;
//
static pci_t *pcis;
static uint pcis_count;
static uint imc_maps_count;
static uint imc_ctrs_count;
static void **imc_maps;
static uint *imc_ctrs;

static state_t load_icelake()
{
    addr_t addr_hi;
    addr_t addr_lo;
    addr_t addr_fi;
    uint p, i, c;
    state_t s;

    imc_maps_count = 4 * pcis_count;     // 4 IMCs * N PCIs (or sockets)
    imc_ctrs_count = imc_maps_count * 2; // N MAPs * 2 counters
    imc_maps       = calloc(imc_maps_count, sizeof(void *));
    imc_ctrs       = calloc(imc_ctrs_count, sizeof(uint));

    for (p = c = 0; p < pcis_count; ++p) {
        addr_hi = 0x00;
        // MMIO_BASE
        if (state_fail(s = pci_read(&pcis[p], &addr_hi, sizeof(uint), 0xD0))) {
            return s;
        }
        addr_hi = (addr_hi & 0x1FFFFFFF) << 23;
        debug("PCI%u base address 0x%lx", p, addr_hi);

        for (i = 0; i < 4; ++i) {
            addr_lo = 0x00;
            // MEMx_BA
            if (state_fail(s = pci_read(&pcis[p], &addr_lo, sizeof(uint), 0xD8 + (i * 0x04)))) {
                return s;
            }
            addr_lo = (addr_lo & 0x7FF) << 12;
            addr_fi = (addr_hi | addr_lo) + 0x2290; // Free running counters base address
            if (state_fail(s = pci_mmio_map(addr_fi, &imc_maps[(p * 4) + i]))) {
                return s;
            }
            debug("IMC%u address physical = 0x%lx (0x%lx | 0x%lx + 0x2290) (map 0x%lx)", (p * 4) + i, addr_fi, addr_hi,
                  addr_lo, imc_maps[(p * 4) + i]);
            // Filling RD and WR counters
            imc_ctrs[c++] = 0x00;
            imc_ctrs[c++] = 0x08;
        }
    }
    return EAR_SUCCESS;
}

state_t bwidth_intel106_load(topology_t *tp, bwidth_ops_t *ops)
{
    state_t s;
    // If AMD or model previous to Intel Haswell
    if (tp->vendor == VENDOR_AMD || tp->model != MODEL_ICELAKE_X) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    debug("Detected Intel Ice Lake");
    if (state_fail(s = pci_scan(0x8086, ice_ids, ice_dfs, O_RDONLY, &pcis, &pcis_count))) {
        serror("pci_scan");
        return s;
    }
    // It is shared by all Intel's architecture from Haswell to Skylake
    debug("PCIs count: %d", pcis_count);

    if (state_fail(s = load_icelake())) {
        return s;
    }
    // In the future it can be initialized in read only mode
    apis_put(ops->get_info, bwidth_intel106_get_info);
    apis_put(ops->init, bwidth_intel106_init);
    apis_put(ops->dispose, bwidth_intel106_dispose);
    apis_put(ops->read, bwidth_intel106_read);

    return EAR_SUCCESS;
}

BWIDTH_F_GET_INFO(bwidth_intel106_get_info)
{
    info->api         = API_INTEL106;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = imc_ctrs_count + 1;
}

state_t bwidth_intel106_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_intel106_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_intel106_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = imc_ctrs_count + 1;
    return EAR_SUCCESS;
}

state_t bwidth_intel106_read(ctx_t *c, bwidth_t *bw)
{
    addr_t addr0;
    addr_t addr1;
    int i, j;

    timestamp_get(&bw[imc_ctrs_count].time);

    for (i = j = 0; i < imc_maps_count; ++i, j += 2) {
        addr0         = ((addr_t) imc_maps[i]) + ((addr_t) imc_ctrs[j + 0]);
        addr1         = ((addr_t) imc_maps[i]) + ((addr_t) imc_ctrs[j + 1]);
        bw[j + 0].cas = (ullong) * ((ullong *) (addr0));
        bw[j + 1].cas = (ullong) * ((ullong *) (addr1));
        bw[j + 0].cas = bw[j + 0].cas & 0x0000ffffffffffff;
        bw[j + 1].cas = bw[j + 1].cas & 0x0000ffffffffffff;
        debug("IMC%d: %llu %llu cas", i, bw[j + 0].cas, bw[j + 1].cas);
    }
    return EAR_SUCCESS;
}
