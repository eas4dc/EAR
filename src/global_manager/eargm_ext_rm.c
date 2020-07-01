/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*   
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*   
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING  
*/

#define _XOPEN_SOURCE 700 //to get rid of the warning

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <common/states.h>
#include <common/config.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <global_manager/eargm_server_api.h>
#include <daemon/eard_rapi.h>


extern uint total_nodes;
extern uint my_port;



/*
*
*	THREAD attending external commands. 
*
*/

void process_remote_requests(int clientfd)
{
    eargm_request_t command;
    uint req;
    ulong ack=EAR_SUCCESS;
    debug("connection received");
    req=read_command(clientfd,&command);
    switch (req){
        case EARGM_NEW_JOB:
			// Computes the total number of nodes in use
            debug("new_job command received %d num_nodes %u",command.req,command.num_nodes);
			total_nodes+=command.num_nodes;
            break;
        case EARGM_END_JOB:
			// Computes the total number of nodes in use
            debug("end_job command received %d num_nodes %u",command.req,command.num_nodes);
			total_nodes-=command.num_nodes;
            break;
        default:
            error("Invalid remote command");
    }  
    send_answer(clientfd,&ack);
}


void *eargm_server_api(void *p)
{
	int eargm_fd,eargm_client;
	struct sockaddr_in eargm_con_client;

    debug("Creating scoket for remote commands,using port %u",my_port);
    eargm_fd=create_server_socket(my_port);
    if (eargm_fd<0){
        error("Error creating socket");
        pthread_exit(0);
    }
    do{
        debug("waiting for remote commands port=%u",my_port);
        eargm_client=wait_for_client(eargm_fd,&eargm_con_client);
        if (eargm_client<0){
            error(" wait_for_client returns error");
        }else{
            process_remote_requests(eargm_client);
            close(eargm_client);
        }
    }while(1);
    verbose(VGM,"exiting");
    close_server_socket(eargm_fd);
	return NULL;

}




