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

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <common/system/sockets.h>
#include <common/output/debug.h>

static pthread_mutex_t lock_send = PTHREAD_MUTEX_INITIALIZER;

state_t sockets_accept(int req_fd, int *cli_fd, struct sockaddr_storage *cli_addr)
{
	struct sockaddr_storage _cli_addr;
	struct sockaddr_storage *p = &_cli_addr;
	socklen_t size = sizeof (struct sockaddr_storage);
	//int flags = 0;

	if (cli_addr != NULL) {
		p = cli_addr;
	}

	*cli_fd = accept(req_fd, (struct sockaddr *) p, &size);

	//
	if (*cli_fd == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			state_return_msg(EAR_NOT_READY, errno, strerror(errno));
		} else {
			state_return_msg(EAR_SOCK_OP_ERROR, errno, strerror(errno));
		}
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_listen(socket_t *socket)
{
	if (socket->protocol == UDP) {
		state_return_msg(EAR_BAD_ARGUMENT, 0, "UDP port can't be listened");
	}

	// Listening the port
	int r = listen(socket->fd, BACKLOG);

	if (r < 0) {
		state_return_msg(EAR_SOCK_OP_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_bind(socket_t *socket, time_t timeout)
{
	timeout = time(NULL) + timeout;
	int s;

	do {
		// Assign to the socket the address and port
		s = bind(socket->fd, socket->info->ai_addr, socket->info->ai_addrlen);
	}
	while ((s < 0) && (time(NULL) < timeout) && !sleep(5));

	if (s < 0) {
		state_return_msg(EAR_SOCK_OP_ERROR, errno, (char *) strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_socket(socket_t *sock)
{
	struct addrinfo hints;
	char c_port[16];
	int r;

	// Format
	sprintf(c_port, "%u", sock->port);
	memset(&hints, 0, sizeof (hints));

	//
	hints.ai_socktype = sock->protocol; // TCP stream sockets
	hints.ai_family = AF_UNSPEC;		// Don't care IPv4 or IPv6

	if (sock->host == NULL) {
		hints.ai_flags = AI_PASSIVE;    // Fill in my IP for me
	}

	if ((r = getaddrinfo(sock->host, c_port, &hints, &sock->info)) != 0) {
		state_return_msg(EAR_ADDR_NOT_FOUND, errno, (char *) gai_strerror(r));
	}

	//
	sock->fd = socket(sock->info->ai_family, sock->info->ai_socktype, sock->info->ai_protocol);

	if (sock->fd < 0) {
		state_return_msg(EAR_SOCK_OP_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_init(socket_t *socket, char *host, uint port, uint protocol)
{
	//state_t s;

	if (protocol != TCP && protocol != UDP) {
		state_return_msg(EAR_BAD_ARGUMENT, 0, "protocol does not exist");
	}

	if (host != NULL) {
		socket->host = strcpy(socket->host_dst, host);
	} else {
		socket->host = NULL;
	}

	socket->fd = -1;
	socket->port = port;
	socket->info = NULL;
	socket->protocol = protocol;

	state_return(EAR_SUCCESS);
}

state_t sockets_dispose(socket_t *socket)
{
	sockets_close_fd(socket->fd);

	if (socket->info != NULL) {
        freeaddrinfo(socket->info);
        socket->info = NULL;
    }

	return sockets_clean(socket);
}

state_t sockets_clean(socket_t *socket)
{
	memset((void *) socket, 0, sizeof(socket_t));
    socket->fd = -1;

    state_return(EAR_SUCCESS);
}

state_t sockets_connect(socket_t *socket)
{
	int r;

	// Assign to the socket the address and port
	r = connect(socket->fd, socket->info->ai_addr, socket->info->ai_addrlen);

	if (r < 0) {
		state_return_msg(EAR_SOCK_OP_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_close(socket_t *socket)
{
	state_t s;

	s = sockets_close_fd(socket->fd);
	socket->fd = -1;

	state_return(s);
}

state_t sockets_close_fd(int fd)
{
	if (fd > 0) {
		close(fd);
	}

	state_return(EAR_SUCCESS);
}

/*
 *
 * Read & write
 *
 */

// Send:
//
// Errors:
//	Returning EAR_BAD_ARGUMENT
//	 - Header or buffer pointers are NULL
//	Returning EAR_SOCK_DISCONNECTED
// 	 - On ENETUNREACH, ENETDOWN, ECONNRESET, ENOTCONN, EPIPE and EDESTADDRREQ
//	Returning EAR_NOT_READY
//	 - On EAGAIN
//	Returning EAR_ERROR
//	 - On EACCES, EBADF, ENOTSOCK, EOPNOTSUPP, EINTR, EIO and ENOBUFS

static state_t _send(socket_t *socket, ssize_t bytes_expc, char *buffer)
{
	ssize_t bytes_left = bytes_expc;
	ssize_t bytes_sent = 0;
	int flags;

	// Better to return error than receive a signal
	flags = MSG_NOSIGNAL;

	// In the near future, it will be better to include a non-blocking
	// implementation with a controlled retry system
	while(bytes_left > 0)
	{
		if (socket->protocol == TCP) {
			bytes_sent = send(socket->fd, buffer + bytes_sent, bytes_left, flags);
		} else {
			bytes_sent = sendto(socket->fd, buffer + bytes_sent, bytes_left, flags,
				socket->info->ai_addr, socket->info->ai_addrlen);
		}
		
		if (bytes_sent < 0)
		{
			debug("fd '%d', %ld to add to %ld/%ld (errno: %d, strerrno: %s)",
				  socket->fd, bytes_sent, bytes_expc - bytes_left, bytes_expc,
				  errno, strerror(errno));

			switch (bytes_sent) {
				case ENETUNREACH:
				case ENETDOWN:
				case ECONNRESET:
				case ENOTCONN:
				case EPIPE:
				case EDESTADDRREQ:
					state_return_msg(EAR_SOCK_DISCONNECTED, errno, strerror(errno));
				case EAGAIN:
					state_return_msg(EAR_NOT_READY, errno, strerror(errno));
				default:
					state_return_msg(EAR_SOCK_OP_ERROR, errno, strerror(errno));
			}
		}

		bytes_left -= bytes_sent;
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_send(socket_t *socket, packet_header_t *header, char *content)
{
	char output_buffer[SZ_BUFF_BIG];
	packet_header_t *output_header;
	char *output_content;
	state_t state;

	// Error handling
	if (socket->fd < 0) {
		state_return_msg(EAR_SOCK_DISCONNECTED, 0, "invalid file descriptor");
	}

	if (header == NULL || content == NULL) {
		state_return_msg(EAR_BAD_ARGUMENT, 0, "passing parameter can't be NULL");
	}

	// Output
	output_header = PACKET_HEADER(output_buffer);
	output_content = PACKET_CONTENT(output_buffer);

	// Copy process
	memcpy(output_header, header, sizeof(packet_header_t));
	memcpy(output_content, content, header->content_size);

	pthread_mutex_lock(&lock_send);
	{
		// Sending
		state = _send(socket, sizeof(packet_header_t) + header->content_size, output_buffer);
		
		pthread_mutex_unlock(&lock_send);
	}

	return state;
}

// Receive:
//
// Errors:
//	Returning EAR_BAD_ARGUMENT
//	 - File descriptor less than 0
//	 - Header or buffer pointers are NULL
//	 - Buffer size is less than the packet content size
//	Returning EAR_SOCK_DISCONNECTED
//	 - Received 0 bytes (meaning disconnection)
//	 - On ECONNRESET, ENOTCONN and ETIMEDOUT
//	Returning EAR_NOT_READY
//	 - On EAGAIN and EINVAL
//	Returning EAR_ERROR
//	 - On EBADF, ENOTSOCK, EOPNOTSUPP, EINTR, EIO, ENOBUFS and ENOMEM

static void _spin_delay()
{
	volatile int i;

	for (i = 0; i < 10000; ++i)
	{}
}

static state_t _receive(int fd, ssize_t bytes_expc, char *buffer, int block)
{
	ssize_t bytes_left = bytes_expc;
	ssize_t bytes_recv = 0;
	ssize_t bytes_acum = 0;
	int intents = 0;
	int flags = 0;

	if (!block) {
		flags = MSG_DONTWAIT;
	}

	while (bytes_left > 0)
	{
		bytes_recv = recv(fd, (void *) &buffer[bytes_acum], bytes_left, flags);

		if (bytes_recv == 0) {
			state_return_msg(EAR_SOCK_DISCONNECTED, 0, "disconnected from socket");
		}
		else if (bytes_recv < 0)
		{
			debug("fd '%d', %ld to add to %ld/%ld (intent: %d, errno: %d, strerrno: %s)",
				  fd, bytes_recv, bytes_expc - bytes_left, bytes_expc,
				  intents, errno, strerror(errno));

			switch (errno) {
				case ENOTCONN:
				case ETIMEDOUT:
				case ECONNRESET:
					state_return_msg(EAR_SOCK_DISCONNECTED, errno, strerror(errno));
					break;
				case EINVAL: // Not sure about this
				case EAGAIN:
					if (!block && intents < NON_BLOCK_TRYS) {
						intents += 1;
					} else {
						state_return_msg(EAR_TIMEOUT, errno, strerror(errno));
					}

					#ifndef SHOW_DEBUGS
					_spin_delay();
					#endif
					break;
				default:
					state_return_msg(EAR_SOCK_OP_ERROR, errno, strerror(errno));
			}
		} else {
			bytes_acum += bytes_recv;
			bytes_left -= bytes_recv;
		}
	}

	state_return(EAR_SUCCESS);
}

state_t sockets_receive(int fd, packet_header_t *header, char *buffer, ssize_t size_buffer, int block)
{
	state_t state;

	// Error handling
	if (fd < 0) {
		state_return_msg(EAR_SOCK_DISCONNECTED, 0, "invalid file descriptor");
	}

	if (header == NULL || buffer == NULL) {
		state_return_msg(EAR_BAD_ARGUMENT, 0, "passing parameter can't be NULL");
	}

	// Receiving the header
	state = _receive(fd, sizeof(packet_header_t), (char *) header, block);

	if (state_fail(state)) {
		state_return(state);
	}

	if (header->content_size > size_buffer) {
		state_return(EAR_NO_RESOURCES);
	}

	// Receiving the content
	state = _receive(fd, header->content_size, buffer, block);

	if (state_fail(state)) {
		state_return(state);
	}

	state_return(EAR_SUCCESS);
}

/*
 *
 * Timeout
 *
 */

state_t sockets_timeout_get(int fd, time_t *timeout)
{
	struct timeval tv;
	socklen_t length;

	if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &length) != 0)
	{
		state_return_msg(EAR_ERROR, errno, strerror(errno));
		*timeout = 0;
	}

	*timeout = (time_t) tv.tv_sec;
	state_return(EAR_SUCCESS);
}

state_t sockets_timeout_set(int fd, time_t timeout)
{
	struct timeval tv;

	tv.tv_sec  = timeout;
	tv.tv_usec = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(struct timeval)) != 0) {
		state_return_msg(EAR_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

/*
 *
 * Non-blocking
 *
 */

state_t sockets_nonblock_set(int fd)
{
	fcntl(fd, F_SETFL, O_NONBLOCK);

	state_return(EAR_SUCCESS);
}

state_t sockets_nonblock_clean(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

	state_return(EAR_SUCCESS);
}

/*
 *
 * Header
 *
 */

state_t sockets_header_clean(packet_header_t *header)
{
	memset((void *) header, 0, sizeof(packet_header_t));
	state_return(EAR_SUCCESS);
}

state_t sockets_header_update(packet_header_t *header)
{
	if (strlen(header->host_src) == 0) {
		gethostname(header->host_src, sizeof(header->host_src));
	}

	header->timestamp = time(NULL);
	state_return(EAR_SUCCESS);
}

/*
 *
 * Addresses
 *
 */

void sockets_get_hostname(struct sockaddr_storage *host_addr, char *buffer, int length)
{
	socklen_t addrlen;

	if (length < INET6_ADDRSTRLEN) {
		return;
	}

	// IPv4
	if (host_addr->ss_family == AF_INET) {
		//struct sockaddr_in *ipv4 = (struct sockaddr_in *) host_addr;
		addrlen = INET_ADDRSTRLEN;
	}
	// IPv6
	else {
		//struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) host_addr;
		addrlen = INET6_ADDRSTRLEN;
	}

	// convert the IP to a string and print it
	getnameinfo((struct sockaddr *) host_addr, addrlen, buffer, length, NULL, 0, 0);
}

void sockets_get_hostname_fd(int fd, char *buffer, int length)
{
	struct sockaddr_storage s_addr;
	socklen_t s_len;

	s_len = sizeof(struct sockaddr_storage);
	memset(&s_addr, 0, s_len);

	getpeername(fd, (struct sockaddr *) &s_addr, &s_len);
	sockets_get_hostname(&s_addr, buffer, length);
}

void sockets_get_ip(struct sockaddr_storage *host_addr, long *ip)
{
	//socklen_t addrlen;
	//void *address;
	//int port;

	// IPv4
	if (host_addr->ss_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *) host_addr;
		//port = (int) ntohs(ipv4->sin_port);
		//addrlen = INET_ADDRSTRLEN;
		//address = &(ipv4->sin_addr);
		*ip = ipv4->sin_addr.s_addr;
	}
	// IPv6
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) host_addr;
		//port = (int) ntohs(ipv6->sin6_port);
		//addrlen = INET6_ADDRSTRLEN;
		//address = &(ipv6->sin6_addr);
		*ip = (long) ipv6->sin6_addr.s6_addr[8];
	}

	// convert the IP to a string and print it
	//inet_ntop(host_addr->sa_family, address, buffer, INET6_ADDRSTRLEN);
}
