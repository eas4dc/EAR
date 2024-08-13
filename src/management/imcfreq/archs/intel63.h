/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_IMCFREQ_INTEL63_H
#define MANAGEMENT_IMCFREQ_INTEL63_H

#include <management/imcfreq/imcfreq.h>

state_t mgt_imcfreq_intel63_load(topology_t *tp, mgt_imcfreq_ops_t *ops, int eard, my_node_conf_t *conf);

state_t mgt_imcfreq_intel63_init(ctx_t *c);

state_t mgt_imcfreq_intel63_dispose(ctx_t *c);

state_t mgt_imcfreq_intel63_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count);

state_t mgt_imcfreq_intel63_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_imcfreq_intel63_set_current_list(ctx_t *c, uint *index_list);

state_t mgt_imcfreq_intel63_set_current(ctx_t *c, uint pstate_index, int socket);

state_t mgt_imcfreq_intel63_set_auto(ctx_t *c);

/** */
state_t mgt_imcfreq_intel63_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list);

state_t mgt_imcfreq_intel63_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list);

#endif //MANAGEMENT_IMCFREQ_INTEL63_H
