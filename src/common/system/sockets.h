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

#ifndef COMMON_SOCKETS_H
#define COMMON_SOCKETS_H

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/types/generic.h>

// Example:
//	Server side example:
//		// Socket structure initialization.
//		sockets_init(&socket, NULL, 8686, TCP);
//		// Creating the file descriptor.
//		sockets_socket(&socket);
//		// Attach the fd to the system port.
//		sockets_bind(&socket, 0);
//		// Start listening incoming connections.
//		sockets_listen(&socket);
//
//		// Then, after select, get the incoming metadata.
//		sockets_recv_header(socket.fd, &type, &data_size, &extra, 1);
//		// After the header information, read the data.
//		sockets_recv(socket.fd, buffer, data_size, 1);
//
//	Client side example:
//		// Socket structure initialization.
//		sockets_init(&socket, NULL, 8686, TCP);
//		// Creating the file descriptor.
//		sockets_socket(&socket);
//		// Connect with the server.
//		sockets_connect(&socket);
//		// Send data.
//		sockets_send(socket.fd, type, data, data_size, 0LLU);
//

#define BACKLOG             10
#define TCP                 SOCK_STREAM
#define UDP                 SOCK_DGRAM
#define NON_BLOCK_TRYS      1000

typedef struct socket {
	struct addrinfo *info;
	char             host_dst[SZ_NAME_SHORT];
	char            *host;
	uint             protocol;
	uint             port;
	uint             fd_closable;
	int              fd;
} socket_t;

/* Common initialization */
state_t sockets_init(socket_t *socket, char *host, uint port, uint protocol);

state_t sockets_clean(socket_t *socket);
/** Closes file descriptor and frees memory. */
state_t sockets_dispose(socket_t *socket);

/* Common side */
state_t sockets_socket(socket_t *socket);

/* Server side */
state_t sockets_bind(socket_t *socket, time_t timeout_secs);

state_t sockets_listen(socket_t *socket);

state_t sockets_accept(int fd_incoming, int *fd_client, struct sockaddr_storage *addr_client);

/* Client side */
state_t sockets_connect(socket_t *socket);

/* Closing/disconnect */
state_t sockets_close(socket_t *socket);

state_t sockets_close_fd(int fd);

/* Write & read*/
state_t sockets_send(int fd, uint type, char *data, size_t data_size, ullong extra);

state_t sockets_recv_header(int fd, uint *type, size_t *data_size, ullong *extra, uint block);

state_t sockets_recv(int fd, char *data, size_t data_size, uint block);

/* File descriptor options */
state_t sockets_timeout_set(int fd, time_t timeout);

state_t sockets_timeout_get(int fd, time_t *timeout);

state_t sockets_nonblock_set(int fd);

state_t sockets_nonblock_unset(int fd);

/* Addresses */
void sockets_get_hostname(struct sockaddr_storage *host_addr, char *buffer, int length);

void sockets_get_hostname_fd(int fd, char *buffer, int length);

void sockets_get_ip(struct sockaddr_storage *host_addr, long *ip);

/*
 *
 * Obsolete
 *
 */

#define packet_header_t socket_header_t

typedef struct socket_header_s {
#if SOCKETS_DEBUG
	char    host_src[SZ_NAME_SHORT]; // Filled in sockets_send()
    time_t  timestamp;               // Filled in sockets_send()
#endif
	size_t  content_size;  // 8 bytes ( 8)
	ullong  padding;       // 8 bytes (16)
	uint    content_type;  // 4 byte  (20)
}  socket_header_t;

#define PACKET_HEADER(buffer) \
	(socket_header_t *) buffer;

#define PACKET_CONTENT(buffer) \
	(void *) &buffer[sizeof(socket_header_t)];

/* (Obsolete) Header */
state_t sockets_header_clean(socket_header_t *header);

state_t sockets_header_update(socket_header_t *header);

/* (Obsolete) Write & read */
state_t __sockets_send(socket_t *socket, socket_header_t *header, char *content);

state_t __sockets_recv(int fd, socket_header_t *header, char *buffer, ssize_t size_buffer, int block);

#endif //COMMON_SOCKETS_H
