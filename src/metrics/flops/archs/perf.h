/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_FLOPS_PERF_H
#define METRICS_FLOPS_PERF_H

#include <metrics/flops/flops.h>

void flops_perf_load(topology_t *tp, flops_ops_t *ops);

void flops_perf_get_info(apinfo_t *info);

state_t flops_perf_init();

state_t flops_perf_dispose();

state_t flops_perf_read(flops_t *ca);

void flops_perf_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

void flops_perf_internals_tostr(char *buffer, int length);

#endif // METRICS_FLOPS_PERF_H
