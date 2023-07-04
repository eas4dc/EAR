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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef METRICS_FLOPS_DUMMY_H
#define METRICS_FLOPS_DUMMY_H

#include <metrics/flops/flops.h>

void flops_dummy_load(topology_t *tp, flops_ops_t *ops);

void flops_dummy_get_api(uint *api);

void flops_dummy_get_granularity(uint *granularity);

void flops_dummy_get_weights(ullong **weights);

state_t flops_dummy_init(ctx_t *c);

state_t flops_dummy_dispose(ctx_t *c);

state_t flops_dummy_read(ctx_t *c, flops_t *fl);

#endif //METRICS_FLOPS_DUMMY_H
