/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_CACHE_PERF_H
#define METRICS_CACHE_PERF_H

#include <metrics/cache/cache.h>

void cache_perf_load(topology_t *tp, cache_ops_t *ops);

void cache_perf_get_info(apinfo_t *info);

state_t cache_perf_init();

state_t cache_perf_dispose();

state_t cache_perf_read(cache_t *ca);

void cache_perf_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

void cache_perf_internals_tostr(char *buffer, int length);

#endif // METRICS_CACHE_PERF_H