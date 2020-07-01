/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
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
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/eard_conf_rapi.h>

/** Connects with the EARD running in the given nodename. The current implementation supports a single command per connection
*	The sequence must be : connect +  command + disconnect
* 	@param the nodename to connect with
*/
int eards_remote_connect(char *nodename,uint port);

/** Notifies the EARD the job with job_id starts the execution. It is supposed to be used by the EAR slurm plugin
*/
int eards_new_job(application_t *new_job);

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
void ping_all_nodes(cluster_conf_t my_cluster_conf);

/** Executes a simple ping to all nodes in a sequential manner */
void old_ping_all_nodes(cluster_conf_t my_cluster_conf);

/** Executes a simple ping to all nodes with the next nodes calculated at runtime */
void new_ping_all_nodes(cluster_conf_t my_cluster_conf);

int status_all_nodes(cluster_conf_t my_cluster_conf, status_t **status);

/** Sends the command to the currently connected fd */
int send_command(request_t *command);

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
#if USE_NEW_PROP
void correct_error(int target_idx, int total_ips, int *ips, request_t *command, uint port);
#else
void correct_error(uint target_ip, request_t *command, uint port);
#endif

/** Corrects a status propagation error, sending to the child nodes when the parent isn't responding. 
*   The corresponding status are placed in status, while the return value is the amount of status obtained. */
#if USE_NEW_PROP
int correct_status(int target_idx, int total_ips, int *ips, request_t *command, uint port, status_t **status);
#else
int correct_status(uint target_ip, request_t *command, uint port, status_t **status);
#endif

/** Sends the status command through the currently open fd, reads the returning value and places it
*   in **status. Returns the amount of status_t placed in **status. */
int send_status(request_t *command, status_t **status);

void correct_error_starter(char *host_name, request_t *command, uint port);
#endif
