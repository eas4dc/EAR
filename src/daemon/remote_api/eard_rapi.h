/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
