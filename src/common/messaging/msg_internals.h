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

#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <common/config.h>
#include <common/types/risk.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/messaging/msg_conf.h>

/** Non-blocking version of write*/
int _write(int fd, void *data, size_t ssize);

/** Connects with the EARD running in the given nodename. The current implementation supports a single command per connection
*	The sequence must be : connect +  command + disconnect
* 	@param the nodename to connect with
*/
int eards_remote_connect(char *nodename,uint port);

/** Disconnect from the previously connected EARD
*/
int eards_remote_disconnect();

/** Sends the command to the currently connected fd */
int send_command(request_t *command);

/** Sends data of size size through the open fd*/
int send_data(int fd, size_t size, char *data, int type);

/** Sends data of size size through the open fd but returns an error if it would block the daemon*/
int send_non_block_data(int fd, size_t size, char *data, int type);

/** Sends the command to all nodes in ear.conf */
void send_command_all(request_t command, cluster_conf_t *my_cluster_conf);

/** Corrects a propagation error, sending to the child nodes when the parent isn't responding. */
void correct_error(int target_idx, int total_ips, int *ips, request_t *command, uint port);

request_header_t correct_data_prop(int target_idx, int total_ips, int *ips, request_t *command, uint port, void **data);

#if NODE_PROP
request_header_t data_nodelist(request_t *command, cluster_conf_t *my_cluster_conf, void **data);

void internal_send_command_nodes(request_t *command, int port, int base_distance, int num_sends);

void send_command_nodelist(request_t *command, cluster_conf_t *my_cluster_conf);

void send_command_all(request_t command, cluster_conf_t *my_cluster_conf);

void correct_error_nodes(request_t *command, int self_ip, uint port);

request_header_t correct_data_prop_nodes(request_t *command, int self_ip, uint port, void **data);
#endif

/** Recieves data from a previously send command */
request_header_t receive_data(int fd, void **data);

request_header_t process_data(request_header_t data_head, char **temp_data_ptr, char **final_data_ptr, int final_size);

request_header_t data_all_nodes(request_t *command, cluster_conf_t *my_cluster_conf, void **data);


/* Server API internals */
int create_server_socket(uint port);
int wait_for_client(int sockfd,struct sockaddr_in *client);
void close_server_socket(int sock);

int read_command(int s,request_t *command);
void send_answer(int s,long *ack);
#endif
