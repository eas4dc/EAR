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

#ifndef EAR_SOCKETS_H
#define EAR_SOCKETS_H

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/types/generic.h>

#define BACKLOG				10
#define TCP					SOCK_STREAM
#define UDP					SOCK_DGRAM
#define NON_BLOCK_TRYS		1000

/* types */
typedef struct socket {
	struct addrinfo *info;
	char host_dst[SZ_NAME_SHORT];
	char *host;
	uint protocol;
	uint port;
	int fd;
} socket_t;

typedef struct packet_header {
	char host_src[SZ_NAME_SHORT]; // Filled in sockets_send()
	time_t timestamp; // Filled in sockets_send()
	size_t content_size; // Filled in sockets_send()
	uchar content_type;
	uchar data_extra1;
	uchar data_extra2;
	uchar data_extra3;
}  packet_header_t;

#define PACKET_HEADER(buffer) \
	(packet_header_t *) buffer;

#define PACKET_CONTENT(buffer) \
	(void *) &buffer[sizeof(packet_header_t)];

/* Initialization */
state_t sockets_init(socket_t *socket, char *host, uint port, uint protocol);

state_t sockets_dispose(socket_t *socket);

state_t sockets_clean(socket_t *socket);

state_t sockets_socket(socket_t *socket);

state_t sockets_bind(socket_t *socket, time_t timeout);

state_t sockets_listen(socket_t *socket);

state_t sockets_accept(int req_fd, int *cli_fd, struct sockaddr_storage *cli_addr);

state_t sockets_connect(socket_t *socket);

/* Closing */
state_t sockets_close(socket_t *socket);

state_t sockets_close_fd(int fd);

/* Write & read */
state_t sockets_send(socket_t *socket, packet_header_t *header, char *content);

state_t sockets_receive(int fd, packet_header_t *header, char *buffer, ssize_t size_buffer, int block);

/* Timeout */
state_t sockets_timeout_set(int fd, time_t timeout);

state_t sockets_timeout_get(int fd, time_t *timeout);

/* Non-blocking */
state_t sockets_nonblock_set(int fd);

state_t sockets_nonblock_clean(int fd);

/* Header */
state_t sockets_header_clean(packet_header_t *header);

state_t sockets_header_update(packet_header_t *header);

/* Addresses */
void sockets_get_hostname(struct sockaddr_storage *host_addr, char *buffer, int length);

void sockets_get_hostname_fd(int fd, char *buffer, int length);

void sockets_get_ip(struct sockaddr_storage *host_addr, long *ip);

#endif //EAR_SOCKETS_H
