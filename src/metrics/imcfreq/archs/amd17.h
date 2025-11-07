/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IMCFREQ_AMD17_H
#define METRICS_IMCFREQ_AMD17_H

#include <metrics/imcfreq/imcfreq.h>

void imcfreq_amd17_load(topology_t *tp, imcfreq_ops_t *ops, int eard);

void imcfreq_amd17_get_api(uint *api, uint *api_intern);

state_t imcfreq_amd17_init(ctx_t *c);

state_t imcfreq_amd17_init_static(ctx_t *c);

state_t imcfreq_amd17_dispose(ctx_t *c);

state_t imcfreq_amd17_count_devices(ctx_t *c, uint *dev_count);
/** */
state_t imcfreq_amd17_read(ctx_t *c, imcfreq_t *i);

state_t imcfreq_amd17_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average);

#endif // METRICS_IMCFREQ_DUMMY_H
