/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef PC_SUPPORT_H
#define PC_SUPPORT_H
#include <common/states.h>
#include <common/types/signature.h>
#include <daemon/powercap/powercap_status_conf.h>

state_t pc_support_init(polctx_t *c);
// ulong pc_support_adapt_freq(polctx_t *c,node_powercap_opt_t *pc,ulong f,signature_t *s);
void pc_support_adapt_gpu_freq(polctx_t *c, node_powercap_opt_t *pc, ulong *f, signature_t *s);

void pc_support_compute_next_state(polctx_t *c, node_powercap_opt_t *pc, signature_t *s);

#endif
