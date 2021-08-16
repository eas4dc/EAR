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

state_t cache_perf_status(topology_t *tp);

state_t cache_perf_init(ctx_t *c);

state_t cache_perf_dispose(ctx_t *c);

state_t cache_perf_reset(ctx_t *c);

state_t cache_perf_start(ctx_t *c);

state_t cache_perf_stop(ctx_t *c, llong *L1_misses, llong *LL_misses);

state_t cache_perf_read(ctx_t *c, llong *L1_misses, llong *LL_misses);

state_t cache_perf_data_print(ctx_t *c, llong L1_misses, llong LL_misses);

#endif //METRICS_CPI_INTEL63
