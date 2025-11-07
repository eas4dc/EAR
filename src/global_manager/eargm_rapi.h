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
 *    \brief This file defines the client side of the remote EAR API
 *
 * 	 Note:Specific functions could be substituted by a generic function passing a local_config_t
 */

#ifndef _EARGM_API_H
#define _EARGM_API_H

#include <global_manager/meta_eargm.h>

/** Notifies the EARGM a new job using N nodes will start. It is supposed to be used by the EAR slurm plugin
 */
int eargm_new_job(uint num_nodes);

/** Notifies the EARGM  a job using N nodes is finishing
 */
int eargm_end_job(uint num_nodes);

/* Increases the EARGM's powercap allocation by increase*/
int eargm_inc_powercap(uint increase);

/* Reduces the EARGM's powercap allocation by decrease*/
int eargm_red_powercap(uint decrease);

/* Resets the EARGM's powercap to its default value (from ear.conf) */
int eargm_reset_powercap();

/* Internal function that sends a new power limit */
int eargm_send_set_powerlimit(uint limit);

/** Disconnect from the previously connected EARGM
 */
int eargm_disconnect();

/** Given a fd, sends a request for an eargm_status and reads it. Returns the number of eargm_status retrieved */
int eargm_status(int fd, eargm_status_t **status);

/** Sends the appropriate message to every sub-eargm on the list */
int eargm_send_table_commands(eargm_table_t *egm_table);

/** Gets all the EARGM status from the EARGMs under eargm_idx control*/
int eargm_get_all_status(cluster_conf_t *conf, eargm_status_t **status, int eargm_idx);

/** Gets all the EARGM status in the cluster */
int eargm_cluster_get_status(cluster_conf_t *conf, eargm_status_t **status);

/** If hosts is not NULL, gets all the EARGM status of the hostnames in hosts. Otherwise it gets all the EARGMs status
 *  in the cluster*/
int eargm_get_status(cluster_conf_t *conf, eargm_status_t **status, char **hosts, int num_hosts);

/** Gets all the EARGM status running in hosts*/
int eargm_nodelist_get_status(cluster_conf_t *conf, eargm_status_t **status, char **hosts, int num_hosts);

/* Sets the all the EARGMs powercap allocation to limit if hosts is NULL, if not it sets it only for the EARGM
 * in hosts. */
void eargm_set_powerlimit(cluster_conf_t *conf, uint limit, char **hosts, int num_hosts);

/* Sets the powercap allocation of all EARGMs in hosts to limit */
void eargm_nodelist_set_powerlimit(cluster_conf_t *conf, uint limit, char **hosts, int num_hosts);

/* Sets the powercap allocation of all EARGMs in the cluster. */
void eargm_cluster_set_powerlimit(cluster_conf_t *conf, uint limit);

#endif
