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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/limits.h>
#include <common/config.h>
#include <common/states.h>
#include <common/system/poll.h>
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
	if (write(pipe_for_new_conn[1],&fd,sizeof(int)) < 0){
		error("Error sending new fd for remote command %s",strerror(errno));
		return EAR_ERROR;
	}
	return EAR_SUCCESS;
}

state_t add_new_connection()
{
	int new_fd;
	/* New fd will be read from the pipe */
	if (read(pipe_for_new_conn[0],&new_fd,sizeof(int)) < 0){
		error("Error sending new fd for remote command %s",strerror(errno));
		return EAR_ERROR;
	}
	if (new_fd < 0){
		error("New fd is less than 0 ");
		return EAR_ERROR;
	}
	/* we have a new valid fd */
	debug("New remote connection %d",new_fd);
	AFD_SET(new_fd, &rfds_basic);
	return EAR_SUCCESS;
}

state_t remove_remote_connection(int fd)
{
	debug("Closing remote connection %d",fd);
	AFD_CLR(fd, &rfds_basic);
	close(fd);
    return EAR_SUCCESS;
}

void check_sockets(afd_set_t *fdlist)
{
	int i;
    for (i = rfds_basic.fd_min; i <= rfds_basic.fd_max; i++) {
		if (AFD_ISSET(i,fdlist)){
			debug("Validating socket %d",i);
			if (!AFD_STAT(i, NULL)){
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
                if (AFD_ISSET(i, &rfds_basic)) {
                    debug("Channel %d ready for reading", i);
                    if (i == pipe_for_new_conn[0]) {
                        verbose(VRAPI, "New connection received in RemoteAPI thread");
                        add_new_connection();
                    } else {
                        verbose(VRAPI, "New request received in RemoteAPI thread");
                        ret = process_remote_requests(i);
                        if (ret != EAR_SUCCESS) remove_remote_connection(i);
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
                check_sockets(&rfds_basic);
            } else {
                error("Unexpected error in processing remote connections %s ", strerror(errno));
            }
        }
    }
    pthread_exit(0);
}
