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

#ifndef METRICS_FLOPS_DUMMY_H
#define METRICS_FLOPS_DUMMY_H

#include <metrics/flops/flops.h>

state_t flops_dummy_load(topology_t *tp, flops_ops_t *ops);

state_t flops_dummy_init(ctx_t *c);

state_t flops_dummy_dispose(ctx_t *c);

state_t flops_dummy_read(ctx_t *c, flops_t *fl);

// Data
state_t flops_dummy_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

state_t flops_dummy_data_accum(flops_t *flA, flops_t *flD, double *gfs);

state_t flops_dummy_data_copy(flops_t *dst, flops_t *src);

state_t flops_dummy_data_print(flops_t *b, double gfs, int fd);

state_t flops_dummy_data_tostr(flops_t *b, double gfs, char *buffer, size_t length);

// Helpers
ullong *flops_dummy_help_toold(flops_t *flD, ullong *flops);

#endif //METRICS_FLOPS_DUMMY_H
