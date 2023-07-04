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

#ifndef METRICS_CACHE_PERF_H
#define METRICS_CACHE_PERF_H

#include <metrics/cache/cache.h>

void cache_perf_load(topology_t *tp, cache_ops_t *ops);

void cache_perf_get_info(apinfo_t *info);

state_t cache_perf_init();

state_t cache_perf_dispose();

state_t cache_perf_read(cache_t *ca);

void cache_perf_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

void cache_perf_details_tostr(char *buffer, int length);

#endif //METRICS_CACHE_PERF_H