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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>
#include <common/output/verbose.h>
#include <daemon/eard_rapi.h>
#include <daemon/eard_conf_rapi.h>
#include <daemon/eard_server_api.h>

static int eards_remote_connected=0;
static int eards_sfd=-1;


// Sends a command to eard
int send_command(request_t *command)
{
	ulong ack;
	int ret;
	debug("Sending command %u\n",command->req);
	if ((ret=write(eards_sfd,command,sizeof(request_t)))!=sizeof(request_t)){
		if (ret<0){ 
			error("Error sending command %s\n",strerror(errno));
		}else{ 
			debug("Warning sending command:sent %d ret %d \n",sizeof(request_t),ret);
		}
	}
	ret=read(eards_sfd,&ack,sizeof(ulong));
	//ret=recv(eards_sfd,&ack,sizeof(ulong), MSG_DONTWAIT);
	if (ret<0){
		printf("ERRO: %d\n", errno);
		error("Error receiving ack %s\n",strerror(errno));
	}
	else if (ret!=sizeof(ulong)){
		debug("Error receiving ack: expected %d ret %d\n",sizeof(ulong),ret);
	}
	return (ret==sizeof(ulong)); // Should we return ack ?
}

/*
There is at least one byte available in the send buffer →send succeeds and returns the number of bytes accepted (possibly fewer than you asked for).
The send buffer is completely full at the time you call send.
→if the socket is blocking, send blocks
→if the socket is non-blocking, send fails with EWOULDBLOCK/EAGAIN
An error occurred (e.g. user pulled network cable, connection reset by peer) →send fails with another error
*/
int send_non_block_command(request_t *command)
{
    ulong ack; 
    int ret; 
	int tries=0;
	uint to_send,sended=0;
	uint to_recv,received=0;
	uint must_abort=0;
    debug("Sending command %u",command->req);
	to_send=sizeof(request_t);
	do
	{
		ret=send(eards_sfd, (char *)command+sended,to_send, MSG_DONTWAIT);
		if (ret>0){
			sended+=ret;
			to_send-=ret;
		}else if (ret<0){
			if ((errno==EWOULDBLOCK) || (errno==EAGAIN)) tries++;
			else{	
				must_abort=1;
      	error("Error sending command to eard %s,%d",strerror(errno),errno);
    	}
  	}else if (ret==0){
			warning("send returns 0 bytes");
			must_abort=1;
		}
	}while((tries<MAX_SOCKET_COMM_TRIES) && (to_send>0) && (must_abort==0));
	if (tries>=MAX_SOCKET_COMM_TRIES) debug("tries reached in recv %d",tries);
	/* If there are bytes left to send, we return a 0 */
	if (to_send){ 
		debug("return non blocking command with 0");
		return 0;
	}
	tries=0;
	to_recv=sizeof(ulong);
	do
	{
		ret=recv(eards_sfd, (char *)&ack+received,to_recv, MSG_DONTWAIT);
		if (ret>0){
			received+=ret;
			to_recv-=ret;
		}else if (ret<0){
			if ((errno==EWOULDBLOCK) || (errno==EAGAIN)) tries++;
			else{
				must_abort=1;
				error("Error receiving ack from eard %s,%d",strerror(errno),errno);
  		}
		}else if (ret==0){
			debug("recv returns 0 bytes");
			must_abort=1;
		}
	}while((tries<MAX_SOCKET_COMM_TRIES) && (to_recv>0) && (must_abort==0));

	if (tries>=MAX_SOCKET_COMM_TRIES){
		debug("Max tries reached in recv");
	}
	debug("send_non_block returns with %d",!to_recv);
    return (!to_recv); // Should we return ack ?
}

//specifically sends and reads the ack of a status command
int send_status(request_t *command, status_t **status)
{
	ulong ack;
	int ret;
	int total, pending;
    status_t *return_status;
	debug("Sending command %u\n",command->req);
	if ((ret=write(eards_sfd,command,sizeof(request_t)))!=sizeof(request_t)){
		if (ret<0){ 
			error("Error sending command (status) %s\n",strerror(errno));
		}else{ 
			debug("Error sending command (status) ret=%d expected=%d\n",ret,sizeof(request_t));
		}
	}
	debug("Reading ack size \n");
	/* We assume first long will not block */
	ret=read(eards_sfd,&ack,sizeof(ulong));
	//ret = recv(eards_sfd, &ack, sizeof(ulong), MSG_DONTWAIT);
	if (ret<0){
		error("Error receiving ack in (status) (%s) \n",strerror(errno));
        return EAR_ERROR;
	}
    if (ack < 1){
        error("Number of status expected is not valid: %lu", ack);
        return EAR_ERROR;
    }
	debug("Waiting for %d ack bytes\n",ack);
    return_status = calloc(ack, sizeof(status_t));
	if (return_status==NULL){
		error("Not enough memory at send_status");
		return EAR_ERROR;
	}
	total=0;
	pending=sizeof(status_t)*ack;
    ret = read(eards_sfd, (char *)return_status+total, pending);
    //ret = recv(eards_sfd, (char *)return_status+total, pending, MSG_DONTWAIT);
	if (ret<0){
		error("Error by reading status (%s)",strerror(errno));
        free(return_status);
		return EAR_ERROR;
	}
	total+=ret;
	pending-=ret;
	while ((ret>0) && (pending>0)){
    	ret = read(eards_sfd, (char *)return_status+total, pending);
    	//ret = recv(eards_sfd, (char *)return_status+total, pending, MSG_DONTWAIT);
		if (ret<0){
			error("Error by reading status (%s)",strerror(errno));
        	free(return_status);
			return EAR_ERROR;
		}
		total+=ret;
		pending-=ret;
	}
    *status = return_status;
	debug("Returning from send_status with %d\n",ack);
	return ack;
}

int set_socket_block(int sfd, char blocking)
{
    if (sfd < 0) return EAR_ERROR;

    int flags = fcntl(sfd, F_GETFL, 0);
    //if flags is -1 fcntl returned an error
    if (flags == -1) return EAR_ERROR;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    if (fcntl(sfd, F_SETFL, flags) != 0) return EAR_ERROR;
    return EAR_ERROR;
}

// based on getaddrinfo  man page
int eards_remote_connect(char *nodename,uint port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
		char port_number[50]; 	// that size needs to be validated
    int sfd, s;
    fd_set set;

		if (eards_remote_connected){ 
			debug("Connection already done!\n");
			return eards_sfd;
		}
   	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

		sprintf(port_number,"%d",port);
   	s = getaddrinfo(nodename, port_number, &hints, &result);
    if (s != 0) {
			debug("getaddrinfo fail for %s and %s\n",nodename,port_number);
			return EAR_ERROR;
    }

    struct timeval timeout;
    memset(&timeout, 0, sizeof(struct timeval));
    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;
    socklen_t  optlen;
		int valopt, sysret;

   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1) // if we can not create the socket we continue
            continue;

        set_socket_block(sfd, 0);
        
        int res = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (res < 0 && errno != EINPROGRESS)
        {
            close(sfd);
            continue;
        }
        if (res == 0) //sucessful connection
        {
            set_socket_block(sfd, 1);
            break;
        }
        else
        {
            FD_ZERO(&set);
            FD_SET(sfd, &set);
            if (select(sfd+1, &set, &set, NULL, &timeout) > 0) 
            {
                optlen = sizeof(int);
                sysret = getsockopt(sfd, SOL_SOCKET, SO_ERROR, (void *)(&valopt), &optlen);
                if (sysret)
                {
                    debug("Error geting sockopt\n");
                    close(sfd);
                    continue;
                }
                else if (optlen != sizeof(int))
                {
                    debug("Error with getsockopt\n");
                    close(sfd);
                    continue;
                }
                else if (valopt)
                {
                    debug("Error opening connection %s",nodename);
                    close(sfd);
                    continue;
                }
                else debug("Connected\n");
            }
            else
            {
                debug("Timeout connecting to %s node\n", nodename);
                close(sfd);
                continue;
            }
            set_socket_block(sfd, 1);
            break;
        }
    }

   	if (rp == NULL) {               /* No address succeeded */
		debug("Failing in connecting to remote eards\n");
		return EAR_ERROR;
    }

    char conection_ok = 0;

    timeout.tv_sec = 1;

    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (void *)(&timeout), sizeof(timeout));
    
    if (read(sfd, &conection_ok, sizeof(char)) > 0)
    {
        debug("Handshake with server completed.");
    }
    else
    {
        debug("Couldn't complete handshake with server, closing conection.");
        close(sfd);
        return EAR_ERROR;
    }

    memset(&timeout, 0, sizeof(struct timeval));
    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (void *)(&timeout), sizeof(timeout));

   	freeaddrinfo(result);           /* No longer needed */
	eards_remote_connected=1;
	eards_sfd=sfd;
	return sfd;

}

int eards_new_job(application_t *new_job)
{
	request_t command;

    debug("eards_new_job");
	command.req=EAR_RC_NEW_JOB;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	copy_application(&command.my_req.new_job,new_job);
	debug("command %u job_id %d,%d\n",command.req,command.my_req.new_job.job.id,command.my_req.new_job.job.step_id);
	return send_non_block_command(&command);
}

int eards_end_job(job_id jid,job_id sid)
{
    request_t command;

    debug("eards_end_job");
    command.req=EAR_RC_END_JOB;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	command.my_req.end_job.jid=jid;
	command.my_req.end_job.sid=sid;
//	command.my_req.end_job.status=status;
	debug("command %u job_id %d step_id %d \n",command.req,command.my_req.end_job.jid,command.my_req.end_job.sid);
	return send_non_block_command(&command);
}

int eards_set_max_freq(unsigned long freq)
{
	request_t command;
	command.req=EAR_RC_MAX_FREQ;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.max_freq=freq;
	return send_command(&command);
}

int eards_set_freq(unsigned long freq)
{
    request_t command;
    command.req=EAR_RC_SET_FREQ;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.max_freq=freq;
    return send_command(&command);
}
int eards_set_def_freq(unsigned long freq)
{
    request_t command;
    command.req=EAR_RC_DEF_FREQ;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.max_freq=freq;
    return send_command(&command);
}


int eards_red_max_and_def_freq(uint p_states)
{
    request_t command;
    command.req=EAR_RC_RED_PSTATE;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.p_states=p_states;
    return send_command(&command);
}

int eards_restore_conf()
{
    request_t command;
    command.req=EAR_RC_REST_CONF;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    return send_command(&command);
}



// New th must be passed as % th=0.75 --> 75
int eards_set_th(unsigned long th)
{
    request_t command;
    command.req=EAR_RC_NEW_TH;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.th=th;
    return send_command(&command);
}

// New th must be passed as % th=0.05 --> 5
int eards_inc_th(unsigned long th)
{
    request_t command;
    command.req=EAR_RC_INC_TH;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.th=th;
    return send_command(&command);
}
int eards_ping()
{
    request_t command;
    command.req=EAR_RC_PING;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    return send_command(&command);
}
int eards_set_powercap(unsigned long pc)
{
    request_t command;
    command.req=EAR_RC_NEW_POWERCAP;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
		command.my_req.pc=pc;
    return send_command(&command);
}


int eards_set_policy_info(new_policy_cont_t *p)
{
    request_t command;
    command.req=EAR_RC_SET_POLICY;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	memcpy(&command.my_req.pol_conf,p,sizeof(new_policy_cont_t));
    return send_command(&command);
}


int eards_remote_disconnect()
{
	eards_remote_connected=0;
	close(eards_sfd);
	return EAR_SUCCESS;
}


/*
*	SAME FUNCTIONALLITY BUT SENT TO ALL NODES
*/
void old_increase_th_all_nodes(ulong th, cluster_conf_t my_cluster_conf)
{
	int i, j, k, rc;
    char node_name[256];
	debug("Sending old_increase_th_all_nodes \n");

    for (i=0;i < my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {
                if (k == -1)
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10)
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                }
    	        rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
        	    if (rc<0){
	    		    debug("Error connecting with node %s", node_name);
            	}else{
	        		debug("Increasing the PerformanceEfficiencyGain in node %s by %lu\n", node_name,th);
		        	if (!eards_inc_th(th)) debug("Error increasing the th for node %s", node_name);
			        eards_remote_disconnect();
        		}
	        }
        }
    }
}

void old_red_max_freq_all_nodes(ulong ps, cluster_conf_t my_cluster_conf)
{
	int i, j, k, rc;
    char node_name[256];
	debug("Sending old_red_max_freq_all_nodes\n");
    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {
                if (k == -1)
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10)
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                }
    	        rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
        	    if (rc<0){
	    		    debug("Error connecting with node %s", node_name);
            	}else{
    
                debug("Reducing  the frequency in node %s by %lu\n", node_name,ps);
		        	if (!eards_red_max_and_def_freq(ps)) debug("Error reducing the max freq for node %s", node_name);
			        eards_remote_disconnect();
        		}
	        }
        }
    }
}

void old_ping_all_nodes(cluster_conf_t my_cluster_conf)
{
    int i, j, k, rc; 
    char node_name[256];
	debug("Sengind old_ping_all_nodes\n");
    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {   
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {   
                if (k == -1) 
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10) 
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                }   
                rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
                if (rc<0){
                    error("Error connecting with node %s", node_name);
                }else{

                    debug("Node %s ping!\n", node_name);
                    if (!eards_ping()) error("Error doing ping for node %s", node_name);
                    eards_remote_disconnect();
                }
            }
        }
    }
}

void increase_th_all_nodes(ulong th, ulong p_id, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_INC_TH;
    command.my_req.ear_conf.th=th;
    command.my_req.ear_conf.p_id=p_id;
    send_command_all(command, my_cluster_conf);
}

void set_th_all_nodes(ulong th, ulong p_id, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_NEW_TH;
    command.my_req.ear_conf.th=th;
    command.my_req.ear_conf.p_id=p_id;
    send_command_all(command, my_cluster_conf);
}

void ping_all_nodes(cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req = EAR_RC_PING;
    send_command_all(command, my_cluster_conf);
}

void set_max_freq_all_nodes(ulong max_freq, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_MAX_FREQ;
    command.my_req.ear_conf.max_freq = max_freq;
    send_command_all(command, my_cluster_conf);
}

void set_freq_all_nodes(ulong freq, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_SET_FREQ;
    command.my_req.ear_conf.max_freq = freq;
    send_command_all(command, my_cluster_conf);
}

void red_def_max_pstate_all_nodes(ulong pstate, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_RED_PSTATE;
    command.my_req.ear_conf.p_states = pstate;
    send_command_all(command, my_cluster_conf);
}

/** Sets the default pstate for a given policy in all nodes */
void set_def_pstate_all_nodes(uint pstate,ulong pid,cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_SET_DEF_PSTATE;
    command.my_req.ear_conf.p_states = pstate;
		command.my_req.ear_conf.p_id = pid;
    send_command_all(command, my_cluster_conf);
}

/** Sets the maximum pstate in all the nodes */
void set_max_pstate_all_nodes(uint pstate,cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_SET_MAX_PSTATE;
    command.my_req.ear_conf.p_states = pstate;
    send_command_all(command, my_cluster_conf);
}


void reduce_frequencies_all_nodes(ulong freq, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_DEF_FREQ;
    command.my_req.ear_conf.max_freq=freq;
    send_command_all(command, my_cluster_conf);
}

void restore_conf_all_nodes(cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_REST_CONF;
    send_command_all(command, my_cluster_conf);
}

void set_def_freq_all_nodes(ulong freq, ulong policy, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_DEF_FREQ;
    command.my_req.ear_conf.max_freq = freq;
	command.my_req.ear_conf.p_id = policy;
    send_command_all(command, my_cluster_conf);
}

#if USE_NEW_PROP
int correct_status(int target_idx, int total_ips, int *ips, request_t *command, uint port, status_t **status)
{
    status_t **temp_status, *final_status;
    int num_status[NUM_PROPS];
    int rc, i;
    struct sockaddr_in temp;
    unsigned int  current_dist;
		char next_ip[50]; 
    memset(num_status, 0, sizeof(num_status));
    temp_status = calloc(NUM_PROPS, sizeof(status_t*));

		debug("correct_status for ip %d with distance %d\n",ips[target_idx],command->node_dist);
    if (command->node_dist > total_ips)
    {
        final_status = calloc(1, sizeof(status_t));
        final_status[0].ip = ips[target_idx];
        final_status[0].ok = STATUS_BAD;
        *status = final_status;
        return 1;
    }

    current_dist = command->node_dist;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((target_idx + current_dist + i*NUM_PROPS) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[target_idx + current_dist + i*NUM_PROPS];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_dist + i*NUM_PROPS;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            debug("propagate_req:Error connecting to node: %s\n", next_ip);
            num_status[i-1] = correct_status(target_idx + current_dist + i*NUM_PROPS, total_ips, ips, command, port, &temp_status[i-1]);
        }
        else
        {
            if ((num_status[i-1] = send_status(command, &temp_status[i-1])) < 1) 
            {
                debug("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                num_status[i-1] = correct_status(target_idx + current_dist + i*NUM_PROPS, total_ips, ips, command, port, &temp_status[i-1]);
            }
            else eards_remote_disconnect();
        }
    }

    //memory allocation for final status
    int total_status = 0;
    for (i = 0; i < NUM_PROPS; i++){
        total_status += num_status[i];
		}
		debug("Total status collected from propagation %d",total_status);    
    final_status = calloc(total_status + 1, sizeof(status_t));
    
    //copy results to final status
    int temp_idx = 0;
    for (i = 0; i < NUM_PROPS; i++)
	{
        memcpy(&final_status[temp_idx], temp_status[i], sizeof(status_t)*num_status[i]);
		temp_idx += num_status[i];
	}

    //set self ip
    final_status[total_status].ip = ips[target_idx];
    final_status[total_status].ok = STATUS_BAD;
    *status = final_status;


    for (i = 0; i < NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((target_idx + current_dist + (i+1)*NUM_PROPS) >= total_ips) break;
        free(temp_status[i]);
    }
    return total_status + 1;

}

#else
int correct_status(uint target_ip, request_t *command, uint port, status_t **status)
{
    status_t *final_status, *status1 = NULL, *status2 = NULL;
    int total_status, num_status1 = 0, num_status2 = 0;
		debug("correct_status for ip %d with distance %d\n",target_ip,command->node_dist);
    if (command->node_dist < 1) {
        final_status = calloc(1, sizeof(status_t));
        final_status[0].ip = target_ip;
        final_status[0].ok = STATUS_BAD;
        *status = final_status;
        return 1;
    }

    char nextip1[50], nextip2[50];

    struct sockaddr_in temp;
    unsigned int self_ip, ip1, ip2; 
    self_ip = ip1 = ip2 = htonl(target_ip);
    ip1 += command->node_dist;
    temp.sin_addr.s_addr = ntohl(ip1);

    strcpy(nextip1, inet_ntoa(temp.sin_addr));

    ip2 -= command->node_dist;
    temp.sin_addr.s_addr = ntohl(ip2);
    strcpy(nextip2, inet_ntoa(temp.sin_addr));

    //the next node will propagate the command at half the distance
    command->node_dist /= 2;
    int actual_dist = command->node_dist;
    //connect to first subnode
    int rc = eards_remote_connect(nextip1, port);
    if (rc < 0)
    {
        debug("Error connecting to node: %s\n", nextip1);
        num_status1 = correct_status(ntohl(ip1), command, port, &status1);
    }
    else
    {
		debug("connection ok, sending status requests %s\n",nextip1);
        if ((num_status1 = send_status(command, &status1)) < 1)
        {
            debug("Error propagating command to node %s\n", nextip1);
            eards_remote_disconnect();
            num_status1 = correct_status(ntohl(ip1), command, port, &status1);
        }
        else eards_remote_disconnect();
    }

	debug("Correcting second node\n");

    command->node_dist = actual_dist;
    //connect to second subnode
    rc = eards_remote_connect(nextip2, port);
    if (rc < 0)
    {
        debug("Error connecting to node: %s\n", nextip2);
        num_status2 = correct_status(ntohl(ip2), command, port, &status2);
    }
    else
    {
		debug("connection ok, sending status requests %s\n",nextip2);
        if ((num_status2 = send_status(command, &status2)) < 1)
        {
            debug("Error propagating command to node %s\n", nextip2);
            eards_remote_disconnect();
            num_status2 = correct_status(ntohl(ip2), command, port, &status2);
        }
        else eards_remote_disconnect();
    } 

    total_status = num_status1 + num_status2;
    final_status = calloc(total_status + 1, sizeof(status_t));
    memcpy(final_status, status1, sizeof(status_t)*num_status1);
    memcpy(&final_status[num_status1], status2, sizeof(status_t)*num_status2);
    final_status[total_status].ip = ntohl(self_ip);
    final_status[total_status].ok = STATUS_BAD;
    *status = final_status;
    free(status1);
    free(status2);
		debug("correct_status ends return value=%d\n",total_status + 1);
    return total_status + 1;
}
#endif

#if USE_NEW_PROP
void correct_error(int target_idx, int total_ips, int *ips, request_t *command, uint port)
{
    if (command->node_dist > total_ips) return;
    struct sockaddr_in temp;

    int i, rc;
    unsigned int  current_dist;
		char next_ip[50]; 

    current_dist = command->node_dist;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((target_idx + current_dist + i*NUM_PROPS) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[target_idx + current_dist + i*NUM_PROPS];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));

        //prepare next node distance
        command->node_dist = current_dist + i*NUM_PROPS;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            debug("propagate_req:Error connecting to node: %s\n", next_ip);
            correct_error(target_idx + current_dist + i*NUM_PROPS, total_ips, ips, command, port);
        }
        else
        {
            if (!send_command(command)) 
            {
                debug("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                correct_error(target_idx + current_dist + i*NUM_PROPS, total_ips, ips, command, port);
            }
            else eards_remote_disconnect();
        }
    }
}
#else
void correct_error(uint target_ip, request_t *command, uint port)
{
    if (command->node_dist < 1) return;
    char nextip1[50], nextip2[50];

    struct sockaddr_in temp;
    unsigned int ip1, ip2; 
    ip1 = ip2 = htonl(target_ip);
    ip1 += command->node_dist;
    temp.sin_addr.s_addr = ntohl(ip1);

    strcpy(nextip1, inet_ntoa(temp.sin_addr));

    ip2 -= command->node_dist;
    temp.sin_addr.s_addr = ntohl(ip2);
    strcpy(nextip2, inet_ntoa(temp.sin_addr));

    //the next node will propagate the command at half the distance
    command->node_dist /= 2;
    int actual_dist = command->node_dist;
    //connect to first subnode
    int rc = eards_remote_connect(nextip1, port);
    if (rc < 0)
    {
        debug("Error connecting to node: %s\n", nextip1);
        correct_error(ntohl(ip1), command, port);
    }
    else
    {
        if (!send_command(command))
        {
            debug("Error propagating command to node %s\n", nextip1);
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
        debug("Error connecting to node: %s\n", nextip2);
        correct_error(ntohl(ip2), command, port);
    }
    else
    {
        if (!send_command(command))
        {
            debug("Error propagating command to node %s\n", nextip2);
            eards_remote_disconnect();
            correct_error(ntohl(ip2), command, port);
        }
        else eards_remote_disconnect();
    } 
}
#endif

#if !USE_NEW_PROP
int correct_status_starter(char *host_name, request_t *command, uint port, status_t **status)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    int ip1, ip2;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    int host_ip = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

   	s = getaddrinfo(host_name, NULL, &hints, &result);
    if (s != 0) {
		debug("getaddrinfo fails for host %s (%s)\n",host_name,strerror(errno));
		return EAR_ERROR;
    }

   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in*) (rp->ai_addr);
            host_ip = saddr->sin_addr.s_addr;
        }
    }
    freeaddrinfo(result);
    return correct_status(host_ip, command, port, status);
}

void correct_error_starter(char *host_name, request_t *command, uint port)
{
	if (command->node_dist < 1) return;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    int ip1, ip2;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    int host_ip = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

   	s = getaddrinfo(host_name, NULL, &hints, &result);
    if (s != 0) {
		debug("getaddrinfo fails for host %s (%s)\n",host_name,strerror(errno));
		return;
    }

   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in*) (rp->ai_addr);
            host_ip = saddr->sin_addr.s_addr;
        }
    }
    freeaddrinfo(result);
    correct_error(host_ip, command, port);
}
#endif

#if USE_NEW_PROP
void send_command_all(request_t command, cluster_conf_t my_cluster_conf)
{
    int i, j,  rc, total_ranges;
    int **ips, *ip_counts;
    struct sockaddr_in temp;
    char next_ip[256];
    time_t ctime = time(NULL);
    command.time_code = ctime;
    total_ranges = get_ip_ranges(&my_cluster_conf, &ip_counts, &ips);
    for (i = 0; i < total_ranges; i++)
    {
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++)
        {
            command.node_dist = 0;
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(next_ip, inet_ntoa(temp.sin_addr));
            
            rc=eards_remote_connect(next_ip, my_cluster_conf.eard.port);
            if (rc<0){
                debug("Error connecting with node %s, trying to correct it", next_ip);
                correct_error(j, ip_counts[i], ips[i], &command, my_cluster_conf.eard.port);
            }
            else{
                debug("Node %s with distance %d contacted!\n", next_ip, command.node_dist);
                if (!send_command(&command)) {
                    debug("Error sending command to node %s, trying to correct it", next_ip);
                    correct_error(j, ip_counts[i], ips[i], &command, my_cluster_conf.eard.port);
                }
                eards_remote_disconnect();
            }
            
        }
    }
    for (i = 0; i < total_ranges; i++)
        free(ips[i]);
    free(ips);
    free(ip_counts);

}
#else
void send_command_all(request_t command, cluster_conf_t my_cluster_conf)
{
    int i, j, k, rc; 
    char node_name[256];
    time_t ctime = time(NULL);
	debug("send_command_all %d\n",command.req);
    command.time_code = ctime;
    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {   
            k = my_cluster_conf.islands[i].ranges[j].start;
            command.node_dist = 0;
            if (k == -1) 
                sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
            else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
            else {
                k += (my_cluster_conf.islands[i].ranges[j].end - my_cluster_conf.islands[i].ranges[j].start)/2;
                if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10) 
                    sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                else 
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 

                command.node_dist = (my_cluster_conf.islands[i].ranges[j].end - my_cluster_conf.islands[i].ranges[j].start)/2;
                int t = 1;
                while (t < command.node_dist) t *= 2;
                command.node_dist = t;
            }   
            
            /*#if USE_EXT
            strcat(node_name, NW_EXT);
            #endif*/
            if (strlen(my_cluster_conf.net_ext))
                strcat(node_name, my_cluster_conf.net_ext);

            rc=eards_remote_connect(node_name, my_cluster_conf.eard.port);
            if (rc<0){
                debug("Error connecting with node %s, trying to correct it", node_name);
                correct_error_starter(node_name, &command, my_cluster_conf.eard.port);
            }
            else{
                debug("Node %s with distance %d contacted!\n", node_name, command.node_dist);
                if (!send_command(&command)) {
                    debug("Error sending command to node %s, trying to correct it", node_name);
                    correct_error_starter(node_name, &command, my_cluster_conf.eard.port);
                }
                eards_remote_disconnect();
            }
        }
    }
}
#endif

#if USE_NEW_PROP
int status_all_nodes(cluster_conf_t my_cluster_conf, status_t **status)
{
    int i, j,  rc, total_ranges, num_all_status = 0, num_temp_status;
    int **ips, *ip_counts;
    struct sockaddr_in temp;
    status_t *temp_status, *all_status = NULL;
    request_t command;
    char next_ip[256];
    time_t ctime = time(NULL);
    
    command.time_code = ctime;
    command.req = EAR_RC_STATUS;

    total_ranges = get_ip_ranges(&my_cluster_conf, &ip_counts, &ips);
    for (i = 0; i < total_ranges; i++)
    {
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++)
        {
            command.node_dist = 0;
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(next_ip, inet_ntoa(temp.sin_addr));
            
            rc=eards_remote_connect(next_ip, my_cluster_conf.eard.port);
            if (rc<0){
                debug("Error connecting with node %s, trying to correct it", next_ip);
                num_temp_status = correct_status(j, ip_counts[i], ips[i], &command, my_cluster_conf.eard.port, &temp_status);
            }
            else{
                debug("Node %s with distance %d contacted!\n", next_ip, command.node_dist);
                if ((num_temp_status = send_status(&command, &temp_status)) < 1) {
                    debug("Error sending command to node %s, trying to correct it", next_ip);
                    num_temp_status = correct_status(j, ip_counts[i], ips[i], &command, my_cluster_conf.eard.port, &temp_status);
                }
                eards_remote_disconnect();
            }
        
            if (num_temp_status > 0)
            {
                all_status = realloc(all_status, sizeof(status_t)*(num_all_status+num_temp_status));
                memcpy(&all_status[num_all_status], temp_status, sizeof(status_t)*num_temp_status);
                free(temp_status);
                num_all_status += num_temp_status;
            }
            else
            {
                debug("Connection to node %s returned 0 status\n", next_ip)
            }
            
        }
    }
    *status = all_status;

    return num_all_status;
}
#else
int status_all_nodes(cluster_conf_t my_cluster_conf, status_t **status)
{
    int i, j, k, rc; 
    char node_name[256];
    status_t *temp_status, *all_status = NULL;
    int num_all_status = 0, num_temp_status;
    request_t command;
    time_t ctime = time(NULL);
    command.time_code = ctime;
    command.req = EAR_RC_STATUS;
    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {   
            num_temp_status = 0;
            k = my_cluster_conf.islands[i].ranges[j].start;
            command.node_dist = 0;
            if (k == -1) 
                sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
            else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
            else {
                k += (my_cluster_conf.islands[i].ranges[j].end - my_cluster_conf.islands[i].ranges[j].start)/2;
                if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10) 
                    sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                else 
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 

                command.node_dist = (my_cluster_conf.islands[i].ranges[j].end - my_cluster_conf.islands[i].ranges[j].start)/2 + 1;
                int t = 1;
                while (t < command.node_dist) t *= 2;
                command.node_dist = t;
            }   
            /*#if USE_EXT
            strcat(node_name, NW_EXT);
            #endif*/
            if (strlen(my_cluster_conf.net_ext) > 0)
                strcat(node_name, my_cluster_conf.net_ext);

            rc=eards_remote_connect(node_name, my_cluster_conf.eard.port);
            if (rc<0){
                debug("Error connecting with node %s, trying to correct it", node_name);
                num_temp_status = correct_status_starter(node_name, &command, my_cluster_conf.eard.port, &temp_status);
            }
            else{
                debug("Node %s with distance %d contacted with status!", node_name, command.node_dist);
                if ((num_temp_status = send_status(&command, &temp_status)) < 1) {
                    debug("Error doing status for node %s, trying to correct it", node_name);
                    num_temp_status = correct_status_starter(node_name, &command, my_cluster_conf.eard.port, &temp_status);
                }
                eards_remote_disconnect();
            }
            if (num_temp_status > 0)
            {
                all_status = realloc(all_status, sizeof(status_t)*(num_all_status+num_temp_status));
                memcpy(&all_status[num_all_status], temp_status, sizeof(status_t)*num_temp_status);
                free(temp_status);
                num_all_status += num_temp_status;
            }

        }
    }
    *status = all_status;
    return num_all_status;
}
#endif

void old_red_def_freq_all_nodes(ulong ps, cluster_conf_t my_cluster_conf)
{
	int i, j, k, rc;
    char node_name[256];
    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {
                if (k == -1)
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10)
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                }
    	        rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
        	    if (rc<0){
	    		    debug("Error connecting with node %s", node_name);
            	}else{
                	debug("Reducing  the default and maximumfrequency in node %s by %lu\n", node_name,ps);
		        	if (!eards_red_max_and_def_freq(ps)) debug("Error reducing the default freq for node %s", node_name);
			        eards_remote_disconnect();
        		}
	        }
        }
    }
}



void old_reduce_frequencies_all_nodes(ulong freq, cluster_conf_t my_cluster_conf)
{
    int i, j, k, rc;
    char node_name[256];

    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {
                if (k == -1)
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10)
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    
                }

                rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
                if (rc<0){
                    debug("Error connecting with node %s",node_name);
                }else{
                	debug("Setting  the frequency in node %s to %lu\n", node_name, freq);
                	if (!eards_set_freq(freq)) debug("Error reducing the freq for node %s", node_name);
            	    eards_remote_disconnect();
		        }
            }
        }
    }
}

void old_set_def_freq_all_nodes(ulong freq, cluster_conf_t my_cluster_conf)
{
    int i, j, k, rc;
    char node_name[256];

    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {
                if (k == -1)
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10)
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    
                }

                rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
                if (rc<0){
                    debug("Error connecting with node %s",node_name);
                }else{
                	debug("Setting  the frequency in node %s to %lu\n", node_name, freq);
                	if (!eards_set_def_freq(freq)) debug("Error setting the freq for node %s", node_name);
            	    eards_remote_disconnect();
		        }
            }
        }
    }
}

void old_restore_conf_all_nodes(cluster_conf_t my_cluster_conf)
{
    int i, j, k, rc;
    char node_name[256];

    for (i=0;i< my_cluster_conf.num_islands;i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {
                if (k == -1)
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10)
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k);
                    
                }

                rc=eards_remote_connect(node_name,my_cluster_conf.eard.port);
                if (rc<0){
                    debug("Error connecting with node %s",node_name);
                }else{
                	debug("Restoring the configuartion in node %s\n", node_name);
                	if (!eards_restore_conf()) debug("Error restoring the configuration for node %s", node_name);
            	    eards_remote_disconnect();
		        }
            }
        }
    }

}
