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

#include <common/output/debug.h>
#include <metrics/common/perf.h>
#include <metrics/flops/cpu/amd49.h>

// Currently we don't count FLOPs, just instructions using a Perf RAW
// event. Old way to count FLOPs:
//
// References:
// PPR for AMD Family 17h. Topic 2.1.15.4.1 Floating Point (FP) Events.
//
// The event FpRetSseAvxOps works next MergeEvent in the set of generic
// AMD Performance Monitoring Counters. MergeEvent (0xFFF) have to be
// set in an odd numbered PMC (there are 6 per thread). Then, the event
// FpRetSseAvxOps have to be set in the corresponding even counter.
// Supposedly, the FpRetSseAvxOps have to be set in the counter 0, 2 or
// 4 and the MergeEvent in 1, 3 and 5.
//
// Working scheme:
//	0 event + 1 merge
//	2 event + 3 merge
//	4 event + 5 merge
//
// We found that in the an AMD node a fixed event was always present
// sitting in the counter 0. So we manage to create a combination of
// even/odd FpRetSseAvxOps and MergeEvent to satisfy at least one
// working scheme (F M M F M).

const ulong	cmd_avx_insts = 0x00000000000004cb; // AVX instructions event
const ulong cmd_avx_flops = 0x000000000000ff03; // Flops event
const ulong cmd_avx_merge = 0x0000000f000000ff; // Merge event

static perf_t perf_evn0;
#if 0
static perf_t perf_mer1;
static perf_t perf_mer2;
static perf_t perf_evn3;
static perf_t perf_mer4;
#endif

// The counters are 48 bit values. But it can be added the MergeEvent
// counter which supposedly expands the counters to 64 bits.
static llong values[8];
static llong accums[8];

state_t flops_amd49_status(topology_t *tp)
{
	if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
		return EAR_SUCCESS;
	}
	return EAR_ERROR;
}

state_t flops_amd49_init(ctx_t *c)
{
	state_t s;

	// Why we are then counting the instructions instead the pure FLOPS?
	// Because its difficult to set the required events in its proper
	// place in the set of counters. We are just counting instructions.
	s = perf_open(&perf_evn0, NULL, 0, PERF_TYPE_RAW, cmd_avx_insts);
	// Old FLOPs counting
	#if 0
	s = perf_opex(&perf_evn0, &perf_evn0, 0, PERF_TYPE_RAW, cmd_avx_flops, pf_exc);
	s = perf_open(&perf_mer1, &perf_evn0, 0, PERF_TYPE_RAW, cmd_avx_merge);
	s = perf_open(&perf_mer2, &perf_evn0, 0, PERF_TYPE_RAW, cmd_avx_merge);
	s = perf_open(&perf_evn3, &perf_evn0, 0, PERF_TYPE_RAW, cmd_avx_flops);
	s = perf_open(&perf_mer4, &perf_evn0, 0, PERF_TYPE_RAW, cmd_avx_merge);
	#endif
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t flops_amd49_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_amd49_reset(ctx_t *c)
{
	state_t s;

	s = perf_reset(&perf_evn0);
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t flops_amd49_start(ctx_t *c)
{
	state_t s;

	s = perf_start(&perf_evn0);
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t flops_amd49_read(ctx_t *c, llong *flops, llong *ops)
{
	state_t s;

	s = perf_read(&perf_evn0, values);
	// Remove warning
	(void) (s);

	accums[0] += (values[0] / WEIGHT_128D);
	// We are counting instructions, and we are doing it in 128D index.
	if (ops != NULL) {
		ops[INDEX_064F] = 0;
		ops[INDEX_064D] = 0;
		ops[INDEX_128F] = 0;
		ops[INDEX_128D] = values[0] / WEIGHT_128D;
		ops[INDEX_256F] = 0;
		ops[INDEX_256D] = 0;
		ops[INDEX_512F] = 0;
		ops[INDEX_512D] = 0;
	}
	// Multiplying by the 128D weight, the minimum SSE/AVX FLOPs
	if (flops != NULL) {
		*flops = values[0];
	}
	debug("total flops %lld", *flops);

	return EAR_SUCCESS;
}

state_t flops_amd49_stop(ctx_t *c, llong *flops, llong *ops)
{
	state_t s;

	s = perf_stop(&perf_evn0);	
	// Remove warning
	(void) (s);

	return flops_amd49_read(c, flops, ops);
}

state_t flops_amd49_count(ctx_t *c, uint *count)
{
	*count = 8;
	return 0;
}

state_t flops_amd49_read_accum(ctx_t *c, llong *ops)
{
	ops[INDEX_064F] = 0;
	ops[INDEX_064D] = 0;
	ops[INDEX_128F] = 0;
	ops[INDEX_128D] = accums[0];
	ops[INDEX_256F] = 0;
	ops[INDEX_256D] = 0;
	ops[INDEX_512F] = 0;
	ops[INDEX_512D] = 0;
	return EAR_SUCCESS;
}

state_t flops_amd49_weights(uint *weigths)
{
	weigths[INDEX_064F] = WEIGHT_064F;
	weigths[INDEX_064D] = WEIGHT_064D;
	weigths[INDEX_128F] = WEIGHT_128F;
	weigths[INDEX_128D] = WEIGHT_128D;
	weigths[INDEX_256F] = WEIGHT_256F;
	weigths[INDEX_256D] = WEIGHT_256D;
	weigths[INDEX_512F] = WEIGHT_512F;
	weigths[INDEX_512D] = WEIGHT_512D;
	return EAR_SUCCESS;
}
