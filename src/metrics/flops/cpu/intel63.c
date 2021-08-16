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
#include <metrics/flops/cpu/intel63.h>

static perf_t perf_064f;
static perf_t perf_064d;
static perf_t perf_128f;
static perf_t perf_128d;
static perf_t perf_256f;
static perf_t perf_256d;
static perf_t perf_512f;
static perf_t perf_512d;

static llong values_064[4];
static llong values_256[4];
static uint weights[8];

static llong accum_064f;
static llong accum_064d;
static llong accum_128f;
static llong accum_128d;
static llong accum_256f;
static llong accum_256d;
static llong accum_512f;
static llong accum_512d;

state_t flops_intel63_status(topology_t *tp)
{
	if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
		return EAR_SUCCESS;
	}
	return EAR_ERROR;
}

state_t flops_intel63_init(ctx_t *c)
{
	state_t s;

	// Intel's Manual Volume 3 (FULL), look for 'FP_ARITH_INST'.
	s = perf_open(&perf_064f, NULL, 0, PERF_TYPE_RAW, 0x02c7);
	s = perf_open(&perf_064d, NULL, 0, PERF_TYPE_RAW, 0x01c7);
	s = perf_open(&perf_128f, NULL, 0, PERF_TYPE_RAW, 0x08c7);
	s = perf_open(&perf_128d, NULL, 0, PERF_TYPE_RAW, 0x04c7);
	s = perf_open(&perf_256f, NULL, 0, PERF_TYPE_RAW, 0x20c7);
	s = perf_open(&perf_256d, NULL, 0, PERF_TYPE_RAW, 0x10c7);
	s = perf_open(&perf_512f, NULL, 0, PERF_TYPE_RAW, 0x80c7);
	s = perf_open(&perf_512d, NULL, 0, PERF_TYPE_RAW, 0x40c7);
	
	// Remove warning
	(void) (s);

	// Copying weights
	flops_intel63_weights(weights);

	return EAR_SUCCESS;
}

state_t flops_intel63_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_intel63_reset(ctx_t *c)
{
	state_t s;

	s = perf_reset(&perf_064f);
	s = perf_reset(&perf_064d);
	s = perf_reset(&perf_128f);
	s = perf_reset(&perf_128d);
	s = perf_reset(&perf_256f);
	s = perf_reset(&perf_256d);
	s = perf_reset(&perf_512f);
	s = perf_reset(&perf_512d);
	
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t flops_intel63_start(ctx_t *c)
{
	state_t s;

	s = perf_start(&perf_064f);
	s = perf_start(&perf_064d);
	s = perf_start(&perf_128f);
	s = perf_start(&perf_128d);
	s = perf_start(&perf_256f);
	s = perf_start(&perf_256d);
	s = perf_start(&perf_512f);
	s = perf_start(&perf_512d);
	
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t flops_intel63_read(ctx_t *c, llong *flops, llong *ops)
{
	state_t s;

	s = perf_read(&perf_064f, &values_064[0]);
	s = perf_read(&perf_064d, &values_064[1]);
	s = perf_read(&perf_128f, &values_064[2]);
	s = perf_read(&perf_128d, &values_064[3]);
	s = perf_read(&perf_256f, &values_256[0]);
	s = perf_read(&perf_256d, &values_256[1]);
	s = perf_read(&perf_512f, &values_256[2]);
	s = perf_read(&perf_512d, &values_256[3]);
	
	// Remove warning
	(void) (s);

	debug("read 064 f/d %lld/%lld", values_064[0], values_064[1]);
	debug("read 128 f/d %lld/%lld", values_064[2], values_064[3]);
	debug("read 256 f/d %lld/%lld", values_256[0], values_256[1]);
	debug("read 512 f/d %lld/%lld", values_256[2], values_256[3]);

	accum_064f += values_064[0];
	accum_064d += values_064[1];
	accum_128f += values_064[2];
	accum_128d += values_064[3];
	accum_256f += values_256[0];
	accum_256d += values_256[1];
	accum_512f += values_256[2];
	accum_512d += values_256[3];

	if (ops != NULL) {
	ops[INDEX_064F] = values_064[0];
	ops[INDEX_064D] = values_064[1];
	ops[INDEX_128F] = values_064[2];
	ops[INDEX_128D] = values_064[3];
	ops[INDEX_256F] = values_256[0];
	ops[INDEX_256D] = values_256[1];
	ops[INDEX_512F] = values_256[2];
	ops[INDEX_512D] = values_256[3];
	}

	if (flops != NULL) {
	*flops  = 0;
	*flops += (values_064[0] * weights[INDEX_064F]);
	*flops += (values_064[1] * weights[INDEX_064D]);
	*flops += (values_064[2] * weights[INDEX_128F]);
	*flops += (values_064[3] * weights[INDEX_128D]);
	*flops += (values_256[0] * weights[INDEX_256F]);
	*flops += (values_256[1] * weights[INDEX_256D]);
	*flops += (values_256[2] * weights[INDEX_512F]);
	*flops += (values_256[3] * weights[INDEX_512D]);
	}

	debug("total flops %lld", *flops);

	return EAR_SUCCESS;
}

state_t flops_intel63_stop(ctx_t *c, llong *flops, llong *ops)
{
	state_t s;

	s = perf_stop(&perf_064f);
	s = perf_stop(&perf_064d);
	s = perf_stop(&perf_128f);
	s = perf_stop(&perf_128d);
	s = perf_stop(&perf_256f);
	s = perf_stop(&perf_256d);
	s = perf_stop(&perf_512f);
	s = perf_stop(&perf_512d);
	
	// Remove warning
	(void) (s);

	return flops_intel63_read(c, flops, ops);
}

state_t flops_intel63_count(ctx_t *c, uint *count)
{
	*count = 8;
	return 0;
}

state_t flops_intel63_read_accum(ctx_t *c, llong *ops)
{
	ops[INDEX_064F] = accum_064f;
	ops[INDEX_064D] = accum_064d;
	ops[INDEX_128F] = accum_128f;
	ops[INDEX_128D] = accum_128d;
	ops[INDEX_256F] = accum_256f;
	ops[INDEX_256D] = accum_256d;
	ops[INDEX_512F] = accum_512f;
	ops[INDEX_512D] = accum_512d;
	return EAR_SUCCESS;
}

state_t flops_intel63_weights(uint *weigths)
{
	weigths[INDEX_064F] = 1;
	weigths[INDEX_064D] = 1;
	weigths[INDEX_128F] = 4;
	weigths[INDEX_128D] = 2;
	weigths[INDEX_256F] = 8;
	weigths[INDEX_256D] = 4;
	weigths[INDEX_512F] = 16;
	weigths[INDEX_512D] = 8;
	return EAR_SUCCESS;
}
