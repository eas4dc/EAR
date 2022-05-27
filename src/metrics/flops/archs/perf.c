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

#include <common/system/time.h>
#include <common/output/debug.h>
#include <metrics/common/perf.h>
#include <metrics/flops/archs/perf.h>

static ulong hwell_evs[8]  = { 0x02c7, 0x01c7, 0x08c7, 0x04c7, 0x20c7, 0x10c7, 0x80c7, 0x40c7 };
//static uint  hwell_count   = 8;
static ulong hwell_divisor = 1;
// It is supposed that Perf keeps ff03 event in the correct CPU registers.
static uint  zen_evs[8]    = { 0xff03 }; // Old event: 0x04cb
//static uint  zen_count     = 1;
static ulong zen_divisor   = 2;

static perf_t perfs[8];
static uint levels[8];
static ulong divisor;
static uint counter;
static uint started;

void set_flops(ullong event, uint type, uint level)
{
	state_t s;

	if (state_fail(s = perf_open(&perfs[counter], NULL, 0, type, event))) {
		debug("perf_open returned %d (%s)", s, state_msg);
		return;
	}
	levels[counter] = level;
	counter++;
}

state_t flops_perf_load(topology_t *tp, flops_ops_t *ops)
{
	if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
		set_flops(hwell_evs[0], PERF_TYPE_RAW, 1);
		set_flops(hwell_evs[1], PERF_TYPE_RAW, 2);
		set_flops(hwell_evs[2], PERF_TYPE_RAW, 3);
		set_flops(hwell_evs[3], PERF_TYPE_RAW, 4);
		set_flops(hwell_evs[4], PERF_TYPE_RAW, 5);
		set_flops(hwell_evs[5], PERF_TYPE_RAW, 6);
		set_flops(hwell_evs[6], PERF_TYPE_RAW, 7);
		set_flops(hwell_evs[7], PERF_TYPE_RAW, 8);
		divisor = hwell_divisor; 
	}
	else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
		set_flops(zen_evs[0], PERF_TYPE_RAW, 4);
		divisor = zen_divisor;
	}
	else {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
    if (!counter) {
        return EAR_ERROR;
    }
	replace_ops(ops->init,    flops_perf_init);
	replace_ops(ops->dispose, flops_perf_dispose);
	replace_ops(ops->read,    flops_perf_read);

	return EAR_SUCCESS;
}

state_t flops_perf_init(ctx_t *c)
{
	uint i;
	if (!started) {
		for (i = 0; i < counter; ++i) {
			perf_start(&perfs[i]);
		}
		++started;
	}
	return EAR_SUCCESS;
}

state_t flops_perf_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_perf_read(ctx_t *c, flops_t *fl)
{
	ullong *p = (ullong *) fl;
	ullong value;
	int i;

	memset(fl, 0, sizeof(flops_t));
	timestamp_get(&fl->time);
	for (i = 0; i < counter; ++i) {
		value = 0LU;
		perf_read(&perfs[i], (long long *)&value);
		debug("values[%d]: %lu", i, value);
		p[levels[i]-1] = value / divisor;
	}
	return EAR_SUCCESS;
}
