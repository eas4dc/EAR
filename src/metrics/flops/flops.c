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
#include <metrics/flops/flops.h>
#include <metrics/flops/cpu/dummy.h>
#include <metrics/flops/cpu/amd49.h>
#include <metrics/flops/cpu/intel63.h>

static struct bandwidth_ops {
	state_t (*init)		(ctx_t *c);
	state_t (*dispose)	(ctx_t *c);
	state_t (*count)	(ctx_t *c, uint *count);
	state_t (*reset)	(ctx_t *c);
	state_t (*start)	(ctx_t *c);
	state_t (*stop)		(ctx_t *c, llong *flops, llong *ops);
	state_t (*read)		(ctx_t *c, llong *flops, llong *ops);
	state_t (*read_accum) (ctx_t *c, llong *flops);
	state_t (*weights)	(uint *weights);
} ops;

static ctx_t context;
static ctx_t *c = &context;
static topology_t topo;

int init_flops_metrics()
{
	topology_init(&topo);

	if (state_ok(flops_intel63_status(&topo)))
	{
		debug("loaded intel63");
		ops.init       = flops_intel63_init;
		ops.dispose    = flops_intel63_dispose;
		ops.count      = flops_intel63_count;
		ops.weights    = flops_intel63_weights;
		ops.reset      = flops_intel63_reset;
		ops.start      = flops_intel63_start;
		ops.stop       = flops_intel63_stop;
		ops.read       = flops_intel63_read;
		ops.read_accum = flops_intel63_read_accum;
	} else if (state_ok(flops_amd49_status(&topo)))
	{
		debug("loaded amd49");
		ops.init       = flops_amd49_init;
		ops.dispose    = flops_amd49_dispose;
		ops.count      = flops_amd49_count;
		ops.weights    = flops_amd49_weights;
		ops.reset      = flops_amd49_reset;
		ops.start      = flops_amd49_start;
		ops.stop       = flops_amd49_stop;
		ops.read       = flops_amd49_read;
		ops.read_accum = flops_amd49_read_accum;
	} else {
		debug("load dummy");
		ops.init       = flops_dummy_init;
		ops.dispose    = flops_dummy_dispose;
		ops.count      = flops_dummy_count;
		ops.weights    = flops_dummy_weights;
		ops.reset      = flops_dummy_reset;
		ops.start      = flops_dummy_start;
		ops.stop       = flops_dummy_stop;
		ops.read       = flops_dummy_read;
		ops.read_accum = flops_dummy_read_accum;
	}

	ops.init(c);

	return 1;
}

void reset_flops_metrics()
{
	if (ops.reset != NULL) {
		ops.reset(c);
	}
}

void start_flops_metrics()
{
	if (ops.start != NULL) {
		ops.start(c);
	}
}

void read_flops_metrics(llong *flops, llong *_ops)
{
	if (ops.read != NULL) {
		ops.read(c, flops, _ops);
	}
}

void stop_flops_metrics(llong *flops, llong *_ops)
{
	if (ops.stop != NULL) {
		ops.stop(c, flops, _ops);
	}
}

int get_number_fops_events()
{
	int count = 0;
	if (ops.count != NULL) {
		ops.count(c, (unsigned int *)&count);
	}
	return count;
}

void get_total_fops(llong *flops)
{
	if (ops.read_accum != NULL) {
		ops.read_accum(c, flops);
	}
}

void get_weigth_fops_instructions(int *weigths)
{
	if (ops.weights != NULL) {
		ops.weights((unsigned int *)weigths);
	}
}
