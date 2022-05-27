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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

// #define SHOW_DEBUGS 1
//#define FREE 1

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
static uint   imc_idxs_count;
static void **imc_maps;
static uint  *imc_idxs;

static state_t load_icelake()
{
	off_t addr_hi;
	off_t addr_lo;
	off_t addr_fi;
	state_t s;
	uint p, i;

	imc_maps_count = 4 * pcis_count;     // 4 IMCs * N PCIs (or sockets)
	imc_idxs_count = imc_maps_count * 2; // N MAPs * 2 counters
	imc_maps = calloc(imc_maps_count, sizeof(void *));
	imc_idxs = calloc(imc_idxs_count, sizeof(uint));

	for (p = 0; p < pcis_count; ++p) {
		addr_hi = 0x00;
		if (state_fail(s = pci_read(&pcis[p], &addr_hi, sizeof(uint), 0xD0))) {
			// Clean PCIs
			return s;
		}
		addr_hi = (addr_hi & 0x1FFFFFFF) << 23;
		debug("PCI%u base address %lx", p, addr_hi);

		for (i = 0; i < 4; ++i) {
			addr_lo = 0x00;
	    		if (state_fail(s = pci_read(&pcis[p], &addr_lo, sizeof(uint), 0xD8+(i*0x04)))) {
				// Clean PCIs
				return s;
			}
			addr_lo = (addr_lo & 0x7FF) << 12;
			addr_fi = (addr_hi | addr_lo);
			debug("IMC%u address = %lx (%lx | %lx)", i, addr_fi, addr_hi, addr_lo);

			if (state_fail(s = pci_mmio_map(addr_fi, 0x4000, &imc_maps[(p*4)+i]))) {
				return s;
			}

			// Filling counters
			// 0*8 + 0*2 + 0 = 0 | 0*8 + 3*2 + 1 = 7
			// 1*8 + 0*2 + 0 = 8 | 1*8 + 3*2 + 1 = 15
			imc_idxs[(p*8)+(i*4)+0] = 0x2290;
			imc_idxs[(p*8)+(i*4)+1] = 0x2298;
		}
	}
	return EAR_SUCCESS;
}

state_t bwidth_intel106_load(topology_t *tp, bwidth_ops_t *ops)
{
	state_t s;
	// If AMD or model previous to Intel Haswell
	if (tp->vendor == VENDOR_AMD || tp->model < MODEL_ICELAKE_X) {
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
		serror("load_icelake");
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
	*devs_count_in = imc_idxs_count+1;
	return EAR_SUCCESS;
}

state_t bwidth_intel106_get_granularity(ctx_t *c, uint *granularity)
{
	*granularity = GRANULARITY_IMC;
	return EAR_SUCCESS;
}

state_t bwidth_intel106_read(ctx_t *c, bwidth_t *bw)
{
	int i, j;
	
	timestamp_get(&bw[imc_idxs_count].time);

	for (i = j = 0; i < imc_maps_count; ++i, j+=2) {
		bw[j+0].cas = (ullong) imc_maps[imc_idxs[j+0]];
		bw[j+1].cas = (ullong) imc_maps[imc_idxs[j+1]];
		bw[j+0].cas = bw[j+0].cas & 0x0000ffffffffffff;
		bw[j+1].cas = bw[j+1].cas & 0x0000ffffffffffff;
		debug("IMC%d: %llu cas", i, bw[j+0].cas + bw[j+1].cas);
	}
	return EAR_SUCCESS;
}

#if 0
// /opt/rh/devtoolset-7/root/bin/gcc -g -I ../../../ -o test intel106.c ../../libmetrics.a ../../../common/libcommon.a

#define N 262144
static ullong array1[N];
static ullong array2[N];
static ullong array3[N];

void work()
{
    int i;

    for (i = 2; i < N; ++i) {
        array1[i] = array2[i-1] + array3[i-2];
    }
}

int main(int argc, char *argv[])
{
    topology_t   tp;
    state_t      s;
    bwidth_ops_t ops;
    bwidth_t     bw[128];    

    topology_init(&tp);
    
    bwidth_intel106_load(&tp, &ops);

    bwidth_intel106_read(NULL, bw);
    debug("WORKING");
    work();
    bwidth_intel106_read(NULL, bw); 

    return 0;
}
#endif
