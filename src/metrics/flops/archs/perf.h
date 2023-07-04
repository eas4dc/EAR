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

#ifndef METRICS_FLOPS_PERF_H
#define METRICS_FLOPS_PERF_H

#include <metrics/flops/flops.h>

void flops_perf_load(topology_t *tp, flops_ops_t *ops);

void flops_perf_get_api(uint *api);

void flops_perf_get_granularity(uint *granularity);

void flops_perf_get_weights(ullong **weights_in);

state_t flops_perf_init(ctx_t *c);

state_t flops_perf_dispose(ctx_t *c);

state_t flops_perf_read(ctx_t *c, flops_t *ca);

#endif //METRICS_FLOPS_PERF_H
