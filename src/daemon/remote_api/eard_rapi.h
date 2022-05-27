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

/**
*    \file remote_daemon_client.h
*    \brief This file defines the client side of the remote EARD API
*
* 	 Note:All these funcions applies to a single node . Global commands must be applying by sending commands to all nodes. 
*/

#ifndef _REMOTE_CLIENT_API_H
#define _REMOTE_CLIENT_API_H


#include <common/config.h>
#include <common/types/risk.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/messaging/msg_internals.h>

#include <daemon/remote_api/eard_rapi_internals.h>


/* Generic function. USE THIS BEFORE SPECIFIC ONES (if available) */
/* If **nodes is NULL or num_nodes < 1 the command will be send to every node in the cluster.
 * Otherwise, the node will be send to the nodes in **nodes. If there is only 1 node
 * calling the function is equivalent to calling remote_connect, the specific function and 
 * remote_disconnect. */
int ear_get_powercap_status(cluster_conf_t *conf, powercap_status_t **pc_status, int release_power, char **nodes, int num_nodes);
int ear_get_status(cluster_conf_t *conf, status_t **status, char **nodes, int num_nodes);
int ear_get_power(cluster_conf_t *my_cluster_conf, power_check_t *power, char **nodes, int num_nodes);
int ear_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt, char **nodes, int num_nodes);
int ear_get_app_master_status(cluster_conf_t *conf, app_status_t **app_status, char **nodes, int num_nodes);
int ear_get_app_node_status(cluster_conf_t *conf, app_status_t **app_status, char **nodes, int num_nodes);

void ear_set_powerlimit(cluster_conf_t *my_cluster_conf, unsigned long limit, char **nodes, int num_nodes);

#endif
