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

#ifndef METRICS_FLOPS_PERF_H
#define METRICS_FLOPS_PERF_H

#include <metrics/flops/flops.h>

state_t flops_perf_load(topology_t *tp, flops_ops_t *ops);

state_t flops_perf_init(ctx_t *c);

state_t flops_perf_dispose(ctx_t *c);

state_t flops_perf_read(ctx_t *c, flops_t *ca);

#endif //METRICS_FLOPS_PERF_H
