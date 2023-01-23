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

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <metrics/common/pci.h>
#include <common/output/debug.h>
#include <metrics/bandwidth/archs/intel106.h>

// TODO: have an ICE LAKE node and test it.
static ushort ice_ids[2] = { 0x3451, 0x00 };
static char  *ice_dfs[2] = { "00.1", NULL };
//static uint   ice_cmds_init[2] = { 0x030000, 0x423F04 };
//static uint   ice_ids_count    = 1;
//static uint   ice_dfs_count    = 1;
//
static pci_t *pcis;
static uint   pcis_count;
static uint   imc_maps_count;
static uint   imc_ctrs_count;
static void **imc_maps;
static uint  *imc_ctrs;

static state_t load_icelake()
{
	addr_t addr_hi;
	addr_t addr_lo;
	addr_t addr_fi;
	uint p, i, c;
	state_t s;

	imc_maps_count = 4 * pcis_count;     // 4 IMCs * N PCIs (or sockets)
	imc_ctrs_count = imc_maps_count * 2; // N MAPs * 2 counters
	imc_maps = calloc(imc_maps_count, sizeof(void *));
	imc_ctrs = calloc(imc_ctrs_count, sizeof(uint));

	for (p = c = 0; p < pcis_count; ++p) {
		addr_hi = 0x00;
        // MMIO_BASE
		if (state_fail(s = pci_read(&pcis[p], &addr_hi, sizeof(uint), 0xD0))) {
			// Clean PCIs
			return s;
		}
		addr_hi = (addr_hi & 0x1FFFFFFF) << 23;
		debug("PCI%u base address %lx", p, addr_hi);

		for (i = 0; i < 4; ++i) {
			addr_lo = 0x00;
            // MEMx_BAR
	    	if (state_fail(s = pci_read(&pcis[p], &addr_lo, sizeof(uint), 0xD8+(i*0x04)))) {
				// Clean PCIs
				return s;
			}
			addr_lo = (addr_lo & 0x7FF) << 12;
			addr_fi = (addr_hi | addr_lo) + 0x2290; // Free running counters base address
			if (state_fail(s = pci_mmio_map(addr_fi, &imc_maps[(p*4)+i]))) {
				return s;
			}
			debug("IMC%u address physical = 0x%lx (0x%lx | 0x%lx)", (p*4)+i, addr_fi, addr_hi, addr_lo);
            debug("IMC%u address virtual  = 0x%lx", (p*4)+i, imc_maps[(p*4)+i]);
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
	debug("detected Intel ICE LAKE");
	if (state_fail(s = pci_scan(0x8086, ice_ids, ice_dfs, O_RDONLY, &pcis, &pcis_count))) {
		serror("pci_scan");
		return s;
	}
	// It is shared by all Intel's architecture from Haswell to Skylake
	debug("PCIs count: %d", pcis_count);
	
    if (state_fail(s = load_icelake())) {
		// serror("load_icelake");
		return s;
	}
	// In the future it can be initialized in read only mode
	replace_ops(ops->init,            bwidth_intel106_init);
	replace_ops(ops->dispose,         bwidth_intel106_dispose);
	replace_ops(ops->count_devices,   bwidth_intel106_count_devices);
	replace_ops(ops->get_granularity, bwidth_intel106_get_granularity);
	replace_ops(ops->read,            bwidth_intel106_read);

	return EAR_SUCCESS;
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
	*devs_count_in = imc_ctrs_count+1;
	return EAR_SUCCESS;
}

state_t bwidth_intel106_get_granularity(ctx_t *c, uint *granularity)
{
	*granularity = GRANULARITY_IMC;
	return EAR_SUCCESS;
}

state_t bwidth_intel106_read(ctx_t *c, bwidth_t *bw)
{
    addr_t addr0;
    addr_t addr1; 
	int i, j;
	
	timestamp_get(&bw[imc_ctrs_count].time);

	for (i = j = 0; i < imc_maps_count; ++i, j+=2) {
		debug("IMC%d: RD DDR addr 0x%lx", i, imc_maps[i]);
        addr0 = ((addr_t) imc_maps[i]) + ((addr_t) imc_ctrs[j+0]);
        addr1 = ((addr_t) imc_maps[i]) + ((addr_t) imc_ctrs[j+1]);
		bw[j+0].cas = (ullong) *((ullong *) (addr0));
		bw[j+1].cas = (ullong) *((ullong *) (addr1));
		bw[j+0].cas = bw[j+0].cas & 0x0000ffffffffffff;
		bw[j+1].cas = bw[j+1].cas & 0x0000ffffffffffff;
		debug("IMC%d: %llu cas", i, bw[i].cas + bw[i].cas);
	}
	return EAR_SUCCESS;
}
