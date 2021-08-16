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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <daemon/remote_api/eard_rapi.h>
#include <common/types/configuration/cluster_conf.h>


#ifndef _ENERGY_CLUSTER_STATUS_H
#define _ENERGY_CLUSTER_STATUS_H

#define EARGM_NO_PROBLEM  3
#define EARGM_WARNING1  2
#define EARGM_WARNING2  1
#define EARGM_PANIC   0


typedef struct node_info{
	uint dist_pstate;
	int ip;
	float power_red;
	uint victim;
	uint idle;
}node_info_t;

state_t get_nodes_status(cluster_conf_t my_cluster_conf,uint *nnodes,node_info_t **einfo);
void manage_warning(risk_t * risk,uint level,cluster_conf_t my_cluster_conf,float target,uint mode);
void create_risk(risk_t *my_risk,int wl);


#endif


