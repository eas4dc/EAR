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

#ifndef METRICS_FLOPS_INTEL63
#define METRICS_FLOPS_INTEL63

#include <metrics/flops/flops.h>

state_t flops_intel63_status(topology_t *tp);

state_t flops_intel63_init(ctx_t *c);

state_t flops_intel63_dispose(ctx_t *c);

state_t flops_intel63_reset(ctx_t *c);

state_t flops_intel63_start(ctx_t *c);

state_t flops_intel63_read(ctx_t *c, llong *flops, llong *ops);

state_t flops_intel63_stop(ctx_t *c, llong *flops, llong *ops);

state_t flops_intel63_count(ctx_t *c, uint *count);

state_t flops_intel63_read_accum(ctx_t *c, llong *metrics);

state_t flops_intel63_weights(uint *weigths);

#endif
