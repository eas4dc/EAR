/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_BANDWIDTH_INTEL143_H
#define METRICS_BANDWIDTH_INTEL143_H

#include <metrics/bandwidth/bandwidth.h>

state_t bwidth_intel143_load(topology_t *tp, bwidth_ops_t *ops);

BWIDTH_F_GET_INFO(bwidth_intel143_get_info);

state_t bwidth_intel143_init(ctx_t *c);

state_t bwidth_intel143_dispose(ctx_t *c);

state_t bwidth_intel143_count_devices(ctx_t *c, uint *devs_count);

state_t bwidth_intel143_get_granularity(ctx_t *c, uint *granularity);

state_t bwidth_intel143_read(ctx_t *c, bwidth_t *b);

#endif
