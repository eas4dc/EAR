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

#ifndef METRICS_BANDWIDTH_DUMMY_H
#define METRICS_BANDWIDTH_DUMMY_H

#include <metrics/bandwidth/bandwidth.h>

state_t bwidth_dummy_load(topology_t *tp, bwidth_ops_t *ops);

state_t bwidth_dummy_init(ctx_t *c);

state_t bwidth_dummy_init_static(ctx_t *c, bwidth_ops_t *ops);

state_t bwidth_dummy_dispose(ctx_t *c);

state_t bwidth_dummy_count_devices(ctx_t *c, uint *devs_count);

state_t bwidth_dummy_get_granularity(ctx_t *c, uint *granularity);

state_t bwidth_dummy_read(ctx_t *c, bwidth_t *b);

state_t bwidth_dummy_data_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs);

state_t bwidth_dummy_data_accum(bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs);

state_t bwidth_dummy_data_alloc(bwidth_t **b);

state_t bwidth_dummy_data_free(bwidth_t **b);

state_t bwidth_dummy_data_copy(bwidth_t *dst, bwidth_t *src);

state_t bwidth_dummy_data_print(ullong cas, double gbs, int fd);

state_t bwidth_dummy_data_tostr(ullong cas, double gbs, char *buffer, size_t length);

double bwidth_dummy_help_castogbs(ullong cas, double secs);

double bwidth_dummy_help_castotpi(ullong cas, ullong instructions);

#endif
