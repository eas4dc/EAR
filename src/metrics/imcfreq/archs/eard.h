/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IMCFREQ_EARD_H
#define METRICS_IMCFREQ_EARD_H

#include <metrics/imcfreq/imcfreq.h>

void imcfreq_eard_load(topology_t *tp, imcfreq_ops_t *ops, int eard);

void imcfreq_eard_get_api(uint *api, uint *api_intern);

state_t imcfreq_eard_init(ctx_t *c);

state_t imcfreq_eard_dispose(ctx_t *c);

state_t imcfreq_eard_count_devices(ctx_t *c, uint *dev_count);

state_t imcfreq_eard_read(ctx_t *c, imcfreq_t *i);

#endif //METRICS_IMCFREQ_EARD_H
