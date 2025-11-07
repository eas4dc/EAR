/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_FLOPS_DUMMY_H
#define METRICS_FLOPS_DUMMY_H

#include <metrics/flops/flops.h>

void flops_dummy_load(topology_t *tp, flops_ops_t *ops);

void flops_dummy_get_info(apinfo_t *info);

state_t flops_dummy_init();

state_t flops_dummy_dispose();

state_t flops_dummy_read(flops_t *fl);

void flops_dummy_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs);

void flops_dummy_internals_tostr(char *buffer, int length);

#endif // METRICS_FLOPS_DUMMY_H
