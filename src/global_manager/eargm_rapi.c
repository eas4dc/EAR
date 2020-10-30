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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <global_manager/eargm_rapi.h>
#include <global_manager/eargm_conf_api.h>

// Verbosity
static int eargm_sfd;
static uint eargm_remote_connected=0;

static int send_command(eargm_request_t *command)
{
	ulong ack;
	int ret;
	verbose(2,"Sending command %u\n",command->req);
	if (!eargm_remote_connected) return 0;
	if ((ret=write(eargm_sfd,command,sizeof(eargm_request_t)))!=sizeof(eargm_request_t)){
		if (ret<0){ 
			verbose(0,"Error sending command %s\n",strerror(errno));
		}else{ 
			verbose(0,"Error sending command \n");
		}
	}
	ret=read(eargm_sfd,&ack,sizeof(ulong));
	if (ret<0){
		verbose(0,"Error receiving ack %d\n",ret);
	}
	return (ret==sizeof(ulong)); // Should we return ack ?
}

// based on getaddrinfo  man page
int eargm_connect(char *nodename,uint use_port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
		char port_number[50]; 	// that size needs to be validated
    int sfd, s;

		if (eargm_remote_connected) return eargm_sfd;
   	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

		sprintf(port_number,"%u",use_port);
   	s = getaddrinfo(nodename, port_number, &hints, &result);
    if (s != 0) {
		#if DEBUG
		verbose(0, "getaddrinfo failt for %s and %s", nodename, port_number);
		#endif
		return EAR_ERROR;
    }

   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1) // if we can not create the socket we continue
            continue;

       if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

       close(sfd);
    }

   	if (rp == NULL) {               /* No address succeeded */
		#if DEBUG
		verbose(0, "Failing in connecting to remote erds");
		#endif
		return EAR_ERROR;
    }

   	freeaddrinfo(result);           /* No longer needed */
	eargm_remote_connected=1;
	eargm_sfd=sfd;
	return sfd;

}

int eargm_new_job(uint num_nodes)
{
	eargm_request_t command;
	command.req=EARGM_NEW_JOB;
	command.num_nodes=num_nodes;
	verbose(2,"command %u num_nodes %u\n",command.req,command.num_nodes);
	return send_command(&command);
}

int eargm_end_job(uint num_nodes)
{
    eargm_request_t command;
    command.req=EARGM_END_JOB;
	command.num_nodes=num_nodes;
	verbose(2,"command %u num_nodes %u\n",command.req,command.num_nodes);
	return send_command(&command);
}

int eargm_disconnect()
{
	eargm_remote_connected=0;
	close(eargm_sfd);
	return EAR_SUCCESS;
}
