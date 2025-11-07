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
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
 * and COPYING.EPL files.
 */

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <fcntl.h>
#include <metrics/bandwidth/archs/intel143.h>
#include <metrics/common/pci.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// Document: SPR Uncore Perfmon Programming Guide
static pci_t *pcis;
static uint pcis_count;
static uint imc_maps_count;
static uint imc_ctrs_count;
static char **imc_maps;
// Table 1-12. Uncore Performance Monitoring Registers
static ushort sap_ids[2] = {0x3251, 0x00};
static char *sap_dfs[2]  = {"00.1", NULL};

static state_t load_sapphire()
{
    addr_t addr_hi;
    addr_t addr_lo;
    addr_t addr_fi;
    uint p, i;
    state_t s;

    imc_maps_count = pcis_count * 4 * 2; // #PCIs (or sockets) * 4 IMcs * 2 channels
    imc_ctrs_count = imc_maps_count;     // #MAPs * 1 counters
    imc_maps       = calloc(imc_maps_count, sizeof(void *));
    debug("#IMC maps: %u", imc_maps_count);
    debug("#IMC ctrs: %u", imc_ctrs_count);
    debug("#IMC ctls: %u", imc_ctrs_count);

    for (p = 0; p < pcis_count; ++p) {
        addr_hi = 0x00;
        // MMIO_BASE (for each PCI device)
        if (state_fail(s = pci_read(&pcis[p], &addr_hi, sizeof(uint), 0xD0))) {
            return s;
        }
        addr_hi = (addr_hi & 0x1FFFFFFF) << 23;
        debug("PCI%u base address 0x%llx", p, addr_hi);
        // 4 IMCs (compute address of each of 4 IMCs)
        for (i = 0; i < 4; ++i) {
            addr_lo = 0x00;
            // MEMx_BAR (0xD8, 0xDC, 0xE0 and 0xE4 in the document, which is the same as 0xD8+(i*0x04).
            if (state_fail(s = pci_read(&pcis[p], &addr_lo, sizeof(uint), 0xD8 + (i * 0x04)))) {
                return s;
            }
            addr_lo = (addr_lo & 0x7FF) << 12;
            // Channel0 Ctr0
            addr_fi = (addr_hi | addr_lo) + 0x22800; // + SPR_IMC_MMIO_CHN_OFFSET (0x22800)
            if (state_ok(s = pci_mmio_map(addr_fi, (void **) &imc_maps[(p * 8) + (i * 2) + 0]))) {
                debug("IMC%u CHANNEL0 physical address = 0x%lx (0x%lx | 0x%lx + 0x22800) (map 0x%lx)", (p * 4) + i,
                      addr_fi, addr_hi, addr_lo, imc_maps[(p * 8) + (i * 2) + 0]);
            }
            // Channel1 Ctr0
            addr_fi = (addr_hi | addr_lo) + 0x2a800; // + SPR_IMC_MMIO_CHN_OFFSET + SPR_IMC_MMIO_CHN_STRIDE (0x8000)
            if (state_ok(s = pci_mmio_map(addr_fi, (void **) &imc_maps[(p * 8) + (i * 2) + 1]))) {
                debug("IMC%u CHANNEL1 physical address = 0x%lx (0x%lx | 0x%lx + 0x26800) (map 0x%lx)", (p * 4) + i,
                      addr_fi, addr_hi, addr_lo, imc_maps[(p * 8) + (i * 2) + 1]);
            }
        }
    }
    return EAR_SUCCESS;
}

state_t bwidth_intel143_load(topology_t *tp, bwidth_ops_t *ops)
{
    state_t s;
    // If AMD or model previous to Intel Haswell
    if (tp->vendor == VENDOR_AMD || tp->model != MODEL_SAPPHIRE_RAPIDS) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    debug("Detected Intel Sapphire Rapids");
    if (state_fail(s = pci_scan(0x8086, sap_ids, sap_dfs, O_RDONLY, &pcis, &pcis_count))) {
        serror("pci_scan");
        return s;
    }
    // It is shared by all Intel's architecture from Haswell to Skylake
    debug("PCIs count: %d", pcis_count);
    if (state_fail(s = load_sapphire())) {
        return s;
    }
    // In the future it can be initialized in read only mode
    apis_put(ops->get_info, bwidth_intel143_get_info);
    apis_put(ops->init, bwidth_intel143_init);
    apis_put(ops->dispose, bwidth_intel143_dispose);
    apis_put(ops->read, bwidth_intel143_read);

    return EAR_SUCCESS;
}

BWIDTH_F_GET_INFO(bwidth_intel143_get_info)
{
    info->api         = API_INTEL143;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = imc_ctrs_count + 1;
}

#define read64(i, address)         *((ullong *) &imc_maps[i][address])
#define write64(i, address, value) read64(i, address) = value

state_t bwidth_intel143_init(ctx_t *c)
{
    int i;
    // Section: IMC Performance Monitoring Overview
    for (i = 0; i < imc_ctrs_count; ++i) {
        write64(i, 0x40, 0x00ff05); // 0x40 CTL0, UMASK[15:8]: RD+WR(0xFF), EVENT[7:0]: CAS_COUNT (0x05)
        debug("MC_CHy_PCI_PMON_CTL%d: 0x%llx", i, read64(i, 0x40));
    }
    return EAR_SUCCESS;
}

state_t bwidth_intel143_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_intel143_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = imc_ctrs_count + 1;
    return EAR_SUCCESS;
}

state_t bwidth_intel143_read(ctx_t *c, bwidth_t *bw)
{
    int i;
    timestamp_get(&bw[imc_ctrs_count].time);
    for (i = 0; i < imc_ctrs_count; ++i) {
        bw[i].cas = read64(i, 0x08);
        debug("MC_CHy_PCI_PMON_CTR%d: %llu cas", i, bw[i].cas);
    }
    return EAR_SUCCESS;
}
