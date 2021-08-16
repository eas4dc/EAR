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
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <linux/limits.h>


#include <common/config.h>

//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>

static int max_fd=-1;
fd_set rfds,rfds_sys;
int pipe_for_new_conn[2];

extern int eard_must_exit;


state_t init_active_connections_list()
{
	FD_ZERO(&rfds);
	/* Nex sockets will be written in the pipe and the thread will wake up */
	if (pipe(pipe_for_new_conn)<0){
		error("Creating pipe for remote connections management");
		return EAR_ERROR;
	}
	debug("Channel for remote connection notification included [%d,%d]",pipe_for_new_conn[0],pipe_for_new_conn[1]);
	FD_SET(pipe_for_new_conn[0],&rfds);
	max_fd = pipe_for_new_conn[0] +1;
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
	if (new_fd == FD_SETSIZE){
		error("Maximum number of active remote connections reached, rejecting the connection");
		close(new_fd);
		return EAR_ERROR;
	}
	/* we have a new valid fd */
	debug("New remote connection %d",new_fd);
	FD_SET(new_fd, &rfds);
	if (new_fd >= max_fd) max_fd = new_fd + 1;
	return EAR_SUCCESS;
}

state_t remove_remote_connection(int fd)
{
    int i=0;
	debug("Closing remote connection %d",fd);
	FD_CLR(fd, &rfds);
	if (fd == (max_fd-1)){
		/* We must update the maximum */	
		max_fd = -1;
		for (i=0;i< FD_SETSIZE; i++){
			if (FD_ISSET(i,&rfds) && (i >= max_fd)) max_fd = i+1;
		}
	}
	close(fd);
  return EAR_SUCCESS;
}

void check_sockets(fd_set *fdlist)
{
	int i;
	struct stat fdstat;
	for (i=0;i<max_fd;i++){
		if (FD_ISSET(i,fdlist)){
			debug("Validating socket %d",i);
			if (fstat(i,&fdstat)<0){
				remove_remote_connection(i);
			}
		}
	}
}

/************* This thread will process the remote requests */
extern state_t process_remote_requests(int clientfd);
void * process_remote_req_th(void * arg)
{
	int i;
	int numfds_ready;
	state_t ret;
	debug("Thread to process remote requests created ");
	rfds_sys=rfds;
  while ((numfds_ready = select(max_fd, &rfds_sys, NULL, NULL, NULL)) && (eard_must_exit == 0))
  {
		verbose(VRAPI,"RemoteAPI thread new info received (new_conn/new_data)");
		if (numfds_ready > 0) {
			/* This is the normal use case */
      for (i = 0; i < max_fd; i++) {
          if (FD_ISSET(i, &rfds_sys)){
						debug("Channel %d ready for reading",i);
						if (i == pipe_for_new_conn[0]){ 
							verbose(VRAPI,"New connection received in RemoteAPI thread");
							add_new_connection();
						}else{ 
							verbose(VRAPI,"New request received in RemoteAPI thread");
							ret=process_remote_requests(i);
							if (ret != EAR_SUCCESS) remove_remote_connection(i);
						}
          }   
      } 
     } else if (numfds_ready == 0) { 
				/* This shouldn't happen since we are not using timeouts*/
				debug("numfds_ready is zero in remote connections activity"); 
		}else{
				/* This should be an error */
				if (errno == EINTR){
					debug("Signal received in remote commads");
				}else if (errno == EBADF){
					debug("EBADF error detected, validating fds");
					check_sockets(&rfds);
				}else{
					error("Unexpected error in processing remote connections %s ",strerror(errno));
				}
		}
		rfds_sys=rfds;
	}
	pthread_exit(0);
}
