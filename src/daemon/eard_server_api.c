/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

//#define SHOW_DEBUGS 0
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


int *ips = NULL;
int total_ips = -1;
int self_id = -1;

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

int read_command(int s,request_t *command)
{
	int ret,pending,done;
	pending=sizeof(request_t);
	done=0;

	ret=read(s,command,sizeof(request_t));
	//ret=recv(s,command,sizeof(request_t), MSG_DONTWAIT);
	if (ret<0){
		error("read_command error errno %s",strerror(errno));
		command->req=NO_COMMAND;
		return command->req;
	}
	pending-=ret;
	done=ret;
	while((ret>0) && (pending>0)){
		verbose(VCONNECT,"Read command continue , pending %d",pending);
		ret=read(s,(char*)command+done,pending);
		//ret=recv(s,(char*)command+done,pending, MSG_DONTWAIT);
		if (ret<0) 
        {
            command->req=NO_COMMAND;
            error("read_command error errno %s",strerror(errno));
        }
		pending-=ret;
		done+=ret;
	}
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
        return EAR_ERROR;
    }
    total_ips = ret;
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

#if USE_NEW_PROP
void propagate_req(request_t *command, uint port)
{
     
    if (command->node_dist > total_ips) return;
    if (self_id == -1 || ips == NULL || total_ips < 1) return;

    struct sockaddr_in temp;

    int i, rc;
    unsigned int  current_dist;
		char next_ip[50]; 

    current_dist = command->node_dist;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((self_id + current_dist + i*NUM_PROPS) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[self_id + current_dist + i*NUM_PROPS];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_dist + i*NUM_PROPS;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("propagate_req:Error connecting to node: %s\n", next_ip);
            correct_error(self_id + current_dist + i*NUM_PROPS, total_ips, ips, command, port);
        }
        else
        {
            if (!send_command(command)) 
            {
                error("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                correct_error(self_id + current_dist + i*NUM_PROPS, total_ips, ips, command, port);
            }
            else eards_remote_disconnect();
        }
    }
}

#else
void propagate_req(request_t *command, uint port)
{
     
    if (command->node_dist < 1) return;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    unsigned int ip1, ip2;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
	char buff[50], nextip1[50], nextip2[50]; 

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
		error("propagate_req:getaddrinfo fails for port %s (%s)",buff,strerror(errno));
		return;
    }


   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in*) (rp->ai_addr);
            struct sockaddr_in temp;

            ip1 = ip2 = htonl(saddr->sin_addr.s_addr);
            ip1 += command->node_dist;
            temp.sin_addr.s_addr = ntohl(ip1);
            strcpy(nextip1, inet_ntoa(temp.sin_addr));

            ip2 -= command->node_dist;
            temp.sin_addr.s_addr = ntohl(ip2);
            strcpy(nextip2, inet_ntoa(temp.sin_addr));
        }
    }

    //the next node will propagate the command at half the distance
    command->node_dist /= 2;
    uint actual_dist = command->node_dist;
    //connect to first subnode
    int rc = eards_remote_connect(nextip1, port);
    if (rc < 0)
    {
        error("propagate_req:Error connecting to node: %s\n", nextip1);
        correct_error(ntohl(ip1), command, port);
    }
    else
    {
        if (!send_command(command)) 
        {
            error("propagate_req: Error propagating command to node %s\n", nextip1);
            eards_remote_disconnect();
            correct_error(ntohl(ip1), command, port);
        }
        else eards_remote_disconnect();
    }
    
    command->node_dist = actual_dist;
    //connect to second subnode
    rc = eards_remote_connect(nextip2, port);
    if (rc < 0)
    {
        error("propagate_req: Error connecting to node: %s\n", nextip2);
        correct_error(ntohl(ip2), command, port);
    }
    else
    {
        if (!send_command(command)) 
        {
            error("propagate_req: Error propagating command to node %s\n", nextip2);
            eards_remote_disconnect();
            correct_error(ntohl(ip2), command, port);
        }
        else eards_remote_disconnect();
    }
}
#endif

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

#if USE_NEW_PROP
int propagate_status(request_t *command, uint port, status_t **status)
{
    status_t **temp_status, *final_status;
    int num_status[NUM_PROPS];
    int rc, i;
    struct sockaddr_in temp;
    unsigned int  current_dist;
		char next_ip[50]; 
    temp_status = calloc(NUM_PROPS, sizeof(status_t *));
    memset(num_status, 0, sizeof(num_status));

    debug("recieved status: total_ips: %d\t self_id: %d\t ips is null: %d\n", total_ips, self_id, ips==NULL);
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

    current_dist = command->node_dist;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((self_id + current_dist + i*NUM_PROPS) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[self_id + current_dist + i*NUM_PROPS];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_dist + i*NUM_PROPS;
			verbose(VCONNECT+2, "Propagating to %s with distance %d\n", next_ip, command->node_dist);

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("propagate_req: Error connecting to node: %s\n", next_ip);
            num_status[i-1] = correct_status(self_id + current_dist + i*NUM_PROPS, total_ips, ips, command, port, &temp_status[i-1]);
        }
        else
        {
            if ((num_status[i-1] = send_status(command, &temp_status[i-1])) < 1) 
            {
                error("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                num_status[i-1] = correct_status(self_id + current_dist + i*NUM_PROPS, total_ips, ips, command, port, &temp_status[i-1]);
            }
            else eards_remote_disconnect();
        }
    }
    
    //memory allocation for final status
    int total_status = 0;
    for (i = 0; i < NUM_PROPS; i++){
				debug("Propagation %d returned %d status",i,num_status[i]);
        total_status += num_status[i];
		}
		debug("Allocating memory for %d status",total_status + 1);
    final_status = calloc(total_status + 1, sizeof(status_t));
    
    //copy results to final status
    int temp_idx = 0;
    for (i = 0; i < NUM_PROPS; i++)
		{
        memcpy(&final_status[temp_idx], temp_status[i], sizeof(status_t)*num_status[i]);
				temp_idx += num_status[i];
		}
	
    //set self ip
    debug("adding self ip in possition %d",total_status);
		temp.sin_addr.s_addr = ips[self_id];
    final_status[total_status].ip = ips[self_id];
    final_status[total_status].ok = STATUS_OK;
    *status = final_status;

    for (i = 0; i < NUM_PROPS; i++)
		{
			debug("Propagation %d had %d status",i,num_status[i]);
			if (num_status[i]>0) free(temp_status[i]);
		}

    return total_status + 1;

}

#else
int propagate_status(request_t *command, uint port, status_t **status)
{
    status_t *status1, *status2, *final_status;
    int num_status1, num_status2;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    unsigned int ip1, ip2, self_ip;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
	char buff[50], nextip1[50], nextip2[50]; 

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

            self_ip = ip1 = ip2 = htonl(saddr->sin_addr.s_addr);
            ip1 += command->node_dist;
            temp.sin_addr.s_addr = ntohl(ip1);
            strcpy(nextip1, inet_ntoa(temp.sin_addr));

            ip2 -= command->node_dist;
            temp.sin_addr.s_addr = ntohl(ip2);
            strcpy(nextip2, inet_ntoa(temp.sin_addr));
        }
    }

    if (command->node_dist < 1)
    {
        final_status = calloc(1, sizeof(status_t));
        final_status[0].ip = ntohl(self_ip);
        final_status[0].ok = STATUS_OK;
        *status = final_status;
        return 1;
    }

    //the next node will propagate the command at half the distance
    command->node_dist /= 2;
    uint actual_dist = command->node_dist;

    //connect to first subnode
    int rc = eards_remote_connect(nextip1, port);
    if (rc < 0)
    {
        error("propagate_status: Error connecting to node: %s\n", nextip1);
        num_status1 = correct_status(ntohl(ip1), command, port, &status1);
    }
    else
    {
        if ((num_status1 = send_status(command, &status1)) < 1)
        {
            error("propagate_status:Error propagating command to node %s\n", nextip1);
            eards_remote_disconnect();
            num_status1 = correct_status(ntohl(ip1), command, port, &status1);
        }
        else eards_remote_disconnect();
    }
    
    command->node_dist = actual_dist;
    //connect to second subnode
    rc = eards_remote_connect(nextip2, port);
    if (rc < 0)
    {
        error("propagate_status: Error connecting to node: %s\n", nextip2);
        num_status2 = correct_status(ntohl(ip2), command, port, &status2);
    }
    else
    {
		debug("Sending status to %s\n",nextip2);
        if ((num_status2 = send_status(command, &status2)) < 1)
        {
            error("propagate_status: Error propagating command to node %s\n", nextip2);
            eards_remote_disconnect();
            num_status2 = correct_status(ntohl(ip2), command, port, &status2);
        }
        else eards_remote_disconnect();
    }
    
    int total_status = num_status1 + num_status2;
    final_status = calloc(total_status + 1, sizeof(status_t));
    memcpy(final_status, status1, sizeof(status_t)*num_status1);
    memcpy(&final_status[num_status1], status2, sizeof(status_t)*num_status2);
    final_status[total_status].ip = ntohl(self_ip);
    final_status[total_status].ok = STATUS_OK;
    *status = final_status;
    free(status1);
    free(status2);
    return total_status + 1;

}
#endif
