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

#ifndef _EARD_SERVER_API_H
#define _EARD_SERVER_API_H
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <daemon/eard_conf_rapi.h>


int create_server_socket(uint port);
int wait_for_client(int sockfd,struct sockaddr_in *client);
void close_server_socket(int sock);

int read_command(int s,request_t *command);
void send_answer(int s,long *ack);
void propagate_req(request_t *command, uint port);
int propagate_status(request_t *command, uint port, status_t **status);
#if POWERCAP
int propagate_release_idle(request_t *command, uint port, pc_release_data_t *release);
int propagate_powercap_status(request_t *command, uint port, powercap_status_t **status);
#endif

int init_ips(cluster_conf_t *my_conf);
void close_ips();

#endif

