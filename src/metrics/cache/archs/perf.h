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

#ifndef METRICS_CACHE_PERF
#define METRICS_CACHE_PERF

#include <metrics/cache/cache.h>

state_t cache_perf_load(topology_t *tp, cache_ops_t *ops);

state_t cache_perf_init(ctx_t *c);

state_t cache_perf_dispose(ctx_t *c);

state_t cache_perf_read(ctx_t *c, cache_t *ca);

#endif //METRICS_CPI_INTEL63
