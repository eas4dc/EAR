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
