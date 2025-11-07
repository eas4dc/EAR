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
 * 	 Note:All these funcions applies to a single node . Global commands must be applying by sending commands to all
 * nodes.
 */

#ifndef _REMOTE_CLIENT_API_H
#define _REMOTE_CLIENT_API_H

#include <common/config.h>
#include <common/messaging/msg_internals.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/risk.h>

#include <daemon/remote_api/eard_rapi_internals.h>

/* Generic function. USE THIS BEFORE SPECIFIC ONES (if available) */
/* If **nodes is NULL or num_nodes < 1 the command will be send to every node in the cluster.
 * Otherwise, the node will be send to the nodes in **nodes. If there is only 1 node
 * calling the function is equivalent to calling remote_connect, the specific function and
 * remote_disconnect. */

/* Gets powercap status. Returns EAR_SUCCESS or EAR_ERROR. */
state_t ear_get_powercap_status(cluster_conf_t *conf, powercap_status_t **pc_status, int release_power, char **nodes,
                                int num_nodes);
/* Gets hardware status. Sets *num_status to the number of status allocated. Returns EAR_SUCCESS or EAR_ERROR.*/
state_t ear_get_status(cluster_conf_t *conf, status_t **status, int32_t *num_status, char **nodes, int num_nodes);
/* Gets accumulated power. Returns EAR_SUCCESS or EAR_ERROR.  */
state_t ear_get_power(cluster_conf_t *my_cluster_conf, power_check_t *power, char **nodes, int num_nodes);

/* Sets the powercap options. Returns EAR_SUCCESS on success, or EAR_ERROR on errors. */
state_t ear_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt, char **nodes, int num_nodes);
/* Gets application status (only from master nodes). Sets *num_status to the number of status allocated. Returns
 * EAR_SUCCESS or EAR_ERROR*/
state_t ear_get_app_master_status(cluster_conf_t *conf, app_status_t **app_status, int32_t *num_status, char **nodes,
                                  int num_nodes);
/* Gets application status (from every node that has an application running). Sets *num_status to the number of status
 * allocated. Returns EAR_SUCCESS or EAR_ERROR*/
state_t ear_get_app_node_status(cluster_conf_t *conf, app_status_t **app_status, int32_t *num_status, char **nodes,
                                int num_nodes);
state_t ear_get_policy_status(cluster_conf_t *conf, policy_status_t **status, int32_t *num_status, char **nodes,
                              int num_nodes);

/* Sets the powercap limit.*/
state_t ear_set_powerlimit(cluster_conf_t *my_cluster_conf, unsigned long limit, char **nodes, int num_nodes);
state_t ear_send_message(cluster_conf_t *conf, char *message, char **nodes, int num_nodes);

/* Restores configuration for nodes to ear.conf values */
state_t ear_restore_conf(cluster_conf_t *conf, char **nodes, int32_t num_nodes);

/* Sets powercap options */
state_t ear_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt, char **nodes, int num_nodes);

/* Sets risks and target to policies that have a set_risk function implemented. */
state_t ear_set_risk(cluster_conf_t *conf, risk_t risk, uint32_t target, char **nodes, int32_t num_nodes);

/* Sets the maximum node CPU frequency */
state_t ear_set_max_freq(cluster_conf_t *conf, uint32_t freq, char **nodes, int32_t num_nodes);
/* Sets a new current CPU frequency */
state_t ear_set_freq(cluster_conf_t *conf, uint32_t freq, char **nodes, int32_t num_nodes);
/* Sets a new default CPU frequency for the policy selected */
state_t ear_set_def_freq(cluster_conf_t *conf, uint32_t freq, int32_t policy_id, char **nodes, int32_t num_nodes);
/* Reduces both the maximum and default CPU frequency by N pstates */
state_t ear_red_max_and_def_freq(cluster_conf_t *conf, uint32_t pstates, char **nodes, int32_t num_nodes);

/* Pings the EARDs. Should print an error if it fails */
state_t ear_ping(cluster_conf_t *conf, char **nodes, int32_t num_nodes);
/* Tells the EARDs to release the current releasable power. Any power released is accumulated in pc_release_data_t
 * *released */
state_t ear_release_idle_power(cluster_conf_t *conf, pc_release_data_t *released, char **nodes, int32_t num_nodes);
#endif
