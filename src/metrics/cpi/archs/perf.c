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

#include <common/output/debug.h>
#include <metrics/common/perf.h>
#include <metrics/cpi/archs/perf.h>

// Generic
static ullong gen_ins_ev   = PERF_COUNT_HW_INSTRUCTIONS;
static ullong gen_cyc_ev   = PERF_COUNT_HW_CPU_CYCLES;
static uint   gen_ins_tp   = PERF_TYPE_HARDWARE;
static uint   gen_cyc_tp   = PERF_TYPE_HARDWARE;
// Since Haswell
//static ullong hwell_sta_ev = 0x010e;
//static uint   hwell_sta_tp = PERF_TYPE_RAW;
//
static perf_t perf_ins;
static perf_t perf_cyc;
static perf_t perf_sta;
static uint ins_en;
static uint cyc_en;
static uint sta_en;
static uint started;

state_t cpi_perf_load(topology_t *tp, cpi_ops_t *ops)
{
	if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
	} else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
	} else {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	// Opening events
	if (state_ok(perf_open(&perf_ins, NULL, 0, gen_ins_tp, gen_ins_ev))) {
	//if (state_ok(perf_open(&perf_ins, NULL, 0, PERF_TYPE_RAW, 0x00c0))) {
		ins_en = 1;
	}
	if (state_ok(perf_open(&perf_cyc, NULL, 0, gen_cyc_tp, gen_cyc_ev))) {
		cyc_en = 1;
	}
    if (!ins_en && !cyc_en) {
        return EAR_ERROR;
    }
	replace_ops(ops->init,    cpi_perf_init);
	replace_ops(ops->dispose, cpi_perf_dispose);
	replace_ops(ops->read,    cpi_perf_read);
	return EAR_SUCCESS;
}

state_t cpi_perf_init(ctx_t *c)
{
	if (!started) {
		if (ins_en) {
			perf_start(&perf_ins);
		}
		if (cyc_en) {
			perf_start(&perf_cyc);
		}
		if (sta_en) {
			perf_start(&perf_sta);
		}
		++started;
	}
	return EAR_SUCCESS;
}

state_t cpi_perf_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cpi_perf_read(ctx_t *c, cpi_t *ci)
{
	memset(ci, 0, sizeof(cpi_t));
	if (ins_en) {
		perf_read(&perf_ins, (llong *)&ci->instructions);
	}
	if (cyc_en) {
		perf_read(&perf_cyc, (llong *)&ci->cycles);
	}
	if (sta_en) {
		perf_read(&perf_sta, (llong *)&ci->stalls);
	}
	debug("Instructions: %llu", ci->instructions);
	debug("Cycles: %llu", ci->cycles);
	debug("Stalls: %llu", ci->stalls);
	return EAR_SUCCESS;
}
