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

#ifndef _REMOTE_CLIENT_API_INTERNALS_H
#define _REMOTE_CLIENT_API_INTERNALS_H

#include <common/config.h>
#include <common/types/risk.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>

#include <common/messaging/msg_conf.h>
#include <common/messaging/msg_internals.h>

/********************************************************/
/*                      IMPORTANT                       */
/*                      ---------                       */
/* ear_node_xxx type functions require manually calling */
/* remote_connect and remote_disconnect before and      */
/* after using them, respectively (both functions can   */ 
/* be found in common/messaging/msg_internals.          */
/********************************************************/

/**  Sets freq as the maximim frequency to be used in the node where the API is connected with
 * 	 Return 0 on success, 1 on error.
 */
int ear_node_set_max_freq(ulong freq);

/**  Sets the frequency of all the cores to freq 
 * 	 Return 0 on success, 1 on error.
 * */
int ear_node_set_freq(unsigned long freq);

/** Sets temporally the default frequency of all the policies to freq 
 * 	 Return 0 on success, 1 on error.
 * */
int ear_node_set_def_freq(unsigned long freq);

/**  Reduce the maximum and default freq by the given number of p_states
 * 	 Return 0 on success, 1 on error.
 */
int ear_node_red_max_and_def_freq(uint p_states);

/**  Restores the configuration originally set in ear.conf (it doesn´t read the file again)
 *	 Return 0 on success, 1 on error.
 */
int ear_node_restore_conf();

/** Sets th as the new threashold to be used by the policy. New th must be passed as % th=0.75 --> 75. It is designed to be used by the min_time_to_solution policy
 *	 Return 0 on success, 1 on error.
 */
int ear_node_set_th(ulong th);

/** Increases the current threshold by th units.New th must be passed as % th=0.05 --> 5. It is designed to be used by the min_time_to_solution policy
 *	 Return 0 on success, 1 on error.
 */
int ear_node_inc_th(ulong th);

/** Sends a local ping 
 *	 Return 0 on success, 1 on error.
 */
int ear_node_ping();

/** Sends a new job request 
 *	 Return 0 on success, 1 on error.
 */
int ear_node_new_job(new_job_req_t *new_job);

/** Sends an end job request
 *	 Return 0 on success, 1 on error.
 */
int ear_node_end_job(job_id jid,job_id sid);

/** Sends a new task request 
 *	 Return 0 on success, 1 on error.
 */
int ear_node_new_task(new_task_req_t *new_task);

/** Sends an end task request 
 *	 Return 0 on success, 1 on error.
 */
int ear_node_end_task(end_task_req_t *end_task);

/** gets the policy_configuration for a given policy 
 *	 Return 0 on success, 1 on error.
 * */
int ear_node_get_policy_info(new_policy_cont_t *p);
/** set the policy_configuration for a given policy 
 *	 Return 0 on success, 1 on error.
 * */
int ear_node_set_policy_info(new_policy_cont_t *p);

/** Increases the current threshold to all nodes in my_ear_cluster_conf. */
void ear_cluster_increase_th(cluster_conf_t *my_cluster_conf, ulong th, ulong p_id);

/** Sets the maximum and default freq to all nodes in my_ear_cluster_conf. */
void ear_cluster_set_max_freq(cluster_conf_t *my_cluster_conf, ulong ps);

/** Reduces the default freq to all nodes in my_ear_cluster_conf. */
void ear_cluster_red_def_max_pstate(cluster_conf_t *my_cluster_conf, ulong ps);

/** Reduces the frequecy of all nodes, using set frequenchy. */
void ear_cluster_reduce_frequencies(cluster_conf_t *my_cluster_conf, ulong freq);

/** Sets the default frequency for all nodes in my_custer_conf. */
void ear_cluster_set_def_freq(cluster_conf_t *my_cluster_conf, ulong freq, ulong policy );

/** Restores the default configuration for all nodes in my_ear_cluster_conf. */
void ear_cluster_restore_conf(cluster_conf_t *my_cluster_conf);

/** Executes a simple ping to all nodes */
void ear_cluster_ping_propagated(cluster_conf_t *my_cluster_conf);

/** Executes a simple ping to all nodes in a sequential manner */
void ear_cluster_ping(cluster_conf_t *my_cluster_conf);

/** Asks all the nodes for their current status */
int ear_cluster_get_status(cluster_conf_t *my_cluster_conf, status_t **status);
int ear_node_get_status(cluster_conf_t *my_cluster_conf,status_t **status);

/** Asks application status to all nodes or single node */
int ear_cluster_get_app_node_status(cluster_conf_t *my_cluster_conf, app_status_t **status);
int ear_node_get_app_node_status(cluster_conf_t *my_cluster_conf,app_status_t **status);

/** Asks application status to all master nodes or single node */
int ear_cluster_get_app_master_status(cluster_conf_t *my_cluster_conf, app_status_t **status);
int ear_node_get_app_master_status(cluster_conf_t *my_cluster_conf,app_status_t **status);

/** Asks for powercap_status for all nodes */
int ear_cluster_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status, int release_power);

int ear_node_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status, int release_power);

/** Send powercap_options to all nodes */
int ear_cluster_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt);

/** Sets frequency for all nodes. */
void ear_cluster_set_freq(cluster_conf_t *my_cluster_conf, ulong freq);

/** Sets the default pstate for a given policy in all nodes */
void ear_cluster_set_def_pstate(cluster_conf_t *my_cluster_conf, uint pstate, ulong policy);

/** Sets the maximum pstate in all the nodes */
void ear_cluster_set_max_pstate(cluster_conf_t *my_cluster_conf, uint pstate);

/** Sets the th to all nodes. */
void ear_cluster_set_th(cluster_conf_t *my_cluster_conf, ulong th, ulong p_id);

/** Sends the status command through the currently open fd, reads the returning value and places it
 *   in **status. Returns the amount of status_t placed in **status. */
int send_status(request_t *command, status_t **status);

/* Power management extensions */
int ear_node_set_powerlimit(unsigned long limit);
int ear_node_red_powerlimit(unsigned int type, unsigned long limit);
int ear_node_inc_powerlimit(unsigned int type, unsigned long limit);
int ear_node_set_risk(risk_t risk, unsigned long target);

void ear_cluster_set_powerlimit(cluster_conf_t *my_cluster_conf, unsigned long limit);
void ear_cluster_red_powerlimit(cluster_conf_t *my_cluster_conf, unsigned int type, unsigned long limit);
void ear_cluster_inc_powerlimit(cluster_conf_t *my_cluster_conf, unsigned int type, unsigned long limit);


void ear_cluster_set_risk(cluster_conf_t *my_cluster_conf, risk_t risk, unsigned long target);

int ear_node_reset_default_powercap();
void ear_cluster_reset_default_powercap(cluster_conf_t *my_cluster_conf);

int ear_cluster_release_idle_power(cluster_conf_t *my_cluster_conf, pc_release_data_t *released);

/** Gets instant eards power 
 * Returns 0 on success, 1 on error
 * */

int ear_node_get_power(power_check_t *power);
/** Gets instant cluster power 
 * Return 0 0n success, 1 on error
 * */
int ear_cluster_get_power(cluster_conf_t *my_cluster_conf, power_check_t *power);


/* Nodelist functions, they do the same as their ear_cluster or ear_nodelist counterparts but only to the
 * nodes in the list. */
void ear_nodelist_ear_node_new_job(new_job_req_t *new_job, uint port, char *net_ext, char **nodes, int num_nodes);
void ear_node_new_job_iplist(new_job_req_t *new_job, cluster_conf_t *my_cluster_conf, int *ips, int num_ips);
void ear_nodelist_ear_node_end_job(job_id jid, job_id sid, uint port, char *net_ext, char **nodes, int num_nodes);
void ear_node_end_job_iplist(job_id jid, job_id sid, cluster_conf_t *my_clister_conf, int *ips, int num_ips);
void ear_nodelist_increase_th(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf, int *ips, int num_ips);
void ear_nodelist_restore_conf(cluster_conf_t *my_cluster_conf, int *ips, int num_ips);
void ear_nodelist_set_powerlimit(cluster_conf_t *my_cluster_conf, unsigned long limit, char **nodes, int num_nodes);
void ear_nodelist_set_risk(risk_t risk, unsigned long target, cluster_conf_t *my_cluster_conf, int *ips, int num_ips);
void ear_nodelist_set_default_powercap(cluster_conf_t *my_cluster_conf, int *ips, int num_ips);

/* The following functions return the number of status retrieved on success, 0 on error */
int iplist_status(cluster_conf_t *my_cluster_conf, status_t **status, int *ips, int num_ips);
int ear_nodelist_get_status(cluster_conf_t *my_cluster_conf, status_t **status, char **nodes, int num_nodes);
int ear_nodelist_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status, int release_power, char **nodes, int num_nodes);
int ear_nodelist_get_power(cluster_conf_t *my_cluster_conf, power_check_t *power, char **nodes, int num_nodes);
int ear_nodelist_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt, char **nodes, int num_nodes);
int ear_nodelist_get_app_master_status(cluster_conf_t *my_cluster_conf, app_status_t **status, char **nodes, int num_nodes);
int ear_nodelist_get_app_node_status(cluster_conf_t *my_cluster_conf, app_status_t **status, char **nodes, int num_nodes);

#endif
