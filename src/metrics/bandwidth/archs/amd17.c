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

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/msr.h>
#include <metrics/bandwidth/archs/amd17.h>

// TODO: explore the HSMP function GetMaxDDRBandwidthAndUtilization
// Controllers (L3, L3 limited and DataFabric).
//static off_t  ctls_dfs[4] = { 0xc0010240, 0xc0010242, 0xc0010244, 0xc0010246 };
//static off_t  ctrs_dfs[4] = { 0xc0010241, 0xc0010243, 0xc0010245, 0xc0010247 };
static off_t  ctls_l3f[6] = { 0xc0010230, 0xc0010232, 0xc0010234, 0xc0010236, 0xc0010238, 0xc001023a };
static off_t  ctrs_l3f[6] = { 0xc0010231, 0xc0010233, 0xc0010235, 0xc0010237, 0xc0010239, 0xc001023b };
//static ullong cmds_dfs[4] = { 0x0000000000403807, 0x0000000000403887, 0x0000000100403807, 0x0000000100403887 };
//static ullong cmds_l3f[4] = { 0x030f000000400104, 0x0c0f000000400104, 0x300f000000400104, 0xc00f000000400104 };
static ullong cmds_l3l[1] = { 0xff0f000000400104 };
//static ullong mask_dfs = 0x0000FFFFFFFFFFFF; // 48 bits
static ullong mask_l3f = 0x0000FFFFFFFFFFFF; // 48 bits
//static uint count_l3f = 4;
static uint count_l3l = 1;
//static uint count_dfs = 4;
//static uint gran_l3f = GRANULARITY_CORE;
static uint gran_l3l = GRANULARITY_CCX;
// static uint gran_dfs = GRANULARITY_CCD;
// Selected
static off_t *ctls;
static off_t *ctrs;
static ullong *cmds;
static ullong mask;
static uint ctrs_count;
static uint gran;
//
static topology_t tp;
static uint devs_count;

state_t bwidth_amd17_load(topology_t *tp_in, bwidth_ops_t *ops)
{
	state_t s;

	if (tp_in->vendor != VENDOR_AMD || tp_in->family < FAMILY_ZEN){
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	if (state_fail(s = msr_test(tp_in, MSR_WR))) {
		return s;
	}
	// Getting the L3 groups
	if (state_fail(s = topology_select(tp_in, &tp, TPSelect.l3, TPGroup.merge, 0))) {
		return s;
	}
	// Selected reading (L3 Limited)
	ctrs_count = count_l3l;
	ctls = ctls_l3f;
	ctrs = ctrs_l3f;
	cmds = cmds_l3l;
	gran = gran_l3l;
	mask = mask_l3f;
	//
	devs_count = tp.cpu_count * ctrs_count;
	//
	replace_ops(ops->init,            bwidth_amd17_init);
	replace_ops(ops->dispose,         bwidth_amd17_dispose);
	replace_ops(ops->count_devices,   bwidth_amd17_count_devices);
	replace_ops(ops->get_granularity, bwidth_amd17_get_granularity);
	replace_ops(ops->read,            bwidth_amd17_read);

	return EAR_SUCCESS;
}

state_t bwidth_amd17_init(ctx_t *c)
{
    state_t s;
	int i, j;

	// Finding the free counters (not by now)
	#if 0
	if (state_fail(s = msr_scan(&tp, ctls, 6, cmd, offs))) {
	}
	#endif
	for (i = 0; i < tp.cpu_count; ++i) {
        if (state_fail(s = msr_open(tp.cpus[i].id, MSR_WR))) {
            return s;
        }
		for (j = 0; j < ctrs_count; ++j) {
			msr_write(tp.cpus[i].id, (void *) &cmds[j], sizeof(ullong), ctls[j]);
		}
	}
	return EAR_SUCCESS;
}

state_t bwidth_amd17_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_amd17_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = devs_count+1;
	return EAR_SUCCESS;
}

state_t bwidth_amd17_get_granularity(ctx_t *c, uint *granularity)
{
	*granularity = GRANULARITY_CCX;
	return EAR_SUCCESS;
}

state_t bwidth_amd17_read(ctx_t *c_in, bwidth_t *b)
{
	int c, d, r;

	// Retrieving time in the position N
	timestamp_get(&b[devs_count].time);
	//
	for (c = d = 0; c < tp.cpu_count; ++c) {
		for (r = 0; r < ctrs_count; ++r, ++d) {
			msr_read(tp.cpus[c].id, &b[d].cas, sizeof(ullong), ctrs[r]);
			// In case of DFS, also you have tu multiply by 2.
			b[d].cas &= mask;
			debug("D%d: %llu cas (CPU%d, R%d)", d, b[d].cas, c, r);
		}
	}
	return EAR_SUCCESS;
}
