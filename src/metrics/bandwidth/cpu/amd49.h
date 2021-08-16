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

#ifndef METRICS_BANDWIDTH_AMD49_H
#define METRICS_BANDWIDTH_AMD49_H

#include <metrics/bandwidth/bandwidth.h>

state_t bwidth_amd49_status(topology_t *tp);

state_t bwidth_amd49_init(ctx_t *c, topology_t *tp);

state_t bwidth_amd49_dispose(ctx_t *c);

state_t bwidth_amd49_count(ctx_t *c, uint *count);

state_t bwidth_amd49_start(ctx_t *c);

state_t bwidth_amd49_reset(ctx_t *c);

state_t bwidth_amd49_stop(ctx_t *c, ullong *cas);

state_t bwidth_amd49_read(ctx_t *c, ullong *cas);

/* Returns the total number of memory accesses */
unsigned long long acum_uncores(unsigned long long * unc,int n);


#endif
