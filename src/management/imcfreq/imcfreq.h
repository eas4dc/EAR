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

#ifndef MANAGEMENT_IMCFREQ_H
#define MANAGEMENT_IMCFREQ_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>
#include <metrics/common/pstate.h>

#define all_sockets all_devs

typedef struct mgt_imcfreq_ops_s
{
	state_t (*init)               (ctx_t *c);
	state_t (*dispose)            (ctx_t *c);
	state_t (*count_devices)      (ctx_t *c, uint *devs_count);
	state_t (*get_available_list) (ctx_t *c, const pstate_t **pstate_list, uint *pstate_count);
	state_t (*get_current_list)   (ctx_t *c, pstate_t *pstate_list);
	state_t (*set_current_list)   (ctx_t *c, uint *index_list);
	state_t (*set_current)        (ctx_t *c, uint pstate_index, int socket);
	state_t (*set_auto)           (ctx_t *c);
	state_t (*get_current_ranged_list) (ctx_t *c, pstate_t *ps_max_list, pstate_t *ps_min_list);
	state_t (*set_current_ranged_list) (ctx_t *c, uint *id_max_list, uint *id_min_list);
	state_t (*data_alloc)         (pstate_t **pstate_list, uint **index_list);
} mgt_imcfreq_ops_t;

/** */
state_t mgt_imcfreq_load(topology_t *c, int eard);
/** Returns the loaded API. */
state_t mgt_imcfreq_get_api(uint *api);
/** */
state_t mgt_imcfreq_init(ctx_t *c);
/** */
state_t mgt_imcfreq_dispose(ctx_t *c);
/** This API is per socket. */
state_t mgt_imcfreq_count_devices(ctx_t *c, uint *devs_count);
/** */
state_t mgt_imcfreq_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count);
/** */
state_t mgt_imcfreq_get_current_list(ctx_t *c, pstate_t *pstate_list);
/** */
state_t mgt_imcfreq_set_current_list(ctx_t *c, uint *index_list);
/** */
state_t mgt_imcfreq_set_current(ctx_t *c, uint pstate_index, int socket);
/** */
state_t mgt_imcfreq_set_auto(ctx_t *c);
/** Min is top limit, it meas higher frequencies and lower PSATEs. Max the opposite. */
state_t mgt_imcfreq_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list);
/** Min is top limit, it meas higher frequencies and lower PSATEs. Max the opposite. */
state_t mgt_imcfreq_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list);
/** */
state_t mgt_imcfreq_data_alloc(pstate_t **pstate_list, uint **index_list);
/** */
void mgt_imcfreq_data_print(pstate_t *ps_list, uint ps_count, int fd);
/** */
char *mgt_imcfreq_data_tostr(pstate_t *ps_list, uint ps_count, char *buffer, int length);

#endif //MANAGEMENT_IMCFREQ_H
