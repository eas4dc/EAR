/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/limits.h>
#include <common/config.h>
#include <common/states.h>
#include <common/system/poll.h>
#include <common/system/file.h>
#include <common/output/verbose.h>

extern int eard_must_exit;

static afd_set_t rfds_basic;
static int pipe_for_new_conn[2];

state_t init_active_connections_list()
{
	AFD_ZERO(&rfds_basic);
	/* Nex sockets will be written in the pipe and the thread will wake up */
	if (pipe(pipe_for_new_conn)<0){
		error("Creating pipe for remote connections management");
		return EAR_ERROR;
	}
	debug("Channel for remote connection notification included [%d,%d]",pipe_for_new_conn[0],pipe_for_new_conn[1]);
	AFD_SET(pipe_for_new_conn[0], &rfds_basic);
	return EAR_SUCCESS;
}

/*********** This function notify a new remote connetion ***************/
state_t notify_new_connection(int fd)
{
	debug("Accept new connection %d", fd);
	if (ear_fd_write(pipe_for_new_conn[1],(char *)&fd,sizeof(int)) != EAR_SUCCESS){
		error("Error sending new fd for remote command %s",strerror(errno));
		return EAR_ERROR;
	}
	return EAR_SUCCESS;
}

state_t add_new_connection()
{
	int new_fd;
	/* New fd will be read from the pipe */
	if (ear_fd_read(pipe_for_new_conn[0],(char *)&new_fd,sizeof(int)) != EAR_SUCCESS){
		error("Error sending new fd for remote command %s",strerror(errno));
		return EAR_ERROR;
	}
	if (new_fd < 0){
		error("New fd is less than 0 ");
		return EAR_ERROR;
	}
	/* we have a new valid fd */
	debug("New remote connection %d",new_fd);
	if (AFD_SET(new_fd, &rfds_basic) == 0) {
        error("new remote connection fd is too large (%d, max is %d). Closing connection", new_fd, AFD_MAX);
        close(new_fd);
    }
	return EAR_SUCCESS;
}

state_t remove_remote_connection(int fd)
{
	debug("Closing remote connection %d",fd);
	AFD_CLR(fd, &rfds_basic);
	close(fd);
    return EAR_SUCCESS;
}

bool is_socket_alive(int32_t socketfd)
{
	debug("Entering is_socket_alive with fd %d\n", socketfd);
	struct tcp_info info = { 0 };
    socklen_t  optlen = sizeof(info);

	int32_t ret = getsockopt(socketfd, IPPROTO_TCP, TCP_INFO, &info, &optlen);
	if (ret < 0) {
		debug("Error getting sockopt FD %d errnum %d (%s)\n", socketfd, errno, strerror(errno));
		return true;
	} else if (info.tcpi_state == TCP_CLOSE_WAIT) {
		debug("Socket opt with TCP_CLOSE_WAIT!\n");
		return false;
	}
	debug("Socket %d opt got with state %d!\n",socketfd, info.tcpi_state);
	return true;
}

static void check_all_fds(afd_set_t *fdset)
{
	debug("Testing sockets min %d max %d", fdset->fd_min+1, fdset->fd_max);
	for (int32_t i = fdset->fd_min+1; i <= fdset->fd_max; i++) {
		if (fdset->fds[i].fd != -1) {
			if (i == pipe_for_new_conn[0]) continue;
			if (!is_socket_alive(i)) {
				debug("Remote api(check_all_fds) closing connection %d", i);
				remove_remote_connection(i);
			}
		}
	}
}

/************* This thread will process the remote requests */
extern state_t process_remote_requests(int clientfd);

void *process_remote_req_th(void * arg)
{
	int numfds_ready;
	state_t ret;
	int i;

	debug("Thread to process remote requests created ");
	while ((numfds_ready = aselectv(&rfds_basic, NULL)) && (eard_must_exit == 0))
	{
		verbose(VRAPI, "RemoteAPI thread new info received (new_conn/new_data)");
		if (numfds_ready > 0) {
			/* This is the normal use case */
			for (i = rfds_basic.fd_min; i <= rfds_basic.fd_max; i++) {
				#if 0
				if ((i != pipe_for_new_conn[0]) && AFD_ISHUP(i, &rfds_basic)) {
					verbose(VRAPI, "ERROR: socket remote client has disconnected, closing the socket");
					remove_remote_connection(i);
				}
				else 
				#endif
				if (AFD_ISSET(i, &rfds_basic)) {
					debug("Channel %d ready for reading", i);
					if (i == pipe_for_new_conn[0]) {
						verbose(VRAPI, "New connection received in RemoteAPI thread");
						add_new_connection();
					} else {
						verbose(VRAPI, "New request received in RemoteAPI thread %d", i);
						ret = process_remote_requests(i);
						if (ret != EAR_SUCCESS){
							verbose(VRAPI, "process_remote_requests returns error, closing %d ret %d", i, ret);
							remove_remote_connection(i);
						}
					}
				}
			}
		} else if (numfds_ready == 0) {
			/* This shouldn't happen since we are not using timeouts*/
			debug("numfds_ready is zero in remote connections activity");
		} else {
			/* This should be an error */
			if (errno == EINTR) {
				debug("Signal received in remote commads");
			} else if (errno == EBADF) {
				debug("EBADF error detected, validating fds");
				afd_check_sockets(&rfds_basic);
			} else {
				error("Unexpected error in processing remote connections %s ", strerror(errno));
			}
		}
		check_all_fds(&rfds_basic);
	}
	pthread_exit(0);
}
