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

#ifndef METRICS_BANDWIDTH_INTEL63_H
#define METRICS_BANDWIDTH_INTEL63_H

#include <metrics/bandwidth/bandwidth.h>

state_t bwidth_intel63_load(topology_t *tp, bwidth_ops_t *ops);

state_t bwidth_intel63_init(ctx_t *c);

state_t bwidth_intel63_dispose(ctx_t *c);

state_t bwidth_intel63_count_devices(ctx_t *c, uint *devs_count);

state_t bwidth_intel63_get_granularity(ctx_t *c, uint *granularity);

state_t bwidth_intel63_read(ctx_t *c, bwidth_t *b);

#endif