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

#include <common/messaging/msg_conf.h>
#include <common/messaging/msg_internals.h>

/**  Sets freq as the maximim frequency to be used in the node where the API is connected with
*/
int eards_set_max_freq(ulong freq);

/** Sets the frequency of all the cores to freq */
int eards_set_freq(unsigned long freq);

/** Sets temporally the default frequency of all the policies to freq */
int eards_set_def_freq(unsigned long freq);

/**  Reduce the maximum and default freq by the given number of p_states
*/
int eards_red_max_and_def_freq(uint p_states);

/** Restores the configuration originally set in ear.conf (it doesn´t read the file again)*/
int eards_restore_conf();

/** Sets th as the new threashold to be used by the policy. New th must be passed as % th=0.75 --> 75. It is designed to be used by the min_time_to_solution policy
*/
int eards_set_th(ulong th);

/** Increases the current threshold by th units.New th must be passed as % th=0.05 --> 5. It is designed to be used by the min_time_to_solution policy
*/
int eards_inc_th(ulong th);

/** Sends a local ping */
int eards_ping();

/** Sends a new job request */
int eards_new_job(new_job_req_t *new_job);

/** Sends a end job request */
int eards_end_job(job_id jid,job_id sid);

/** gets the policy_configuration for a given policy 
 * */
int eards_get_policy_info(new_policy_cont_t *p);
/** set the policy_configuration for a given policy 
 * */
int eards_set_policy_info(new_policy_cont_t *p);


/** Increases the current threshold to all nodes in my_cluster_conf. */
void increase_th_all_nodes(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf);

/** Sets the maximum and default freq to all nodes in my_cluster_conf. */
void set_max_freq_all_nodes(ulong ps, cluster_conf_t *my_cluster_conf);

/** Reduces the default freq to all nodes in my_cluster_conf. */
void red_def_max_pstate_all_nodes(ulong ps, cluster_conf_t *my_cluster_conf);

/** Reduces the frequecy of all nodes, using set frequenchy. */
void reduce_frequencies_all_nodes(ulong freq, cluster_conf_t *my_cluster_conf);

/** Sets the default frequency for all nodes in my_custer_conf. */
void set_def_freq_all_nodes(ulong freq, ulong policy, cluster_conf_t *my_cluster_conf);

/** Restores the default configuration for all nodes in my_cluster_conf. */
void restore_conf_all_nodes(cluster_conf_t *my_cluster_conf);

/** Executes a simple ping to all nodes */
void ping_all_nodes_propagated(cluster_conf_t *my_cluster_conf);

/** Executes a simple ping to all nodes in a sequential manner */
void ping_all_nodes(cluster_conf_t *my_cluster_conf);

/** Asks all the nodes for their current status */
int status_all_nodes(cluster_conf_t *my_cluster_conf, status_t **status);
int eards_get_status(cluster_conf_t *my_cluster_conf,status_t **status);

/** Asks application status to all nodes or single node */
int get_app_node_status_all_nodes(cluster_conf_t *my_cluster_conf, app_status_t **status);
int eards_get_app_node_status(cluster_conf_t *my_cluster_conf,app_status_t **status);

/** Asks application status to all master nodes or single node */
int get_app_master_status_all_nodes(cluster_conf_t *my_cluster_conf, app_status_t **status);
int eards_get_app_master_status(cluster_conf_t *my_cluster_conf,app_status_t **status);

/** Asks for powercap_status for all nodes */
int cluster_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status);

int eards_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status);

/** Send powercap_options to all nodes */
int cluster_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt);

/** Sets frequency for all nodes. */
void set_freq_all_nodes(ulong freq, cluster_conf_t *my_cluster_conf);

/** Sets the default pstate for a given policy in all nodes */
void set_def_pstate_all_nodes(uint pstate,ulong policy,cluster_conf_t *my_cluster_conf);

/** Sets the maximum pstate in all the nodes */
void set_max_pstate_all_nodes(uint pstate,cluster_conf_t *my_cluster_conf);

/** Sets the th to all nodes. */
void set_th_all_nodes(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf);

/** Sends the status command through the currently open fd, reads the returning value and places it
*   in **status. Returns the amount of status_t placed in **status. */
int send_status(request_t *command, status_t **status);

/* Power management extensions */
int eards_set_powerlimit(unsigned long limit);
int eards_red_powerlimit(unsigned int type, unsigned long limit);
int eards_inc_powerlimit(unsigned int type, unsigned long limit);

void cluster_set_powerlimit_all_nodes(unsigned long limit, cluster_conf_t *my_cluster_conf);
void cluster_red_powerlimit_all_nodes(unsigned int type, unsigned long limit, cluster_conf_t *my_cluster_conf);
void cluster_inc_powerlimit_all_nodes(unsigned int type, unsigned long limit, cluster_conf_t *my_cluster_conf);


int eards_set_risk(risk_t risk,unsigned long target);
void set_risk_all_nodes(risk_t risk, unsigned long target, cluster_conf_t *my_cluster_conf);

int eards_reset_default_powercap();
void cluster_reset_default_powercap_all_nodes(cluster_conf_t *my_cluster_conf);

int cluster_release_idle_power(cluster_conf_t *my_cluster_conf, pc_release_data_t *released);

/* Nodelist functions */
void eards_new_job_nodelist(new_job_req_t *new_job, uint port, char *net_ext, char **nodes, int num_nodes);
void eards_new_job_iplist(new_job_req_t *new_job, cluster_conf_t *my_cluster_conf, int *ips, int num_ips);

void eards_end_job_nodelist(job_id jid, job_id sid, uint port, char *net_ext, char **nodes, int num_nodes);
void eards_end_job_iplist(job_id jid, job_id sid, cluster_conf_t *my_clister_conf, int *ips, int num_ips);

void increase_th_nodelist(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf, int *ips, int num_ips);

void restore_conf_nodelist(cluster_conf_t *my_cluster_conf, int *ips, int num_ips);

void set_risk_nodelist(risk_t risk, unsigned long target, cluster_conf_t *my_cluster_conf, int *ips, int num_ips);

void set_default_powercap_nodelist(cluster_conf_t *my_cluster_conf, int *ips, int num_ips);

int status_nodelist(cluster_conf_t *my_cluster_conf, status_t **status, int *ips, int num_ips);

#endif
