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
#include <common/system/time.h>
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
	debug("Sending command %u",command->req);
    
    if (send_data(eards_sfd, sizeof(request_t), (char *)command, EAR_TYPE_COMMAND) != EAR_SUCCESS)
        error("Error sending command");

	ret=read(eards_sfd,&ack,sizeof(ulong));
	if (ret<0){
		printf("ERRO: %d", errno);
		error("Error receiving ack %s",strerror(errno));
	}
	else if (ret!=sizeof(ulong)){
		debug("Error receiving ack: expected %lu ret %d",sizeof(ulong),ret);
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
    request_header_t head;
    head.type = EAR_TYPE_COMMAND;
    head.size = sizeof(request_t);
    ret = write(eards_sfd, &head, sizeof(request_header_t));
    if (ret < sizeof(request_header_t))
    {
        warning("error sending request_header in non_block command");
        return 0;
    }
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

int send_data(int fd, size_t size, char *data, int type)
{
    int ret;
    request_header_t head;
    head.size = size;
    head.type = type;

    debug("sending data of size %lu and type %d", size, type);
    debug("data sizes: %lu and %lu", sizeof(head.size), sizeof(head.type));
    ret = write(fd, &head, sizeof(request_header_t));
    debug("sent head, %d bytes", ret);
    ret = write(fd, data, size);
    debug("sent data, %d bytes", ret);

    return EAR_SUCCESS; 

}

char is_valid_type(int type)
{
    if (type >= MIN_TYPE_VALUE && type <= MAX_TYPE_VALUE) return 1;
    return 0;
}

request_header_t recieve_data(int fd, void **data)
{
    int ret, total, pending;
    request_header_t head;
    char *read_data;
    head.type = 0;
    head.size = 0;

    ret = read(fd, &head, sizeof(request_header_t));
    debug("values read: type %d size %u", head.type, head.size);
    if (ret < 0) {
        error("Error recieving response data header (%s) ", strerror(errno));
        head.type = EAR_ERROR;
        head.size = 0;
        return head;
    }

    if (head.size < 1 || !is_valid_type(head.type)) {
        if (!((head.size == 0) && (head.type == 0))) error("Error recieving response data. Invalid data size (%d) or type (%d).", head.size, head.type);
        head.type = EAR_ERROR;
        head.size = 0;
        return head;
    }
    //write ack should go here if we implement it
    read_data = calloc(head.size, sizeof(char));
    total = 0;
    pending = head.size;

    ret = read(fd, read_data+total, pending);
	if (ret<0){
		error("Error by recieve data (%s)",strerror(errno));
        free(read_data);
        head.type = EAR_ERROR;
        head.size = 0;
		return head;
	}
	total+=ret;
	pending-=ret;
	while ((ret>0) && (pending>0)){
    	ret = read(fd, read_data+total, pending);
    	//ret = recv(eards_sfd, (char *)return_status+total, pending, MSG_DONTWAIT);
		if (ret<0){
		    error("Error by recieve data (%s)",strerror(errno));
            free(read_data);
            head.type = EAR_ERROR;
            head.size = 0;
			return head;
		}
		total+=ret;
		pending-=ret;
	}
    *data = read_data;
    debug("returning from recieve_data with type %d and size %u", head.type, head.size);
	return head;

}

int send_status(request_t *command, status_t **status)
{
    request_header_t head;
    send_command(command);
    head = recieve_data(eards_sfd, (void**)status);
    debug("recieve_data with type %d and size %u", head.type, head.size);
    if (head.type != EAR_TYPE_STATUS) {
        error("Invalid type error, got type %d expected %d", head.type, EAR_TYPE_STATUS);
        if (head.size > 0 && head.type != EAR_ERROR) free(status);
        return EAR_ERROR;
    }

    return (head.size/sizeof(status_t));

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
			debug("Connection already done!");
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
			debug("getaddrinfo fail for %s and %s",nodename,port_number);
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
                    debug("Error geting sockopt");
                    close(sfd);
                    continue;
                }
                else if (optlen != sizeof(int))
                {
                    debug("Error with getsockopt");
                    close(sfd);
                    continue;
                }
                else if (valopt)
                {
                    debug("Error opening connection %s",nodename);
                    close(sfd);
                    continue;
                }
                else debug("Connected");
            }
            else
            {
                debug("Timeout connecting to %s node", nodename);
                close(sfd);
                continue;
            }
            set_socket_block(sfd, 1);
            break;
        }
    }

   	if (rp == NULL) {               /* No address succeeded */
		debug("Failing in connecting to remote eards");
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

int eards_remote_disconnect()
{
	eards_remote_connected=0;
	close(eards_sfd);
	return EAR_SUCCESS;
}


/** REMOTE FUNCTIONS FOR SINGLE NODE COMMUNICATION */
int eards_new_job(new_job_req_t *new_job)
{
	request_t command;

    debug("eards_new_job");
	command.req=EAR_RC_NEW_JOB;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	memcpy(&command.my_req.new_job,new_job,sizeof(new_job_req_t));
	debug("command %u job_id %lu,%lu",command.req,command.my_req.new_job.job.id,command.my_req.new_job.job.step_id);
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
	debug("command %u job_id %lu step_id %lu ",command.req,command.my_req.end_job.jid,command.my_req.end_job.sid);
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

/* Power management */
int eards_set_powerlimit(unsigned long limit)
{
    request_t command;
  	command.node_dist = INT_MAX;
    command.req=EAR_RC_SET_POWER;
    command.time_code = time(NULL);
	command.my_req.pc.limit=limit;
    return send_command(&command);
}

int eards_red_powerlimit(unsigned int type, unsigned long limit)
{
    request_t command;
    command.node_dist = INT_MAX;
    command.req=EAR_RC_RED_POWER;
    command.time_code = time(NULL);
    command.my_req.pc.limit=limit;
    command.my_req.pc.type=type;
    return send_command(&command);
}

int eards_inc_powerlimit(unsigned int type, unsigned long limit)
{
    request_t command;
    command.node_dist = INT_MAX;
    command.req=EAR_RC_INC_POWER;
    command.time_code = time(NULL);
    command.my_req.pc.limit=limit;
    command.my_req.pc.type=type;
    return send_command(&command);
}   

int eards_set_risk(risk_t risk,unsigned long target)
{
    request_t command;
    command.node_dist = INT_MAX;
    command.req=EAR_RC_SET_RISK;
    command.time_code = time(NULL);
	command.my_req.risk.level=risk;
	command.my_req.risk.target=target;
    return send_command(&command);
}

void set_risk_all_nodes(risk_t risk, unsigned long target, cluster_conf_t my_cluster_conf)
{
    request_t command;
    command.req=EAR_RC_SET_RISK;
    command.time_code = time(NULL);
	command.my_req.risk.level=risk;
	command.my_req.risk.target=target;
    send_command_all(command, my_cluster_conf);
}

/* End new functions for power limit management */

int eards_set_policy_info(new_policy_cont_t *p)
{
    request_t command;
    command.req=EAR_RC_SET_POLICY;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	memcpy(&command.my_req.pol_conf,p,sizeof(new_policy_cont_t));
    return send_command(&command);
}

/* END OF SINGLE NODE COMMUNICATION */


/*
*	SAME FUNCTIONALLITY BUT SENT TO ALL NODES
*/

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


void ping_all_nodes_propagated(cluster_conf_t my_cluster_conf)
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

request_header_t correct_data_prop(int target_idx, int total_ips, int *ips, request_t *command, uint port, void **data)
{
    char *temp_data, *final_data = NULL;
    int rc, i, off_ip, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    unsigned int current_dist;
    request_header_t head;
    char next_ip[64];

    current_dist = target_idx - target_idx%NUM_PROPS;
    off_ip = target_idx%NUM_PROPS;

    for ( i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((current_dist*NUM_PROPS + i*NUM_PROPS + off_ip) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[current_dist*NUM_PROPS + i*NUM_PROPS + off_ip];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_dist*NUM_PROPS + i*NUM_PROPS;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            debug("propagate_req:Error connecting to node: %s", next_ip);
            head = correct_data_prop(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port, (void **)&temp_data);
        }
        else
        {
            send_command(command);
            head = recieve_data(rc, (void **)&temp_data);
            if ((head.size) < 1 || head.type == EAR_ERROR)
            {
                debug("propagate_req: Error propagating command to node %s", next_ip);
                eards_remote_disconnect();
                head = correct_data_prop(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port, (void **)&temp_data);
            }
            else eards_remote_disconnect();
        }

        //TODO: data processing, this is a workaround for status_t
        if (head.size > 0 && head.type != EAR_ERROR)
        {
            default_type = head.type;
            final_data = realloc(final_data, final_size + head.size);
            memcpy(&final_data[final_size], temp_data, head.size);
            final_size += head.size;
            free(temp_data);
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
    else if (final_size == 0 && default_type != EAR_ERROR) head.type = EAR_ERROR;


    return head;

}

void correct_error(int target_idx, int total_ips, int *ips, request_t *command, uint port)
{
    if (command->node_dist > total_ips) return;
    struct sockaddr_in temp;

    int i, rc, off_ip;
    unsigned int  current_dist;
    char next_ip[50]; 

    current_dist = command->node_dist;

    current_dist = target_idx - target_idx%NUM_PROPS;
    off_ip = target_idx%NUM_PROPS;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        //check that the next ip exists within the range
        if ((current_dist*NUM_PROPS + i*NUM_PROPS + off_ip) >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[current_dist*NUM_PROPS + i*NUM_PROPS + off_ip];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));

        //prepare next node distance
        command->node_dist = current_dist*NUM_PROPS + i*NUM_PROPS;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            debug("propagate_req:Error connecting to node: %s", next_ip);
            correct_error(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port);
        }
        else
        {
            if (!send_command(command)) 
            {
                debug("propagate_req: Error propagating command to node %s", next_ip);
                eards_remote_disconnect();
                correct_error(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, total_ips, ips, command, port);
            }
            else eards_remote_disconnect();
        }
    }
}

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
                debug("Node %s with distance %d contacted!", next_ip, command.node_dist);
                if (!send_command(&command)) {
                    debug("Error sending command to node %s, trying to correct it", next_ip);
                    eards_remote_disconnect();
                    correct_error(j, ip_counts[i], ips[i], &command, my_cluster_conf.eard.port);
                }
                else eards_remote_disconnect();
            }
            
        }
    }
    for (i = 0; i < total_ranges; i++)
        free(ips[i]);
    free(ips);
    free(ip_counts);

}

request_header_t data_all_nodes(request_t *command, cluster_conf_t *my_cluster_conf, void **data)
{
    int i, j, rc, total_ranges, final_size = 0, default_type = EAR_ERROR;
    int **ips, *ip_counts;
    struct sockaddr_in temp;
    request_header_t head;
    char *temp_data, *all_data = NULL;
    char next_ip[64];
    timestamp rapi_initt,rapi_endt;
    ulong rapi_time;
   
    #if DEBUG_TIME
    timestamp_get(&rapi_initt);
    #endif 
    total_ranges = get_ip_ranges(my_cluster_conf, &ip_counts, &ips);
    #if DEBUG_TIME
    timestamp_get(&rapi_endt);
    rapi_time=timestamp_diff(&rapi_endt,&rapi_initt,TIME_MSECS);
    fprintf(stdout,"Time for get_ip_ranges %lu ms total_ranges %d \n",rapi_time,total_ranges);
    #endif
    for (i = 0; i < total_ranges; i++)
    {
	#if DEBUG_TIME
	fprintf(stdout,"Range %d\n",i);
	#endif
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++)
        {
	    #if DEBUG_TIME
	    fprintf(stdout,"Range %d prop %d\n",i,j);
      	    #endif
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(next_ip, inet_ntoa(temp.sin_addr));

            #if DEBUG_TIME
            timestamp_get(&rapi_initt);
            #endif
            
            rc=eards_remote_connect(next_ip, my_cluster_conf->eard.port);
            if (rc<0){
                debug("Error connecting with node %s, trying to correct it", next_ip);
                head = correct_data_prop(j, ip_counts[i], ips[i], command, my_cluster_conf->eard.port, (void **)&temp_data);
            }
            else{
                debug("Node %s with distance %d contacted!", next_ip, command->node_dist);
                send_command(command);
                head = recieve_data(rc, (void **)&temp_data);
                if (head.size < 1 || head.type == EAR_ERROR) {
                    debug("Error sending command to node %s, trying to correct it", next_ip);
                    eards_remote_disconnect();
                    head = correct_data_prop(j, ip_counts[i], ips[i], command, my_cluster_conf->eard.port, (void **)&temp_data);
                }
                else eards_remote_disconnect();
            }
        
            if (head.size > 0 && head.type != EAR_ERROR)
            {
                head = process_data(head, (char **)&temp_data, (char **)&all_data, final_size);
                free(temp_data);
                final_size = head.size;
                default_type = head.type;
            }
	    #if DEBUG_TIME
	    timestamp_get(&rapi_endt);
	    rapi_time=timestamp_diff(&rapi_endt,&rapi_initt,TIME_MSECS);
	    fprintf(stdout,"Time for %s IP %lu ms \n",next_ip,rapi_time);
	    #endif
            
        }
    }
    head.size = final_size;
    head.type = default_type;
    *data = all_data;

    if (default_type == EAR_ERROR && final_size > 0)
    {
        free(all_data);
        head.size = 0;
    }
    else if (final_size < 1 && default_type != EAR_ERROR) head.type = EAR_ERROR;

    // Freeing allocated memory
    if (total_ranges > 0) {
        for (i = 0; i < total_ranges; i++)
            free(ips[i]);
        free(ip_counts);
        free(ips);
    }

    return head;
}

int status_all_nodes(cluster_conf_t my_cluster_conf, status_t **status)
{
    request_t command;
    status_t *temp_status;
    request_header_t head;
    time_t ctime = time(NULL);
    int num_status = 0;

    command.time_code = ctime;
    command.req = EAR_RC_STATUS;
    command.node_dist = 0;

    head = data_all_nodes(&command, &my_cluster_conf, (void **)&temp_status);
    num_status = head.size / sizeof(status_t);

    if (head.type != EAR_TYPE_STATUS || head.size < sizeof(status_t))
    {
        if (head.size > 0) free (temp_status);
        *status = temp_status;
        return 0;
    }

    *status = temp_status;

    return num_status;

}



request_header_t process_data(request_header_t data_head, char **temp_data_ptr, char **final_data_ptr, int final_size)
{
    char *temp_data = *temp_data_ptr;
    char *final_data = *final_data_ptr;
    request_header_t head;
    head.type = data_head.type;
    switch(data_head.type)
    {
        case EAR_TYPE_STATUS:
            final_data = realloc(final_data, final_size + data_head.size);
            memcpy(&final_data[final_size], temp_data, data_head.size);
            head.size = final_size + data_head.size;
        break;
				#if POWERCAP
        case EAR_TYPE_RELEASED:
						head.size = data_head.size;
            if (final_data != NULL)
            {
               pc_release_data_t *released = (pc_release_data_t *)final_data; 
               pc_release_data_t *new_released = (pc_release_data_t *)temp_data; 
               released->released += new_released->released;
            }
            else
            {
                final_data = realloc(final_data, final_size + data_head.size);
                memcpy(&final_data[final_size], temp_data, data_head.size);
                //cannot directly assign final_data = temp_data because the caller function (data_all_nodes) frees temp_data after passing through here
            }
            break;
        case EAR_TYPE_POWER_STATUS:
            if (final_data != NULL)
            {
                int total_size = 0;
                powercap_status_t *original_status = memmap_powercap_status(final_data, &final_size);
                total_size += final_size - sizeof(powercap_status_t);
                powercap_status_t *new_status = memmap_powercap_status(temp_data, &final_size);
                total_size += final_size;

                char *final_status = calloc(total_size, sizeof(char));
                powercap_status_t *status = (powercap_status_t *)final_status;

								status->total_nodes = original_status->total_nodes + new_status->total_nodes;
                status->idle_nodes = original_status->idle_nodes + new_status->idle_nodes;
                status->released= original_status->released + new_status->released;
                status->num_greedy = original_status->num_greedy + new_status->num_greedy;
                status->requested = original_status->requested + new_status->requested;
                status->current_power = original_status->current_power + new_status->current_power;
                status->total_powercap = original_status->total_powercap + new_status->total_powercap;

                final_size = sizeof(powercap_status_t);
                memcpy(&final_status[final_size], original_status->greedy_nodes, sizeof(int)*original_status->num_greedy);
                final_size += sizeof(int)*original_status->num_greedy;
                memcpy(&final_status[final_size], new_status->greedy_nodes, sizeof(int)*new_status->num_greedy);
                final_size += sizeof(int)*new_status->num_greedy;

                memcpy(&final_status[final_size], original_status->greedy_req, sizeof(uint)*original_status->num_greedy);
                final_size += sizeof(uint)*original_status->num_greedy;
                memcpy(&final_status[final_size], new_status->greedy_req, sizeof(uint)*new_status->num_greedy);
                final_size += sizeof(uint)*new_status->num_greedy;

                memcpy(&final_status[final_size], original_status->extra_power, sizeof(uint)*original_status->num_greedy);
                final_size += sizeof(uint)*original_status->num_greedy;
                memcpy(&final_status[final_size], new_status->extra_power, sizeof(uint)*new_status->num_greedy);
                final_size += sizeof(uint)*new_status->num_greedy;

                final_data = realloc(final_data, final_size);
                memcpy(final_data, final_status, final_size);
                
                status = memmap_powercap_status(final_data, &final_size);
                head.size = final_size;
                free(final_status);

            }
            else
            {
                final_data = realloc(final_data, final_size + data_head.size);
                memcpy(&final_data[final_size], temp_data, data_head.size);
                head.size = data_head.size;
                final_data = (char *)memmap_powercap_status(final_data, &final_size);
                //setting powercap_status pointers to its right value
                //check if final_size == head.size??

            }
        break;
				#endif
    }

    *final_data_ptr = final_data;

    return head;
}
#if POWERCAP

powercap_status_t *mem_alloc_powercap_status(char *final_data)
{   
    int final_size;
    powercap_status_t *status = calloc(1, sizeof(powercap_status_t));
    
    final_size = sizeof(powercap_status_t);
    memcpy(status, final_data, final_size);
    
    status->greedy_nodes = calloc(status->num_greedy, sizeof(int));
    status->greedy_req = calloc(status->num_greedy, sizeof(uint));
    status->extra_power = calloc(status->num_greedy, sizeof(uint));
    
    memcpy(status->greedy_nodes, &final_data[final_size], sizeof(int)*status->num_greedy);
    final_size += status->num_greedy*sizeof(int);
    
    memcpy(status->greedy_req, &final_data[final_size], sizeof(uint)*status->num_greedy);
    final_size += status->num_greedy*sizeof(uint);
    
    memcpy(status->extra_power, &final_data[final_size], sizeof(uint)*status->num_greedy);
    final_size += status->num_greedy*sizeof(uint);
    
    return status;
}

char *mem_alloc_char_powercap_status(powercap_status_t *status)
{
    int size = sizeof(powercap_status_t) + ((status->num_greedy) * (sizeof(int) + sizeof(uint)*2));
    char *data = calloc(size, sizeof(char));

    memcpy(data, status, sizeof(powercap_status_t));
    size = sizeof(powercap_status_t);

    memcpy(&data[size], status->greedy_nodes, sizeof(int) * status->num_greedy);
    size += sizeof(int) * status->num_greedy;

    memcpy(&data[size], status->greedy_req, sizeof(uint) * status->num_greedy);
    size += sizeof(uint) * status->num_greedy;

    memcpy(&data[size], status->extra_power, sizeof(uint) * status->num_greedy);
    size += sizeof(uint) * status->num_greedy;


    return data;
}

powercap_status_t *memmap_powercap_status(char *final_data, int *size)
{
    int final_size;
    powercap_status_t *status = (powercap_status_t *) final_data;

    final_size = sizeof(powercap_status_t);
    status->greedy_nodes = (int *)&final_data[final_size];
    final_size += status->num_greedy*sizeof(int);

    status->greedy_req = (uint *)&final_data[final_size];
    final_size += status->num_greedy*sizeof(uint);

    status->extra_power = (uint *)&final_data[final_size];
    final_size += status->num_greedy*sizeof(uint);

    *size = final_size;
    return status;

}

request_header_t send_powercap_status(request_t *command, powercap_status_t **status)
{
    request_header_t head;
    send_command(command);
    head = recieve_data(eards_sfd, (void**)status);
    debug("recieve_data with type %d and size %u", head.type, head.size);
    if (head.type != EAR_TYPE_POWER_STATUS) {
        error("Invalid type error, got type %d expected %d", head.type, EAR_TYPE_STATUS);
        if (head.size > 0 && head.type != EAR_ERROR) free(status);
        head.size = 0;
        head.type = EAR_ERROR;
        return head;
    }

    //return head.size >= sizeof(powercap_status_t);
    return head;

}

int eards_get_powercap_status(cluster_conf_t my_cluster_conf, powercap_status_t **pc_status) 
{
    int num_temp_status;
    powercap_status_t *temp_status;
    request_t command;

    command.node_dist = 0;
    command.req = EAR_RC_GET_POWERCAP_STATUS;
    command.time_code = time(NULL);
    request_header_t head;

    head = send_powercap_status(&command, &temp_status);
    if (head.size < sizeof(powercap_status_t) || head.type != EAR_TYPE_POWER_STATUS) {
        debug("Error sending command to node");
    }
    *pc_status = temp_status;

    return head.size >= sizeof(powercap_status_t);
}

/** Asks for powercap_status for all nodes */
int cluster_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status)
{
    request_t command;
    powercap_status_t *temp_status;
    request_header_t head;
    int num_status = 0;

    command.time_code = time(NULL);
    command.req = EAR_RC_GET_POWERCAP_STATUS;
    command.node_dist = 0;

    head = data_all_nodes(&command, my_cluster_conf, (void **)&temp_status);
    num_status = head.size / sizeof(powercap_status_t);

    if (head.type != EAR_TYPE_POWER_STATUS || head.size < sizeof(powercap_status_t))
    {
        if (head.size > 0) free (temp_status);
        *pc_status = temp_status;
        num_status = 0;
    }

    *pc_status = temp_status;

    return num_status;

}

/** Asks nodes to release idle power */
int cluster_release_idle_power(cluster_conf_t *my_cluster_conf, pc_release_data_t *released)
{
    request_t command;
    request_header_t head;
    pc_release_data_t *temp_released;

    command.req = EAR_RC_RELEASE_IDLE;
    command.time_code = time(NULL);
    command.node_dist = 0;

    head = data_all_nodes(&command, my_cluster_conf, (void **)&temp_released);
    debug("head.type: %d\t head.size: %d\n", head.type, head.size);

    if (head.type != EAR_TYPE_RELEASED || head.size < sizeof(pc_release_data_t))
    {
        if (head.size > 0) free(temp_released);
        memset(released, 0, sizeof(pc_release_data_t));
        return 0;
    }

    memcpy(released, temp_released, sizeof(pc_release_data_t));
    free(temp_released);

    return 1;
}
/** Send powercap_options to all nodes */
int cluster_set_powercap_opt(cluster_conf_t my_cluster_conf, powercap_opt_t *pc_opt)
{
    request_t command;
    command.req=EAR_RC_SET_POWERCAP_OPT;
    command.time_code = time(NULL);
		command.my_req.pc_opt=*pc_opt;
    send_command_all(command, my_cluster_conf);
	return EAR_SUCCESS;
}
#endif

/* pings all nodes */
void ping_all_nodes(cluster_conf_t my_cluster_conf)
{
    int i, j, k, rc; 
    char node_name[256];
    debug("Sendind ping_all_nodes");
    //it is always secuential as it is only used for debugging purposes
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
                    debug("Node %s ping!", node_name);
                    if (!eards_ping()) error("Error doing ping for node %s", node_name);
                    eards_remote_disconnect();
                }
            }
        }
    }
}
