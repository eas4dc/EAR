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

#ifndef _EARGM_SERVER_API_H
#define _EARGM_SERVER_API_H

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <global_manager/eargm_conf_api.h>

int create_server_socket(uint use_port);
int wait_for_client(int sockfd,struct sockaddr_in *client);
void close_server_socket(int sock);

int read_command(int s,eargm_request_t *command);
void send_answer(int s,ulong *ack);

#endif

