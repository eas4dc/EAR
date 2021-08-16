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

//#define SHOW_DEBUGS 1

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <common/output/debug.h>
#include <common/system/sockets.h>

static pthread_mutex_t lock_send = PTHREAD_MUTEX_INITIALIZER;

/*
 *
 * Common initialization
 *
 */
state_t sockets_init(socket_t *socket, char *host, uint port, uint protocol)
{
	if (protocol != TCP && protocol != UDP) {
		return_msg(EAR_ERROR, "Protocol does not exist");
	}
	// Cleaning and settings
	sockets_clean(socket);
	socket->port     = port;
	socket->protocol = protocol;
	// If is not NULL, then copy in the small buffer in socket_t.
	if (host != NULL) {
		socket->host = strcpy(socket->host_dst, host);
	}
	return EAR_SUCCESS;
}

state_t sockets_clean(socket_t *socket)
{
	memset((void *) socket, 0, sizeof(socket_t));
	socket->fd = -1;
	return EAR_SUCCESS;
}

state_t sockets_dispose(socket_t *socket)
{
	if (socket->fd_closable) {
		if (socket->fd >= 0) {
			close(socket->fd);
		}
	}
	if (socket->info != NULL) {
		freeaddrinfo(socket->info);
		socket->info = NULL;
	}
	return sockets_clean(socket);
}

/*
 *
 * Common client/server
 *
 */

state_t sockets_socket(socket_t *sock)
{
	struct addrinfo hints;
	char string_port[16];
	int ret;

	// Format
	sprintf(string_port, "%u", sock->port);
	memset(&hints, 0, sizeof(hints));
	// Initialize addrinfo basic data.
	hints.ai_socktype = sock->protocol; // TCP stream sockets
	hints.ai_family = AF_UNSPEC;		// Don't care IPv4 or IPv6
	// If NULL, fill my IP for me.
	if (sock->host == NULL) {
		hints.ai_flags = AI_PASSIVE;
	}
	// Fills the struct. If different than 0, it returns error.
	if ((ret = getaddrinfo(sock->host, string_port, &hints, &sock->info)) != 0) {
		return_msg(EAR_ERROR, (char *) gai_strerror(ret));
	}
	// With address struct complete, we can initialize the socket and get the fd.
	sock->fd = socket(sock->info->ai_family, sock->info->ai_socktype, sock->info->ai_protocol);
	if (sock->fd < 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	debug("granted file descriptor for socket %s:%d (fd %d)",
		sock->host, sock->port, sock->fd);
	// It can be closed now.
	sock->fd_closable = 1;
	return EAR_SUCCESS;
}

/*
 *
 * Server side
 *
 */

state_t sockets_bind(socket_t *socket, time_t timeout)
{
	timeout = time(NULL) + timeout;
	int ret;

	do {
		// Assign to the socket the address and port. Wait until successfully
		// binds. It is when timeout (in seconds) has passed.
		ret = bind(socket->fd, socket->info->ai_addr, socket->info->ai_addrlen);
	} while ((ret < 0) && (time(NULL) < timeout) && !sleep(5));

	if (ret < 0) {
		return_msg(EAR_ERROR, (char *) strerror(errno));
	}
	debug("binded socket %s:%d (fd %d)",
		socket->host, socket->port, socket->fd);
	return EAR_SUCCESS;
}

state_t sockets_accept(int fd_incoming, int *fd_client, struct sockaddr_storage *addr_client)
{
	struct sockaddr_storage aux_addr;
	socklen_t size;

	size = sizeof (struct sockaddr_storage);
	// If addr_client is NULL, we fill a stack auxiliar address.
	if (addr_client != NULL) {
		*fd_client = accept(fd_incoming, (struct sockaddr *) addr_client, &size);
	} else {
		*fd_client = accept(fd_incoming, (struct sockaddr *) &aux_addr, &size);
	}
	// fd_client can't be NULL.
	if (*fd_client == -1) {
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return_msg(EAR_NOT_READY, strerror(errno));
		} else {
			return_msg(EAR_ERROR, strerror(errno));
		}
	}
	#if SHOW_DEBUGS
	char address[512];
	sockets_get_hostname_fd(*fd_client, address, 512);
	debug("listening descriptor %d accepted socket from address %s (fd %d)",
		fd_incoming, address, *fd_client);
	#endif

	return EAR_SUCCESS;
}

state_t sockets_listen(socket_t *socket)
{
	if (socket->protocol == UDP) {
		return EAR_SUCCESS;
	}
	// Listening the port.
	if (listen(socket->fd, BACKLOG) < 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	debug("listening socket %s:%d returned (fd %d)",
		socket->host, socket->port, socket->fd);
	return EAR_SUCCESS;
}

/*
 *
 * Client side
 *
 */

state_t sockets_connect(socket_t *socket)
{
	// Assign to the socket the address and port.
	if (connect(socket->fd, socket->info->ai_addr, socket->info->ai_addrlen) < 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	debug("granted connection for socket %s:%d returned (fd %d)",
		socket->host, socket->port, socket->fd);
	return EAR_SUCCESS;
}

/*
 *
 * Closing/disconnect
 *
 */

state_t sockets_close(socket_t *socket)
{
	state_t s = EAR_SUCCESS;
	if (socket->fd_closable) {
		s = sockets_close_fd(socket->fd);
	}
	socket->fd_closable = 0;
	socket->fd = -1;
	return s;
}

state_t sockets_close_fd(int fd)
{
	if (fd > 0) {
		close(fd);
	}
	return EAR_SUCCESS;
}

/*
 *
 * Read & write
 *
 */

static state_t static_send(int fd, char *data, size_t size)
{
	ssize_t bytes_left = (ssize_t) size;
	ssize_t bytes_sent = 0;
	int flags;
	// Better return an error than receive a signal.
	flags = MSG_NOSIGNAL;
	// In the near future, it will be better to include a non-blocking
	// implementation with a controlled retry system.
	while(bytes_left > 0) {
		// TCP protocol
		if ((bytes_sent = send(fd, data + bytes_sent, bytes_left, flags)) < 0) {
			debug("fd '%d', %ld to add to %ld/%ld (errno: %d, strerrno: %s)",
				  fd, bytes_sent, size - bytes_left, size, errno, strerror(errno));
			switch (bytes_sent) {
				case ENETUNREACH:
				case ENETDOWN:
				case ECONNRESET:
				case ENOTCONN:
				case EPIPE:
				case EDESTADDRREQ:
					return_msg(EAR_ERROR, strerror(errno));
				case EAGAIN:
					return_msg(EAR_NOT_READY, strerror(errno));
				default:
					return_msg(EAR_ERROR, strerror(errno));
			}
		}
		bytes_left -= bytes_sent;
	}
	return EAR_SUCCESS;
}

static state_t static_unlock(state_t s)
{
	pthread_mutex_unlock(&lock_send);
	return s;
}

state_t sockets_send(int fd, uint type, char *data, size_t size, ullong extra)
{
	state_t s;
	struct header_s {
		size_t size;
		ullong extra;
		uint type;
	} header = { .size = size, .extra = extra, .type = type };
	// Locking (I don't know why).
	pthread_mutex_lock(&lock_send);
	// Sending header
	if(state_fail(s = static_send(fd, (char *) &header, sizeof(header)))) {
		return static_unlock(s);
	}
	// Sending content
	if (data != NULL && size > 0) {
		if (state_fail(s = static_send(fd, data, size))) {
			return static_unlock(s);
		}
	}
	// Unlocking
	return static_unlock(EAR_SUCCESS);
}

static state_t static_recv(int fd, char *buffer, ssize_t size, int block)
{
	ssize_t bytes_left = size;
	ssize_t bytes_recv = 0;
	ssize_t bytes_acum = 0;
	int intents = 0;
	int flags = 0;

	if (!block) {
		flags = MSG_DONTWAIT;
	}
	while (bytes_left > 0) {
		//
		if ((bytes_recv = recv(fd, (void *) &buffer[bytes_acum], bytes_left, flags)) == 0) {
			return_msg(EAR_ERROR, "Disconnected from socket");
		}
		else if (bytes_recv < 0) {
			debug("fd '%d', %ld to add to %ld/%ld (intent: %d, errno: %d, strerrno: %s)",
				  fd, bytes_recv, size - bytes_left, size, intents, errno, strerror(errno));
			switch (errno) {
				case ENOTCONN:
				case ETIMEDOUT:
				case ECONNRESET:
					return_msg(EAR_ERROR, strerror(errno));
					break;
				case EINVAL: // Not sure abou this
				case EAGAIN:
					if (!block && intents < NON_BLOCK_TRYS) {
						intents += 1;
					} else {
						return_msg(EAR_TIMEOUT, strerror(errno));
					}
					break;
				default:
					return_msg(EAR_ERROR, strerror(errno));
			}
		} else {
			bytes_acum += bytes_recv;
			bytes_left -= bytes_recv;
		}
	}
	return EAR_SUCCESS;
}

state_t sockets_recv_header(int fd, uint *type, size_t *size, ullong *extra, uint block)
{
	state_t s = EAR_SUCCESS;
	struct header_s {
		size_t size;
		ullong extra;
		uint type;
	} header;

	// Initializing data
	header.type  = 0;
	header.size  = 0;
	header.extra = 0;
	// Receiving header
	s = static_recv(fd, (char *) &header, sizeof(header), block);
	*type = header.type;
	*size = header.size;
	if (extra != NULL) {
		*extra = header.extra;
	}

	return s;
}

state_t sockets_recv(int fd, char *data, size_t size, uint block)
{
	return static_recv(fd, data, size, block);
}

/*
 *
 * File descriptor options
 *
 */

state_t sockets_timeout_get(int fd, time_t *timeout)
{
	struct timeval tv;
	socklen_t length;

	*timeout = 0;
	//
	if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &length) != 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	*timeout = (time_t) tv.tv_sec;
	return EAR_SUCCESS;
}

state_t sockets_timeout_set(int fd, time_t timeout)
{
	struct timeval tv;
	tv.tv_sec  = timeout;
	tv.tv_usec = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(struct timeval)) != 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t sockets_nonblock_set(int fd)
{
	fcntl(fd, F_SETFL, O_NONBLOCK);
	return EAR_SUCCESS;
}

state_t sockets_nonblock_unset(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
	return EAR_SUCCESS;
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
	struct sockaddr_storage adress;
	socklen_t size;

	size = sizeof(struct sockaddr_storage);
	memset(&adress, 0, size);

	getpeername(fd, (struct sockaddr *) &adress, &size);
	sockets_get_hostname(&adress, buffer, length);
}

void sockets_get_ip(struct sockaddr_storage *host_addr, long *ip)
{
	//socklen_t addrlen;
	//void *address;
	//int port;

	// IPv4
	if (host_addr->ss_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *) host_addr;
		//port = (int) ntohs(ipv4->sin_port);
		//addrlen = INET_ADDRSTRLEN;
		//address = &(ipv4->sin_addr);
		*ip = ipv4->sin_addr.s_addr;
	}
	// IPv6
	else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) host_addr;
		//port = (int) ntohs(ipv6->sin6_port);
		//addrlen = INET6_ADDRSTRLEN;
		//address = &(ipv6->sin6_addr);
		*ip = (long) ipv6->sin6_addr.s6_addr[8];
	}
	// convert the IP to a string and print it
	//inet_ntop(host_addr->sa_family, address, buffer, INET6_ADDRSTRLEN);
}

/*
 *
 * Obsolete
 *
 */

state_t sockets_header_clean(socket_header_t *header)
{
	memset((void *) header, 0, sizeof(socket_header_t));
	return EAR_SUCCESS;
}

state_t sockets_header_update(socket_header_t *header)
{
	#if SOCKETS_DEBUG
	if (strlen(header->host_src) == 0) {
		gethostname(header->host_src, sizeof(header->host_src));
	}
	header->timestamp = time(NULL);
	#endif
	return EAR_SUCCESS;
}

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

static state_t __static_send(socket_t *socket, ssize_t bytes_expc, char *buffer)
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

state_t __sockets_send(socket_t *socket, socket_header_t *header, char *content)
{
	char output_buffer[SZ_BUFFER];
	socket_header_t *output_header;
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
	output_header  = PACKET_HEADER(output_buffer);
	output_content = PACKET_CONTENT(output_buffer);
	// Copy process
	memcpy(output_header, header, sizeof(socket_header_t));
	memcpy(output_content, content, header->content_size);
	// Locking and sending
	pthread_mutex_lock(&lock_send);
	{
		// Sending
		state = __static_send(socket, sizeof(socket_header_t) + header->content_size, output_buffer);
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

static state_t __static_recv(int fd, ssize_t bytes_expc, char *buffer, int block)
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
		debug("fd '%d' received %ld of %ld/%ld (intent: %d)",
			  fd, bytes_recv, bytes_expc - bytes_left, bytes_expc, intents);

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

state_t __sockets_recv(int fd, socket_header_t *header, char *buffer, ssize_t size_buffer, int block)
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
	state = __static_recv(fd, sizeof(socket_header_t), (char *) header, block);
	if (state_fail(state)) {
		state_return(state);
	}
	if (header->content_size > size_buffer) {
		state_return(EAR_NO_RESOURCES);
	}
	// Receiving the content
	state = __static_recv(fd, header->content_size, buffer, block);
	if (state_fail(state)) {
		state_return(state);
	}

	state_return(EAR_SUCCESS);
}
