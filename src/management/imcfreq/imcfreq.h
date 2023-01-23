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

#ifndef MANAGEMENT_IMCFREQ_H
#define MANAGEMENT_IMCFREQ_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/hardware/topology.h>
#include <metrics/common/pstate.h>

// The API
//
// This API is designed to set the frequency in the of the uncore or integrated
// memory controllers of the system.
//
// Props:
// 	- Thread safe: yes.
//	- Daemon bypass: yes.
//  - Dummy API: yes.
//
// Compatibility:
//  --------------------------------------------------------
//  | Architecture    | F/M | Comp. | Granularity | System |
//  --------------------------------------------------------
//  | Intel HASWELL   | 63  | v     | Socket      | MSR    |
//  | Intel BROADWELL | 79  | v     | Socket      | MSR    |
//  | Intel SKYLAKE   | 85  | v     | Socket      | MSR    |
//  | Intel ICELAKE   | 106 | v     | Socket      | MSR    |
//  | AMD ZEN+/2      | 17h | v     | Socket      | HSMP   |
//  | AMD ZEN3        | 19h | v     | Socket      | HSMP   |
//  -------------------------------------------------------

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
state_t mgt_imcfreq_load(topology_t *c, int eard, my_node_conf_t *conf);
/** Returns the loaded API. */
state_t mgt_imcfreq_get_api(uint *api);
/** */
state_t mgt_imcfreq_init(ctx_t *c);
/** */
state_t mgt_imcfreq_dispose(ctx_t *c);
/** Counts the number of devices (by now devices means sockets). */
state_t mgt_imcfreq_count_devices(ctx_t *c, uint *devs_count);
/** Get the available list of P_STATEs available. */
state_t mgt_imcfreq_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count);
/** Get the current list of P_STATEs in the IMCs. */
state_t mgt_imcfreq_get_current_list(ctx_t *c, pstate_t *pstate_list);
/** Sets a P_STATE in all IMCs given a list of P_STATE indexes. */
state_t mgt_imcfreq_set_current_list(ctx_t *c, uint *index_list);
/** Sets a P_STATE in a specific socket given a P_STATE index. Consider also all_devices constant. */
state_t mgt_imcfreq_set_current(ctx_t *c, uint pstate_index, int socket);
/** Returns to default values. Normally automatic frequency scalling by the system. */
state_t mgt_imcfreq_set_auto(ctx_t *c);
/** Min is top limit, it meas higher frequencies and lower PSATEs. Max the opposite. */
state_t mgt_imcfreq_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list);
/** Min is top limit, it meas higher frequencies and lower PSATEs. Max the opposite. */
state_t mgt_imcfreq_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list);
/** Allocates P_STATEs and indexes lists. */
state_t mgt_imcfreq_data_alloc(pstate_t **pstate_list, uint **index_list);
/** */
void mgt_imcfreq_data_print(pstate_t *ps_list, uint ps_count, int fd);
/** */
char *mgt_imcfreq_data_tostr(pstate_t *ps_list, uint ps_count, char *buffer, int length);

#endif //MANAGEMENT_IMCFREQ_H