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
#include <metrics/cpi/cpi.h>
#include <metrics/cpi/cpu/dummy.h>
#include <metrics/cpi/cpu/intel63.h>

static struct uncore_op {
	state_t (*init)		(ctx_t *c);
	state_t (*dispose)	(ctx_t *c);
	state_t (*reset)	(ctx_t *c);
	state_t (*start)	(ctx_t *c);
	state_t (*stop)		(ctx_t *c, llong *cycles, llong *insts);
	state_t (*read)		(ctx_t *c, llong *cycles, llong *insts);
} ops;

static ctx_t context;
static ctx_t *c = &context;
static topology_t topo;

int init_basic_metrics()
{
	topology_init(&topo);

	if (state_ok(cpi_intel63_status(&topo)))
	{
		debug("loaded intel63");
		ops.init  = cpi_intel63_init;
		ops.reset = cpi_intel63_reset;
		ops.start = cpi_intel63_start;
		ops.stop  = cpi_intel63_stop;
		ops.read  = cpi_intel63_read;
		ops.dispose = cpi_intel63_dispose;
	}
	else
	{
		debug("loaded dummy");
		ops.init    = cpi_dummy_init;
		ops.reset   = cpi_dummy_reset;
		ops.start   = cpi_dummy_start;
		ops.stop    = cpi_dummy_stop;
		ops.read    = cpi_dummy_read;
		ops.dispose = cpi_dummy_dispose;
	}

	return ops.init(c);
}

void reset_basic_metrics()
{
	if (ops.reset != NULL) {
		ops.reset(c);
	}
}

void start_basic_metrics()
{
	if (ops.start != NULL) {
		ops.start(c);
	}
}

/* This is an obsolete function to make metrics.c compatible. */
static long long accum_cycles;
static long long accum_insts;

void stop_basic_metrics(llong *cycles, llong *insts)
{
	if (ops.stop != NULL) {
		ops.stop(c, cycles, insts);
	}
	accum_cycles += *cycles;
	accum_insts  += *insts;
}

void read_basic_metrics(llong *cycles, llong *insts)
{
	if (ops.read != NULL) {
		ops.read(c, cycles, insts);
	}
	// accum_cycles += *cycles;
	// accum_insts  += *insts;
}

void get_basic_metrics(llong *cycles, llong *insts)
{
	read_basic_metrics(cycles, insts);
	*cycles = accum_cycles;
	*insts  = accum_insts;
}
