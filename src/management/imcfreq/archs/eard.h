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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef MANAGEMENT_IMCFREQ_EARD_H
#define MANAGEMENT_IMCFREQ_EARD_H

#include <management/imcfreq/imcfreq.h>

state_t mgt_imcfreq_eard_load(topology_t *tp, mgt_imcfreq_ops_t *ops, int eard);

state_t mgt_imcfreq_eard_init(ctx_t *c);

state_t mgt_imcfreq_eard_dispose(ctx_t *c);

state_t mgt_imcfreq_eard_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count);

state_t mgt_imcfreq_eard_get_current_list(ctx_t *c, pstate_t *pstate_list);

state_t mgt_imcfreq_eard_set_current_list(ctx_t *c, uint *pstate_list);

state_t mgt_imcfreq_eard_set_current(ctx_t *c, uint pstate_index, int socket);

state_t mgt_imcfreq_eard_set_auto(ctx_t *c);

state_t mgt_imcfreq_eard_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list);

state_t mgt_imcfreq_eard_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list);

#endif //MANAGEMENT_IMCFREQ_EARD_H
