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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>
#include <common/output/verbose.h>
#include <daemon/eard_rapi.h>
#include <daemon/eard_conf_rapi.h>

// 2000 and 65535
#define DAEMON_EXTERNAL_CONNEXIONS 1
#define NEW_STATUS 1

int *ips = NULL;
int total_ips = -1;
int self_id = -1;
int init_ips_ready=0;

// based on getaddrinfo man pages
int create_server_socket(uint port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
		char buff[50]; // This must be checked

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

		sprintf(buff,"%d",port);

   	s = getaddrinfo(NULL, buff, &hints, &result);
    if (s != 0) {
		error("getaddrinfo fails for port %s (%s)",buff,strerror(errno));
		return EAR_ERROR;
    }


   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
            continue;



        while (bind(sfd, rp->ai_addr, rp->ai_addrlen) != 0){ 
			verbose(VAPI,"Waiting for connection");
			sleep(10);
	    }
            break;                  /* Success */

    }

   	if (rp == NULL) {               /* No address succeeded */
		error("bind fails for eards server (%s) ",strerror(errno));
		return EAR_ERROR;
    }else{
		verbose(VAPI+1,"socket and bind for erads socket success");
	}

   	freeaddrinfo(result);           /* No longer needed */

   	if (listen(sfd,DAEMON_EXTERNAL_CONNEXIONS)< 0){
		error("listen eards socket fails (%s)",strerror(errno));
		close(sfd);
 		return EAR_ERROR;
	}
	verbose(VAPI,"socket listen ready!");
 	return sfd;
}

int wait_for_client(int s,struct sockaddr_in *client)
{
	int new_sock;
	socklen_t client_addr_size;


    client_addr_size = sizeof(struct sockaddr_in);
    new_sock = accept(s, (struct sockaddr *) client, &client_addr_size);
    if (new_sock < 0){ 
		error("accept for eards socket fails %s\n",strerror(errno));
		return EAR_ERROR;
	}
    char conection_ok = 1;
    write(new_sock, &conection_ok, sizeof(char));
    verbose(VCONNECT, "Sending handshake byte to client.");
	verbose(VCONNECT, "new connection ");
	return new_sock;
}

void close_server_socket(int sock)
{
	close(sock);
}

int read_command(int s, request_t *command)
{
    request_header_t head;

    request_t *tmp_command;
    head = recieve_data(s, (void **)&tmp_command);
    if (head.type != EAR_TYPE_COMMAND || head.size < sizeof(request_t))
    {
        command->req = NO_COMMAND;
        if (head.size > 0) free(tmp_command);
#if DYN_PAR
        return EAR_SOCK_DISCONNECTED;
#endif
        return command->req;
    }
    memcpy(command, tmp_command, sizeof(request_t));
    free(tmp_command);
    return command->req;
}

void send_answer(int s,long *ack)
{
	int ret;
	if ((ret=write(s,ack,sizeof(ulong)))!=sizeof(ulong)) error("Error sending the answer");
	if (ret<0) error("(%s)",strerror(errno));
}

int init_ips(cluster_conf_t *my_conf)
{
    int ret, i, temp_ip;
    char buff[64];
    gethostname(buff, 64);
    strtok(buff,".");
    ret = get_range_ips(my_conf, buff, &ips);
    if (ret < 1) {
        ips = NULL;
        self_id = -1;
				init_ips_ready=-1;
        return EAR_ERROR;
    }
    if (strlen(my_conf->net_ext) > 0)
    strcat(buff, my_conf->net_ext);
    temp_ip = get_ip(buff, my_conf);
    for (i = 0; i < ret; i++)
    {
        if (ips[i] == temp_ip)
        {
            self_id = i;
            break;
        }
    }
    if (self_id < 0)
    {
        free(ips);
        ips = NULL;
        error("Couldn't find node in IP list.");
				init_ips_ready=-1;
        return EAR_ERROR;
    }
    total_ips = ret;
		verbose(0,"Init_ips_ready=1");	
		init_ips_ready=1;	
    debug("Total ips %d",total_ips);
    return EAR_SUCCESS;
  
}

void close_ips()
{
    if (total_ips > 0)
    {
        free(ips);
        total_ips = -1;
        self_id = -1;
    }
    else
    {
        warning("IPS were not initialised.\n");
    }
}

void propagate_req(request_t *command, uint port)
{
     
    if (command->node_dist > total_ips) return;
    if (self_id == -1 || ips == NULL || total_ips < 1) return;

    struct sockaddr_in temp;

    int i, rc, off_ip;
    unsigned int  current_dist;
    char next_ip[50]; 

    current_dist = self_id - (self_id%NUM_PROPS);
    off_ip = self_id%NUM_PROPS;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((i*NUM_PROPS + current_dist*NUM_PROPS + off_ip) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[current_dist*NUM_PROPS + i*NUM_PROPS + off_ip];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_dist*NUM_PROPS + i*NUM_PROPS;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("propagate_req:Error connecting to node: %s\n", next_ip);
            correct_error(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port);
        }
        else
        {
            if (!send_command(command)) 
            {
                error("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                correct_error(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port);
            }
            else eards_remote_disconnect();
        }
    }
}


int get_self_ip()
{
    int s;
    char buff[512];
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    gethostname(buff, 50);

    strtok(buff,".");
    #if USE_EXT
    strcat(buff, NW_EXT);
    #endif


   	s = getaddrinfo(buff, NULL, &hints, &result);
    if (s != 0) {
		error("propagate_status:getaddrinfo fails for port %s (%s)",buff,strerror(errno));
		return EAR_ERROR;
    }

   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in*) (rp->ai_addr);
            struct sockaddr_in temp;

            return saddr->sin_addr.s_addr;
        }
    }
	return EAR_ERROR;
}


request_header_t propagate_data(request_t *command, uint port, void **data)
{
    char *temp_data, *final_data = NULL;
    int rc, i, off_ip, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    unsigned int current_dist;
    request_header_t head;
    char next_ip[64];

    current_dist = self_id - (self_id%NUM_PROPS);
    off_ip = self_id%NUM_PROPS;
    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((i*NUM_PROPS + current_dist*NUM_PROPS + off_ip) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[(i*NUM_PROPS + current_dist*NUM_PROPS + off_ip) ];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_dist*NUM_PROPS + i*NUM_PROPS;
        debug("Propagating to %s with distance %d\n", next_ip, command->node_dist);

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("propagate_req: Error connecting to node: %s\n", next_ip);
            head = correct_data_prop(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port, (void **)&temp_data);
        }
        else
        {
            send_command(command);
            head = recieve_data(rc, (void **)&temp_data);
            if (head.size < 1 || head.type == EAR_ERROR) 
            {
                error("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                head = correct_data_prop(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port, (void **)&temp_data);
            }
            else eards_remote_disconnect();
        }
        //TODO: Process data (ex. when it's a powercap_status the memory won't be an exact map of power_cap_statuses
        //temporary workaround for status_t:
        if (head.size > 0 && head.type != EAR_ERROR)
        {
            head = process_data(head, &temp_data, &final_data, final_size);
            free(temp_data);
            default_type = head.type;
            final_size = head.size;
        }
    }

    head.size = final_size;
    head.type = default_type;
    *data = final_data;

    if (default_type == EAR_ERROR && final_size > 0)
    {
        free(final_data);
        head.size = 0;
    }
    else if (final_size < 1 && default_type != EAR_ERROR) head.type = EAR_ERROR;

    return head;
	
}
#if POWERCAP
int propagate_powercap_status(request_t *command, uint port, powercap_status_t **status)
{
    powercap_status_t *temp_status, *final_status;

    // if the current node is a leaf node (either last node or ip_init had failed)
    // we allocate the power_status in this node and return
    if (command->node_dist > total_ips || self_id < 0 || ips == NULL || total_ips < 1)
    {
        final_status = calloc(1, sizeof(powercap_status_t));
        *status = final_status;
        return 1;
    }

    request_header_t head = propagate_data(command, port, (void **)&temp_status); //we don't check for number of powercap_status since we will have 1 at most
    int num_status = head.size / sizeof(powercap_status_t); //we still need a return value
    if (head.type != EAR_TYPE_POWER_STATUS || head.size < sizeof(powercap_status_t))
    {
        final_status = calloc(1, sizeof(powercap_status_t));
        if (head.size > 0) free(temp_status);
        *status = final_status;
        return 1; //maybe return EAR_ERROR?? -> for now we return 1 as we get a correct power_status (for this node)
    }

    *status = temp_status;
    num_status = 1;

    return num_status;
}

int propagate_release_idle(request_t *command, uint port, pc_release_data_t *release)
{
    pc_release_data_t *new_released;
    
    // if the current node is a leaf node (either last node or ip_init had failed)
    // we allocate the power_status in this node and return
    if (command->node_dist > total_ips || self_id < 0 || ips == NULL || total_ips < 1)
    {
        memset(release, 0, sizeof(pc_release_data_t));
        return 1;
    }
    request_header_t head = propagate_data(command, port, (void **)&new_released);

    if (head.type != EAR_TYPE_RELEASED || head.size < sizeof(pc_release_data_t))
    {
        memset(release, 0, sizeof(pc_release_data_t));
        if (head.size > 0) free(new_released);
        return 1;
    }

    *release = *new_released;

    return 1;

}

#endif

int propagate_status(request_t *command, uint port, status_t **status)
{
    status_t *temp_status, *final_status;
    int num_status = 0;

    // if the current node is a leaf node (either last node or ip_init had failed)
    // we set the status for this node and return
    if (command->node_dist > total_ips || self_id < 0 || ips == NULL || total_ips < 1)
    {
        final_status = calloc(1, sizeof(status_t));
        if (self_id < 0 || ips == NULL)
            final_status[0].ip = get_self_ip();
        else
            final_status[0].ip = ips[self_id];
        final_status[0].ok = STATUS_OK;
	    debug("status has 1 status\n");
        *status = final_status;
        return 1;
    }

    // propagate request and recieve the data
    request_header_t head = propagate_data(command, port, (void **)&temp_status);
    num_status = head.size / sizeof(status_t);
    
    if (head.type != EAR_TYPE_STATUS || head.size < sizeof(status_t))
    {
        final_status = calloc(1, sizeof(status_t));
        final_status[0].ip = ips[self_id];
        final_status[0].ok = STATUS_OK;
        if (head.size > 0) free(temp_status);
        *status = final_status;
        return 1;
    }

    //memory allocation with the current node
    final_status = calloc(num_status + 1, sizeof(status_t));
    memcpy(final_status, temp_status, head.size);

    //current node info
    final_status[num_status].ip = ips[self_id];
    final_status[num_status].ok = STATUS_OK;
    *status = final_status;
    num_status++;   //we add the original status

    free(temp_status);

    return num_status;

}

