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

#ifndef METRICS_CACHE_DUMMY_H
#define METRICS_CACHE_DUMMY_H

#include <metrics/cache/cache.h>

state_t cache_dummy_load(topology_t *tp, cache_ops_t *ops);

state_t cache_dummy_init(ctx_t *c);

state_t cache_dummy_dispose(ctx_t *c);

state_t cache_dummy_read(ctx_t *c, cache_t *ca);

// Helpers
state_t cache_dummy_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs);

state_t cache_dummy_data_copy(cache_t *dst, cache_t *src);

state_t cache_dummy_data_print(cache_t *b, double gbs, int fd);

state_t cache_dummy_data_tostr(cache_t *b, double gbs, char *buffer, size_t length);

#endif //METRICS_CACHE_DUMMY_H
