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

#define NEW_STATUS 1
#include <common/config.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/eard_conf_rapi.h>
#include <common/types/risk.h>

/** Connects with the EARD running in the given nodename. The current implementation supports a single command per connection
*	The sequence must be : connect +  command + disconnect
* 	@param the nodename to connect with
*/
int eards_remote_connect(char *nodename,uint port);

/** Notifies the EARD the job with job_id starts the execution. It is supposed to be used by the EAR slurm plugin
*/
int eards_new_job(new_job_req_t  *new_job);

/** Notifies the EARD the job with job_id ends the execution. It is supposed to be used by the EAR slurm plugin
*/
//int eards_end_job(job_id jid,job_id sid,int status);
int eards_end_job(job_id jid,job_id sid);

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


/** gets the policy_configuration for a given policy 
 * */
int eards_get_policy_info(new_policy_cont_t *p);
/** set the policy_configuration for a given policy 
 * */
int eards_set_policy_info(new_policy_cont_t *p);



/** Disconnect from the previously connected EARD
*/
int eards_remote_disconnect();

/** Increases the current threshold to all nodes in my_cluster_conf. */
void increase_th_all_nodes(ulong th, ulong p_id, cluster_conf_t my_cluster_conf);

/** Sets the maximum and default freq to all nodes in my_cluster_conf. */
void set_max_freq_all_nodes(ulong ps, cluster_conf_t my_cluster_conf);

/** Reduces the default freq to all nodes in my_cluster_conf. */
void red_def_max_pstate_all_nodes(ulong ps, cluster_conf_t my_cluster_conf);

/** Reduces the frequecy of all nodes, using set frequenchy. */
void reduce_frequencies_all_nodes(ulong freq, cluster_conf_t my_cluster_conf);

/** Sets the default frequency for all nodes in my_custer_conf. */
void set_def_freq_all_nodes(ulong freq, ulong policy, cluster_conf_t my_cluster_conf);

/** Restores the default configuration for all nodes in my_cluster_conf. */
void restore_conf_all_nodes(cluster_conf_t my_cluster_conf);

/** Executes a simple ping to all nodes */
void ping_all_nodes_propagated(cluster_conf_t my_cluster_conf);

/** Executes a simple ping to all nodes in a sequential manner */
void ping_all_nodes(cluster_conf_t my_cluster_conf);

/** Asks all the nodes for their current status */
int status_all_nodes(cluster_conf_t my_cluster_conf, status_t **status);




/** Sends the command to the currently connected fd */
int send_command(request_t *command);

/** Sends data of size size through the open fd*/
int send_data(int fd, size_t size, char *data, int type);

/** Sets frequency for all nodes. */
void set_freq_all_nodes(ulong freq, cluster_conf_t my_cluster_conf);

/** Sets the default pstate for a given policy in all nodes */
void set_def_pstate_all_nodes(uint pstate,ulong policy,cluster_conf_t my_cluster_conf);

/** Sets the maximum pstate in all the nodes */
void set_max_pstate_all_nodes(uint pstate,cluster_conf_t my_cluster_conf);

/** Sets the th to all nodes. */
void set_th_all_nodes(ulong th, ulong p_id, cluster_conf_t my_cluster_conf);

/** Sends the command to all nodes in ear.conf */
void send_command_all(request_t command, cluster_conf_t my_cluster_conf);

/** Corrects a propagation error, sending to the child nodes when the parent isn't responding. */
void correct_error(int target_idx, int total_ips, int *ips, request_t *command, uint port);

/** Sends the status command through the currently open fd, reads the returning value and places it
*   in **status. Returns the amount of status_t placed in **status. */
int send_status(request_t *command, status_t **status);

void correct_error_starter(char *host_name, request_t *command, uint port);

request_header_t correct_data_prop(int target_idx, int total_ips, int *ips, request_t *command, uint port, void **data);

/** Recieves data from a previously send command */
request_header_t recieve_data(int fd, void **data);

request_header_t process_data(request_header_t data_head, char **temp_data_ptr, char **final_data_ptr, int final_size);

int eards_set_risk(risk_t risk,unsigned long target);
void set_risk_all_nodes(risk_t risk, unsigned long target, cluster_conf_t my_cluster_conf);
#endif
