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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <metrics/common/pci.h>
#include <common/output/debug.h>
#include <metrics/bandwidth/archs/intel63.h>

// Haswell
static ushort hwell_ids[9] = { 0x2FB4, 0x2FB5, 0x2FB0, 0x2FB1, 0x2FD4, 0x2FD5, 0x2FD0, 0x2FD1, 0x00 };
static char  *hwell_dfs[9] = { "14.0", "14.1", "15.0", "15.1", "17.0", "17.1", "18.0", "18.1", NULL };
static off_t  hwell_ctls_addr[2]  = { 0xF4, 0xD8 };
static off_t  hwell_ctrs_addr[1]  = { 0xA0 };
static uint   hwell_cmds_start[2] = { 0x030000, 0x420F04 };
//static uint   hwell_ids_count     = 8;
//static uint   hwell_dfs_count     = 8;
static uint   hwell_ctls_count    = 2;
static uint   hwell_ctrs_count    = 1;
// Broadwell
static ushort bwell_ids[9] = { 0x6FB4, 0x6FB5, 0x6FB0, 0x6FB1, 0x6FD4, 0x6FD5, 0x6FD0, 0x6FD1, 0x00 };
//static uint   bwell_ids_count     = 8;
// Skylake
static ushort slake_ids[4] = { 0x2042, 0x2046, 0x204A, 0x00 };
static char  *slake_dfs[9] = { "0a.2","0a.6","0b.2","0b.6","0c.2","0c.6","0d.2","0d.6", NULL };
//static uint   slake_ids_count     = 3;
//static uint   slake_dfs_count     = 8;
//
static pci_t *pcis;
static uint   pcis_count;
static off_t *ctls_addr;
static off_t *ctrs_addr;
static uint  *cmds_start;
static uint   ctls_count;
static uint   ctrs_count;
static uint   devs_count;


state_t bwidth_intel63_load(topology_t *tp, bwidth_ops_t *ops)
{
	state_t s;
	// If AMD or model previous to Intel Haswell
	if (tp->vendor == VENDOR_AMD || tp->model < MODEL_HASWELL_X || tp->model > MODEL_SKYLAKE_X) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	if (tp->vendor == VENDOR_INTEL && tp->model == MODEL_SKYLAKE_X) {
		debug("detected Intel SKY LAKE");
		if(state_fail(s = pci_scan(0x8086, slake_ids, slake_dfs, O_RDWR, &pcis, &pcis_count))) {
			return s;
		}
	} else if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_BROADWELL_X) {
		debug("detected Intel BROADWELL");
		if(state_fail(s = pci_scan(0x8086, bwell_ids, hwell_dfs, O_RDWR, &pcis, &pcis_count))) {
			return s;
		}
	} else {
		debug("detected Intel HASWELL");
		if(state_fail(s = pci_scan(0x8086, hwell_ids, hwell_dfs, O_RDWR, &pcis, &pcis_count))) {
			return s;
		}
	}
	ctls_addr  = hwell_ctls_addr;
	ctrs_addr  = hwell_ctrs_addr;
	cmds_start = hwell_cmds_start;
	ctls_count = hwell_ctls_count;
	ctrs_count = hwell_ctrs_count;
	// It is shared by all Intel's architecture from Haswell to Skylake
	devs_count = pcis_count * ctrs_count;
	debug("PCIs count: %d", pcis_count);
	// In the future it can be initialized in read only mode
	apis_put(ops->get_info, bwidth_intel63_get_info);
	apis_put(ops->init,     bwidth_intel63_init);
	apis_put(ops->dispose,  bwidth_intel63_dispose);
	apis_put(ops->read,     bwidth_intel63_read);
	return EAR_SUCCESS;
}

BWIDTH_F_GET_INFO(bwidth_intel63_get_info)
{
    info->api         = API_INTEL63;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = devs_count+1;
}

state_t bwidth_intel63_init(ctx_t *c)
{
	// In the future the counter could be dynamically selected
	return pci_mwrite32(pcis, pcis_count, (const uint *) cmds_start, ctls_addr, ctls_count);
}

state_t bwidth_intel63_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_intel63_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = devs_count+1;
	return EAR_SUCCESS;
}

state_t bwidth_intel63_read(ctx_t *c, bwidth_t *bw)
{
	int p, t, i;
	ullong cas = 0LLU;

	timestamp_get(&bw[devs_count].time);
	for (p = i = 0; p < pcis_count; ++p) {
		for (t = 0; t < ctrs_count; ++t, ++i){
            bw[i].cas = 0LLU;
			pci_read(&pcis[p], &bw[i].cas, sizeof(ullong), ctrs_addr[t]);
			bw[i].cas = bw[i].cas & 0x0000ffffffffffff;
			debug("IMC%d/%u: %llu (read)", i, devs_count-1, bw[i].cas);
			cas += bw[i].cas; 
		}
	}
    debug("CAS %llu", cas);
 
	return EAR_SUCCESS;
}
