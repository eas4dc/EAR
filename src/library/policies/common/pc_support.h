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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef PC_SUPPORT_H
#define PC_SUPPORT_H
#include <common/states.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <common/types/signature.h>

state_t pc_support_init(polctx_t *c);
ulong pc_support_adapt_freq(polctx_t *c,node_powercap_opt_t *pc,ulong f,signature_t *s);
void pc_support_adapt_gpu_freq(polctx_t *c,node_powercap_opt_t *pc,ulong *f,signature_t *s);

void pc_support_compute_next_state(polctx_t *c,node_powercap_opt_t *pc,signature_t *s);

#endif

