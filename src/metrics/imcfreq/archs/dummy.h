/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IMCFREQ_DUMMY_H
#define METRICS_IMCFREQ_DUMMY_H

#include <metrics/imcfreq/imcfreq.h>

void imcfreq_dummy_load(topology_t *tp, imcfreq_ops_t *ops);

void imcfreq_dummy_get_api(uint *api, uint *api_intern);

state_t imcfreq_dummy_init(ctx_t *c);

state_t imcfreq_dummy_init_static(ctx_t *c);

state_t imcfreq_dummy_dispose(ctx_t *c);

state_t imcfreq_dummy_count_devices(ctx_t *c, uint *dev_count);
/** */
state_t imcfreq_dummy_read(ctx_t *c, imcfreq_t *i);

state_t imcfreq_dummy_data_alloc(imcfreq_t **i, ulong **freq_list);

state_t imcfreq_dummy_data_free(imcfreq_t **i, ulong **freq_list);

state_t imcfreq_dummy_data_copy(imcfreq_t *i2, imcfreq_t *i1);

state_t imcfreq_dummy_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average);

void imcfreq_dummy_data_print(ulong *freq_list, ulong *average, int fd);

char *imcfreq_dummy_data_tostr(ulong *freq_list, ulong *average, char *buffer, size_t length);

#endif // METRICS_IMCFREQ_DUMMY_H
