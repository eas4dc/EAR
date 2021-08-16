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
#include <common/output/verbose.h>

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>

#include <common/messaging/msg_conf.h>
#include <common/messaging/msg_internals.h>


int eards_remote_connected=0;
int eards_sfd=-1;

// 2000 and 65535
#define DAEMON_EXTERNAL_CONNEXIONS 1


int _read(int fd, void *data, size_t ssize)
{

    int ret;
    int tries = 0;
    uint to_read, read = 0;
    uint must_abort = 0;

	to_read = ssize;
	do
	{
        ret = recv(fd, (char *)data+read, to_read, MSG_DONTWAIT);
        if (ret > 0) 
        {
            read += ret;
            to_read -= ret;
        }
        else if (ret < 0)
        {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) tries++;
            else {	
                must_abort = 1;
                error("_read: Error sending command to eard %s,%d", strerror(errno), errno);
            }
        }else if (ret == 0) {
            debug("_read: read returns 0 bytes");
            must_abort = 1;
        }
	} while ((tries < MAX_SOCKET_COMM_TRIES) && (to_read > 0) && (must_abort == 0)); //still pending to send and hasn't tried more than MAX_TRIES

	if (tries >= MAX_SOCKET_COMM_TRIES) { 
        debug("_read: tries reached in send %d", tries); 
    }

	/* If there are bytes left to send, we return a 0 */
	if (to_read) { 
		debug("_read: return non blocking command with 0");
		return read;
	}
    debug("_read: exiting send successfully");
    return read;

}

int _write(int fd, void *data, size_t ssize) 
{
    int ret;
    int tries = 0;
    uint to_send, sent = 0;
    uint must_abort = 0;

	to_send = ssize;
	do
	{
        ret = send(fd, (char *)data+sent, to_send, MSG_DONTWAIT);
        if (ret > 0) 
        {
            sent += ret;
            to_send -= ret;
        }
        else if (ret < 0)
        {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) tries++;
            else {	
                must_abort = 1;
                error("_write: Error sending command to eard %s,%d", strerror(errno), errno);
            }
        }else if (ret == 0) {
            debug("_write: send returns 0 bytes");
            must_abort = 1;
        }
	} while ((tries < MAX_SOCKET_COMM_TRIES) && (to_send > 0) && (must_abort == 0)); //still pending to send and hasn't tried more than MAX_TRIES

	if (tries >= MAX_SOCKET_COMM_TRIES) { 
        debug("_write: tries reached in send %d", tries); 
    }

	/* If there are bytes left to send, we return a 0 */
	if (to_send) { 
		debug("_write: return non blocking command with 0");
		return sent;
	}
    debug("_write: exiting send successfully");
    return sent;

}


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
        debug("getaddrinfo fails for port %s (%s)",buff,strerror(errno));
        return EAR_ERROR;
    }

   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sfd == -1)
            continue;

        while (bind(sfd, rp->ai_addr, rp->ai_addrlen) != 0)
        { 
            verbose(VAPI,"Waiting for connection");
            sleep(10);
	    }
        break;   /* Success */

    }

   	if (rp == NULL) {   /* No address succeeded */
        error("bind fails for eards server (%s) ",strerror(errno));
        return EAR_ERROR;
    } else {
        verbose(VAPI+1,"socket and bind for erads socket success");
    }

   	freeaddrinfo(result);   /* No longer needed */

   	if (listen(sfd, DAEMON_EXTERNAL_CONNEXIONS) < 0)
    {
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
    if (new_sock < 0)
    { 
		error("accept for eards socket fails %s\n",strerror(errno));
		return EAR_ERROR;
	}

    char conection_ok = 1;
    _write(new_sock, &conection_ok, sizeof(char));

    verbose(VCONNECT, "Sending handshake byte to client.");
	verbose(VCONNECT, "new connection ");

	return new_sock;
}

void close_server_socket(int sock)
{
	close(sock);
}

#if DYNAMIC_COMMANDS
int read_command(int s, request_t *command)
{
    request_header_t head;
    char *tmp_command;
    size_t aux_size = 0;


    head = receive_data(s, (void **)&tmp_command);
    debug("read_command:received command type %d\t size: %u \t sizeof req: %lu", head.type, head.size, sizeof(request_t));

    if (head.type != EAR_TYPE_COMMAND || head.size < sizeof(internal_request_t))
    {
        debug("read_command:NO_COMMAND");
        command->req = NO_COMMAND;
        if (head.size > 0) free(tmp_command);
        return EAR_SOCK_DISCONNECTED;
        return command->req;
    }
    memcpy(command, tmp_command, sizeof(internal_request_t));
    aux_size += sizeof(internal_request_t);

#if NODE_PROP
    if (command->nodes > 0)
    {
        command->nodes = calloc(command->num_nodes, sizeof(int)); //this will be freed in propagate_data since it's the last point where it is needed and the common point in all requests
        memcpy(command->nodes, &tmp_command[aux_size], command->num_nodes * sizeof(int));
        aux_size += command->num_nodes * sizeof(int);
    }
#endif

    //there is still data pending to read
    if (head.size > aux_size)
    {
        if (command->req == EAR_RC_SET_POWERCAP_OPT)
        {
            debug("read_command:received powercap_opt");
            memcpy(&command->my_req, &tmp_command[aux_size], sizeof(powercap_opt_t)); //first we copy the base powercap_opt_t
            //auxiliar variables
            size_t offset = command->my_req.pc_opt.num_greedy * sizeof(int);
            aux_size += sizeof(powercap_opt_t);

            //allocation
            command->my_req.pc_opt.greedy_nodes = calloc(command->my_req.pc_opt.num_greedy, sizeof(int));
            command->my_req.pc_opt.extra_power = calloc(command->my_req.pc_opt.num_greedy, sizeof(int));

            //copy
            memcpy(command->my_req.pc_opt.greedy_nodes, &tmp_command[aux_size], offset);
            memcpy(command->my_req.pc_opt.extra_power, &tmp_command[aux_size + offset], offset);
        }
        else
        {
            debug("read_command:recieved command with additional data: %d", command->req);
            debug("read_command:head.size %u internal_req_t size %lu current_size %lu", 
                                        head.size, sizeof(internal_request_t), aux_size); 
            memcpy(&command->my_req, &tmp_command[aux_size], head.size - aux_size);
        }
    }

    free(tmp_command);
    return command->req;
}

#else

int read_command(int s, request_t *command)
{
    request_header_t head;
    char *tmp_command;


    head = receive_data(s, (void **)&tmp_command);
    debug("read_command:received command type %d\t size: %u \t sizeof req: %lu", head.type, head.size, sizeof(request_t));

    if (head.type != EAR_TYPE_COMMAND || head.size < sizeof(request_t))
    {
        debug("read_command:NO_COMMAND");
        command->req = NO_COMMAND;
        if (head.size > 0) free(tmp_command);
        return EAR_SOCK_DISCONNECTED;
        return command->req;
    }
    memcpy(command, tmp_command, sizeof(request_t));

    if (head.size > sizeof(request_t))
    {
        if (command->req == EAR_RC_SET_POWERCAP_OPT)
        {
            debug("read_command:received powercap_opt");
            size_t offset = command->my_req.pc_opt.num_greedy * sizeof(int);
            command->my_req.pc_opt.greedy_nodes = calloc(command->my_req.pc_opt.num_greedy, sizeof(int));
            command->my_req.pc_opt.extra_power = calloc(command->my_req.pc_opt.num_greedy, sizeof(int));
            memcpy(command->my_req.pc_opt.greedy_nodes, &tmp_command[sizeof(request_t)], offset);
            memcpy(command->my_req.pc_opt.extra_power, &tmp_command[sizeof(request_t) + offset], offset);
        }
    }

    free(tmp_command);

    return command->req;
}
#endif

void send_answer(int s,long *ack)
{
    int ret;
    if ((ret = _write(s, ack, sizeof(ulong))) != sizeof(ulong)) error("Error sending the answer");
    if (ret < 0) error("(%s)", strerror(errno));
}

#if DYNAMIC_COMMANDS
size_t get_command_size(request_t *command, char **data_to_send)
{
    size_t size = sizeof(internal_request_t);
    size_t offset = 0;
    size_t aux_size = 0;
    char *command_b;

    switch(command->req)
    {
        case EAR_RC_SET_POWERCAP_OPT:
            size += sizeof(powercap_opt_t);
            size += command->my_req.pc_opt.num_greedy * sizeof(int) * 2;
            offset = command->my_req.pc_opt.num_greedy * sizeof(int);
            break;
        
        //NOTE: individual values are not being passed to keep the code simple. Instead, we measure the actual data size of the
        //union and send that instead. Ideally we would only copy the value that we need, but that would require a big re-write.

        case EAR_RC_MAX_FREQ:
        case EAR_RC_SET_FREQ:
        case EAR_RC_DEF_FREQ:
        case EAR_RC_INC_TH:
        case EAR_RC_RED_PSTATE:
        case EAR_RC_NEW_TH:
            size += sizeof(new_conf_t); //all this values use the union as ear_conf (a new_conf_t)
            break;
        case EAR_RC_SET_POWER:
        case EAR_RC_INC_POWER:
        case EAR_RC_RED_POWER:
            size += sizeof(power_limit_t); //powercap struct
            break;
        case EAR_RC_SET_RISK:
            size += sizeof(risk_dec_t); //risk struct
            break;
        case EAR_RC_SET_POLICY:
            size += sizeof(new_policy_cont_t); //new policy_conf
            break;
        case EAR_RC_NEW_JOB:
            size += sizeof(new_job_req_t);
            break;
        case EAR_RC_END_JOB:
            size += sizeof(end_job_req_t);
            break;
        case EARGM_NEW_JOB:
        case EARGM_END_JOB:
            size += sizeof(eargm_req_t);
            break;
    }
 
#if NODE_PROP
    if (command->num_nodes > 0)
    {
        size += command->num_nodes * sizeof(int);
        aux_size = command->num_nodes * sizeof(int);
    }
#endif

    //copy the original command
    command_b = calloc(1, size);
    memcpy(command_b, command, sizeof(internal_request_t)); //the first portion of request_t and internal_request_t align
#if NODE_PROP
    debug("get_command_size:copying nodes (%d num_nodes)", command->num_nodes);
    memcpy(&command_b[sizeof(internal_request_t)], command->nodes, aux_size); //copy the nodes to propagate to
#endif

    aux_size += sizeof(internal_request_t); //aux_size holds the position we have to start writing from

    switch(command->req)
    {
        case EAR_RC_SET_POWERCAP_OPT:
            //copy the base powercap_opt_t since we don't automatically copy anything beyond internal_request_t
            memcpy(&command_b[aux_size], &command->my_req, sizeof(powercap_opt_t)); 
            aux_size += sizeof(powercap_opt_t); //add the copied size to the index

            //proceed as with fixed size method, copying both arrays
            memcpy(&command_b[aux_size], command->my_req.pc_opt.greedy_nodes, offset);
            memcpy(&command_b[aux_size + offset], command->my_req.pc_opt.extra_power, offset);
            break;
        default:
            debug("get_command_size: copying additional data, total_size: %lu, already copied %lu", size, aux_size);
            memcpy(&command_b[aux_size], &command->my_req, size-aux_size); 
            break;
    }

    *data_to_send = command_b; 
    return size;

}
#else
size_t get_command_size(request_t *command, char **data_to_send)
{
    size_t size = sizeof(request_t);
    size_t offset = 0;
    char *command_b;


		#if POWERCAP
    switch(command->req)
    {
        case EAR_RC_SET_POWERCAP_OPT:
            size += command->my_req.pc_opt.num_greedy * sizeof(int) * 2;
            offset = command->my_req.pc_opt.num_greedy * sizeof(int);
            break;
    }
		#endif

    //copy the original command
    command_b = calloc(1, size);
    memcpy(command_b, command, sizeof(request_t));
		#if POWERCAP
    switch(command->req)
    {
        case EAR_RC_SET_POWERCAP_OPT:
            memcpy(&command_b[sizeof(request_t)], command->my_req.pc_opt.greedy_nodes, offset);
            memcpy(&command_b[sizeof(request_t) + offset], command->my_req.pc_opt.extra_power, offset);
            break;
    }
		#endif

    *data_to_send = command_b; 
    return size;
}
#endif

// Sends a command to eard
int send_command(request_t *command)
{
	ulong ack;
	int ret;
    size_t command_size;
    char *command_b;

	debug("send_command: sending command %u",command->req);

    command_size = get_command_size(command, &command_b);
    debug("send_command: command size: %lu\t request_t size: %lu", command_size, sizeof(request_t));
    
    if (send_non_block_data(eards_sfd, command_size, command_b, EAR_TYPE_COMMAND) != EAR_SUCCESS)
    {
        error("send_command: Error sending command");
        free(command_b);
        return 0;
    }

    free(command_b);
    debug("reading ack");

	ret = _read(eards_sfd, &ack, sizeof(ulong)); //read ack
	if (ret<0){
		error("send_command: Error receiving ack %s",strerror(errno));
	}
	else if (ret!=sizeof(ulong)) {
		debug("send_command: Error receiving ack: expected %lu ret %d",sizeof(ulong),ret);
	}
	return (ret==sizeof(ulong)); // Should we return ack ?
}

int send_non_block_data(int fd, size_t size, char *data, int type)
{
    int ret; 
	//int tries=0;
	//uint to_send,sent=0;
	//uint must_abort=0;

    /* Prepare header */
    request_header_t head;
    head.type = type;
    head.size = size;

    /* Send header, blocking */
    ret = _write(eards_sfd, &head, sizeof(request_header_t));

    if (ret < sizeof(request_header_t)) {
        error("send_non_block_data:error sending request_header in non_block command");
        return 0;
    }

    ret = _write(eards_sfd, data, size);
    if (ret < size) {
		debug("send_non_block_data: return non blocking command with EAR_ERROR");
		return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

int send_data(int fd, size_t size, char *data, int type)
{
    int ret;
    request_header_t head;
    head.size = size;
    head.type = type;

    debug("send_data: sending data of size %lu and type %d", size, type);
    debug("send_data: data sizes: %lu and %lu", sizeof(head.size), sizeof(head.type));
    ret = write(fd, &head, sizeof(request_header_t));
    if (ret < sizeof(request_header_t))
        return EAR_ERROR;
    debug("send_data: sent head, %d bytes", ret);

    ret = write(fd, data, size);
    if (ret < size)
        return EAR_ERROR;
    debug("send_data: sent data, %d bytes", ret);

    return EAR_SUCCESS; 

}

char is_valid_type(int type)
{
    if (type >= MIN_TYPE_VALUE && type <= MAX_TYPE_VALUE) return 1;
    return 0;
}


request_header_t receive_data(int fd, void **data)
{
    int ret;
    request_header_t head;
    char *read_data;
    head.type = 0;
    head.size = 0;

    /* Read header */
    ret = _read(fd, &head, sizeof(request_header_t));
    debug("receive_data: values read: type %d size %u", head.type, head.size);
    if (ret < 0) {
        error("receive_data: Error recieving response data header (%s) ", strerror(errno));
        head.type = EAR_ERROR;
        head.size = 0;
        return head;
    }

    /* Check header */
    if (head.size < 1 || !is_valid_type(head.type)) 
    {
        if (head.type != EAR_TYPE_APP_STATUS)
        {
            if (!((head.size == 0) && (head.type == 0))) {
                debug("receive_data: error recieving response data. Invalid data size (%d) or type (%d).", head.size, head.type);
            }
            head.type = EAR_ERROR;
            head.size = 0;
            return head;
        }
        else return head;
    }
    //write ack should go here if we implement it
    read_data = calloc(head.size, sizeof(char));

    /* Read the data indicated by the header */
    ret = _read(fd, read_data, head.size);
	if (ret < head.size) 
    {
		error("Error by receive data (%s)",strerror(errno));
        if (ret > 0)
            free(read_data);
        head.type = EAR_ERROR;
        head.size = 0;
		return head;
	}
    *data = read_data;

    debug("receive_data: returning from receive_data with type %d and size %u", head.type, head.size);
    return head;

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
    fd_set r_set, w_set;

    /*if (eards_remote_connected){ 
        debug("Connection already done!");
        return eards_sfd;
    }*/
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
            FD_ZERO(&w_set);
            FD_SET(sfd, &w_set);
            FD_ZERO(&r_set);
            FD_SET(sfd, &r_set);
            if (select(sfd+1, &r_set, &w_set, NULL, &timeout) > 0) 
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
                else {
                    debug("Connected"); 
                }
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

   	freeaddrinfo(result);           /* No longer needed */

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

	eards_remote_connected=1;
	eards_sfd=sfd;
	return sfd;

}

int eards_remote_disconnect()
{
	eards_remote_connected=0;
    debug("normal disconnect, closing fd %d\n", eards_sfd);
	close(eards_sfd);
	return EAR_SUCCESS;
}

int eards_remote_disconnect_fd(int sfd)
{
    debug("targeted disconnect, closing fd %d\n", sfd);
    close(sfd);
    return EAR_SUCCESS;
}

request_header_t correct_data_prop(int target_idx, int total_ips, int *ips, request_t *command, uint port, void **data)
{
    char *temp_data, *final_data = NULL;
    int rc, i, current_dist, off_ip, current_idx, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    request_header_t head;

    head.size = 0;
    head.type = EAR_ERROR;

    char next_ip[64];

    current_dist = target_idx - target_idx%NUM_PROPS;
    off_ip = target_idx%NUM_PROPS;

    debug("correct_data_prop:current idx: %d, total_ips: %d, current_dist: %d, offset: %d", 
                                                target_idx, total_ips, current_dist, off_ip);

    for ( i = 1; i <= NUM_PROPS; i++)
    {
        current_idx = current_dist*NUM_PROPS + i*NUM_PROPS + off_ip;
        //check that the next ip exists within the range
        if (current_idx >= total_ips) break;

        debug("correct_data_prop:contacting id %d in total_ips %d\n", current_idx, total_ips);
        //prepare next node data
        temp.sin_addr.s_addr = ips[current_idx];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        //prepare next node distance
        command->node_dist = current_idx - off_ip;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            debug("correct_data_prop:Error connecting to node: %s", next_ip);
            head = correct_data_prop(current_idx, total_ips, ips, command, port, (void **)&temp_data);
        }
        else
        {
            debug("correct_data_prop:connected with node: %s", next_ip);
            send_command(command);
            head = receive_data(rc, (void **)&temp_data);
            if (head.size < 1)
            {
                if (head.type == EAR_TYPE_APP_STATUS)
                    default_type = head.type;
                else
                    head.type = EAR_ERROR;
            }
            if (head.type == EAR_ERROR)
            {
                debug("propagate_req: Error propagating command to node %s", next_ip);
                eards_remote_disconnect();
                head = correct_data_prop(current_idx, total_ips, ips, command, port, (void **)&temp_data);
            }
            else eards_remote_disconnect();
        }

        if (head.size > 0 && head.type != EAR_ERROR)
        {
            head = process_data(head, (char **)&temp_data, (char **)&final_data, final_size);
            free(temp_data);
            final_size = head.size;
            default_type = head.type;
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
    else if (final_size == 0 && (default_type != EAR_TYPE_APP_STATUS || default_type != EAR_ERROR)) head.type = EAR_ERROR;


    return head;

}

void correct_error(int target_idx, int total_ips, int *ips, request_t *command, uint port)
{
    if (command->node_dist > total_ips) return;
    struct sockaddr_in temp;

    int i, rc, current_dist, off_ip, current_idx;
    char next_ip[50]; 

    current_dist = target_idx - target_idx%NUM_PROPS;
    off_ip = target_idx%NUM_PROPS;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        current_idx = current_dist*NUM_PROPS + i*NUM_PROPS + off_ip;
        //check that the next ip exists within the range
        if (current_idx >= total_ips) break;

        //prepare next node data
        temp.sin_addr.s_addr = ips[current_idx];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));

        //prepare next node distance
        command->node_dist = current_idx - off_ip;

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            debug("correct_error:Error connecting to node: %s", next_ip);
            correct_error(current_idx, total_ips, ips, command, port);
        }
        else
        {
            if (!send_command(command)) 
            {
                debug("correct_error: Error propagating command to node %s", next_ip);
                eards_remote_disconnect();
                correct_error(current_idx, total_ips, ips, command, port);
            }
            else eards_remote_disconnect();
        }
    }
}

#if NODE_PROP
void correct_error_nodes(request_t *command, int self_ip, uint port)
{
     
    request_t tmp_command;
    int i;

    if (command->num_nodes < 1) return;
    else
    {   //if there is only one node and it is the current one nothing needs to be done
        if (command->num_nodes == 1 && command->nodes[0] == self_ip) return;
        //if not, we remove the current node from the list
        for (i = 0; i < command->num_nodes; i++)
        {
            if (command->nodes[i] == self_ip)
            {
                command->num_nodes --;
                command->nodes[i] = command->nodes[command->num_nodes];
                command->nodes = realloc(command->nodes, sizeof(int) * command->num_nodes);
            }
        }
    }

    memcpy(&tmp_command, command, sizeof(request_t));
    int base_distance = command->num_nodes/NUM_PROPS;
    if (base_distance < 1) base_distance = 1; //if there is only 1 or 2 nodes

    internal_send_command_nodes(command, port, base_distance, NUM_PROPS);

    // we set num_nodes to 0 to prevent unknown errors, but memory is freed in request_propagation
    command->num_nodes = 0;
    
}

request_header_t correct_data_prop_nodes(request_t *command, int self_ip, uint port, void **data)
{
    request_t tmp_command;
    request_header_t head;
    char *temp_data, *final_data = NULL;
    int rc, i, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    char next_ip[64];

    head.size = 0;
    head.type = 0;

    if (command->num_nodes < 1) return head;
    else
    {   //if there is only one node and it is the current one nothing needs to be done
        if (command->num_nodes == 1 && command->nodes[0] == self_ip) return head;
        //if not, we remove the current node from the list
        for (i = 0; i < command->num_nodes; i++)
        {
            if (command->nodes[i] == self_ip)
            {
                command->num_nodes --;
                command->nodes[i] = command->nodes[command->num_nodes];
                command->nodes = realloc(command->nodes, sizeof(int) * command->num_nodes);
            }
        }
    }

    memcpy(&tmp_command, command, sizeof(request_t));
    int base_distance = command->num_nodes/NUM_PROPS;
    if (base_distance < 1) base_distance = 1; //if there is only 1 or 2 nodes
    for (i = 0; i < NUM_PROPS; i++)
    {
        if (base_distance * i >= command->num_nodes) break;
        //prepare next node data
        temp.sin_addr.s_addr = command->nodes[base_distance * i];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));

        // node distance is not needed
        // we calculate how many ips will be sent to this node by getting the max possible ip index and subtracting the starting one
        int max_ips = (base_distance * (1 + i) < command->num_nodes) ? base_distance * (1 + i) : command->num_nodes;
        max_ips -= base_distance * i;

        tmp_command.nodes = calloc(max_ips, sizeof(int));
        tmp_command.num_nodes = max_ips;

        memcpy(tmp_command.nodes, &command->nodes[base_distance*i], max_ips*sizeof(int));

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("correct_data_prop_nodes: Error connecting to node: %s\n", next_ip);
            head = correct_data_prop_nodes(&tmp_command, self_ip, port, (void **)&temp_data);
        }
        else
        {
            send_command(&tmp_command);
            head = receive_data(rc, (void **)&temp_data);
            if ((head.size) < 1 || head.type == EAR_ERROR)
            {
                error("correct_data_prop_nodes: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                head = correct_data_prop_nodes(&tmp_command, self_ip, port, (void **)&temp_data);
            }
            else eards_remote_disconnect();
        }

        if (head.size > 0 && head.type != EAR_ERROR)
        {
            default_type = head.type;
            final_data = realloc(final_data, final_size + head.size);
            memcpy(&final_data[final_size], temp_data, head.size);
            final_size += head.size;
            free(temp_data);
        }

        free(tmp_command.nodes);
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

    // we set num_nodes to 0 to prevent unknown errors, but memory is freed in request_propagation
    command->num_nodes = 0;
    return head;

}

#endif

#if NODE_PROP

#define MAX_PROP_DEPTH 3
/* Calculates the "ideal" amount of nodes a node will comunicate to. See send_command_nodes for
 * how the comunication works. We calculate it taking into account the total number of nodes 
 * the message will be send to, the number of propagations a node will do and the maximum propagation
 * depth we want to achieve (how many propagations can it take to get to leaf nodes). */
int get_max_prop_group(int num_props, int max_depth, int num_nodes)
{

    int max_nodes;
    /* If we the standard number of propagations is higher than the number of nodes, we
     * set the distance to 1 so every node is contacted via the initial communication */
    if (num_nodes < num_props) return 1;
    max_nodes = powl(num_props, max_depth);

    /* Once we have the max number of nodes one initial communication can propagate to, the idea is
     * to optimise the number of initial communications. For now we optimise the lower bound, to guarantee
     * that the initial communication will, at the very least communicate with num_props nodes. */
    while (max_nodes > num_nodes / num_props)
    {
        max_nodes /= num_props;
    }

    /* The following is an idea to guarantee that each initial communication propagates to at least 
     * a certain % of nodes. The example is for 10%. Not implemented yet, certain cases would have to
     * be explored: the % makes a max_nodes > num_nodes / num_props; the % makes max_nodes = num_props. */
    /*while (max_nodes < num_nodes * 0.10)
    {
        max_nodes *= num_props;
    }*/

    if (max_nodes > num_props) return max_nodes;

    return num_props;

}

void internal_send_command_nodes(request_t *command, int port, int base_distance, int num_sends)
{
    int i, rc;
    struct sockaddr_in temp;
    char next_ip[256];
    request_t tmp_command;

    memcpy(&tmp_command, command, sizeof(request_t));
		#if SHOW_DEBUGS
		for (i = 0; i < command->num_nodes;i++){
			temp.sin_addr.s_addr = command->nodes[i];
			debug("Node %i : %s",i,inet_ntoa(temp.sin_addr));
		}
		#endif

    for (i = 0; i < num_sends; i++)
    {

        // if no more nodes to propagate to
        debug("internal_send_command_nodes: base distance %d i %d num_nodes %d", base_distance, i, command->num_nodes);
        if (base_distance * i >= command->num_nodes) break;

        temp.sin_addr.s_addr = command->nodes[i*base_distance];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));

        // node distance is not needed
        int max_ips = (base_distance * (1 + i) < command->num_nodes) ? base_distance * (1 + i) : command->num_nodes;
        max_ips -= base_distance * i;

        tmp_command.nodes = calloc(max_ips, sizeof(int));
        tmp_command.num_nodes = max_ips;

        memcpy(tmp_command.nodes, &command->nodes[base_distance*i], max_ips*sizeof(int));

        rc=eards_remote_connect(next_ip, port);
        if (rc<0){
            debug("internal_send_command_nodes: Error connecting with node %s, trying to correct it", next_ip);
            correct_error_nodes(&tmp_command, command->nodes[base_distance*i], port);
        }
        else{
            debug("internal_send_command_nodes: Node %s with %d num_nodes contacted!", next_ip, tmp_command.num_nodes);
            if (!send_command(&tmp_command)) {
                debug("internal_send_command_nodes: Error sending command to node %s, trying to correct it", next_ip);
                eards_remote_disconnect();
                correct_error_nodes(&tmp_command, command->nodes[i*base_distance], port);
            }
            else eards_remote_disconnect();
        }
        free(tmp_command.nodes);
        tmp_command.num_nodes = 0;
    }
}

void send_command_nodelist(request_t *command, cluster_conf_t *my_cluster_conf)
{
    int base_distance, num_sends;

    time_t ctime = time(NULL);
    command->time_code = ctime;

    /* We get which nodes this function will comunicate with by sending a message
     * every _base_distance_ nodes, and sending to that node the list of nodes between 
     * themselves and the next node. IE: if we have nodes A B C D E F G and we get a 
     * base_distance of 3, we will send the message to A, D and G. To A we will send A B C,
     * D will receive D E F, and G will only receive themselves. */
    base_distance = get_max_prop_group(NUM_PROPS, MAX_PROP_DEPTH, command->num_nodes);
    num_sends = command->num_nodes / base_distance; //number of nodes this function will comunicate with
		if (command->num_nodes % base_distance) num_sends++;
    debug("sending command with %d nodes, %d base_distance and %d num_sends", command->num_nodes, base_distance, num_sends);

    /* Inner functionality for initial communication has been moved to its own function to avoid code replication */
    internal_send_command_nodes(command, my_cluster_conf->eard.port, base_distance, num_sends);
}

request_header_t internal_data_nodes(request_t *command, int port, int base_distance, int num_sends, void **data)
{
    request_t tmp_command;
    request_header_t head;
    char *temp_data, *final_data = NULL;
    int rc, i, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    char next_ip[64];

    head.size = 0;
    head.type = 0;

    if (command->num_nodes < 1) return head;

    memcpy(&tmp_command, command, sizeof(request_t));
    if (base_distance < 1) base_distance = 1; //if there is only 1 or 2 nodes
    for (i = 0; i < NUM_PROPS; i++)
    {
        if (base_distance * i >= command->num_nodes) break;
        //prepare next node data
        temp.sin_addr.s_addr = command->nodes[base_distance * i];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));

        // node distance is not needed
        // we calculate how many ips will be sent to this node by getting the max possible ip index and subtracting the starting one
        int max_ips = (base_distance * (1 + i) < command->num_nodes) ? base_distance * (1 + i) : command->num_nodes;
        max_ips -= base_distance * i;

        tmp_command.nodes = calloc(max_ips, sizeof(int));
        tmp_command.num_nodes = max_ips;

        memcpy(tmp_command.nodes, &command->nodes[base_distance*i], max_ips*sizeof(int));

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("correct_data_prop_nodes: Error connecting to node: %s\n", next_ip);
            head = correct_data_prop_nodes(&tmp_command, command->nodes[base_distance * i], port, (void **)&temp_data);
        }
        else
        {
            send_command(&tmp_command);
            head = receive_data(rc, (void **)&temp_data);
            if ((head.size) < 1 || head.type == EAR_ERROR)
            {
                error("correct_data_prop_nodes: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                head = correct_data_prop_nodes(&tmp_command, command->nodes[base_distance*i], port, (void **)&temp_data);
            }
            else eards_remote_disconnect();
        }

        if (head.size > 0 && head.type != EAR_ERROR)
        {
            default_type = head.type;
            final_data = realloc(final_data, final_size + head.size);
            memcpy(&final_data[final_size], temp_data, head.size);
            final_size += head.size;
            free(temp_data);
        }

        free(tmp_command.nodes);
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

    // we set num_nodes to 0 to prevent unknown errors, but memory is freed in request_propagation
    command->num_nodes = 0;
    return head;


}

request_header_t data_nodelist(request_t *command, cluster_conf_t *my_cluster_conf, void **data)
{
    int base_distance, num_sends;
    request_header_t head;


    time_t ctime = time(NULL);
    command->time_code = ctime;

    /* We get which nodes this function will comunicate with by sending a message
     * every _base_distance_ nodes, and sending to that node the list of nodes between 
     * themselves and the next node. IE: if we have nodes A B C D E F G and we get a 
     * base_distance of 3, we will send the message to A, D and G. To A we will send A B C,
     * D will receive D E F, and G will only receive themselves. */
    base_distance = get_max_prop_group(NUM_PROPS, MAX_PROP_DEPTH, command->num_nodes);
    num_sends = command->num_nodes / base_distance; //number of nodes this function will comunicate with
    debug("sending command with %d nodes, %d base_distance and %d num_sends", command->num_nodes, base_distance, num_sends);

    /* Inner functionality for initial communication has been moved to its own function to avoid code replication */
    head = internal_data_nodes(command, my_cluster_conf->eard.port, base_distance, num_sends, data);
    return head;
}
#endif

void send_command_all(request_t command, cluster_conf_t *my_cluster_conf)
{
    int i, j,  rc, total_ranges;
    int **ips, *ip_counts;
    struct sockaddr_in temp;
    char next_ip[256];

    time_t ctime = time(NULL);
    command.time_code = ctime;

    total_ranges = get_ip_ranges(my_cluster_conf, &ip_counts, &ips);
    for (i = 0; i < total_ranges; i++)
    {
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++)
        {
            command.node_dist = 0;
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(next_ip, inet_ntoa(temp.sin_addr));
            
            rc=eards_remote_connect(next_ip, my_cluster_conf->eard.port);
            if (rc<0){
                debug("Error connecting with node %s, trying to correct it", next_ip);
                correct_error(j, ip_counts[i], ips[i], &command, my_cluster_conf->eard.port);
            }
            else{
                debug("Node %s with distance %d contacted!", next_ip, command.node_dist);
                if (!send_command(&command)) {
                    debug("Error sending command to node %s, trying to correct it", next_ip);
                    eards_remote_disconnect();
                    correct_error(j, ip_counts[i], ips[i], &command, my_cluster_conf->eard.port);
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
    int i, j, rc, total_ranges, final_size = 0, default_type = EAR_ERROR, num_sfds = 0, offset = 0;
    int **ips, *ip_counts, *sfds;
    struct sockaddr_in temp;
    request_header_t head;
    char *temp_data, *all_data = NULL;
    char next_ip[64];
    
    total_ranges = get_ip_ranges(my_cluster_conf, &ip_counts, &ips);

    for (i = 0; i < total_ranges; i++)
        num_sfds += (ip_counts[i] < NUM_PROPS) ? ip_counts[i] : NUM_PROPS;

    sfds = calloc(num_sfds, sizeof(int));

    for (i = 0; i < total_ranges; i++)
    {
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++, offset++)
        {
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(next_ip, inet_ntoa(temp.sin_addr));
            
            rc=eards_remote_connect(next_ip, my_cluster_conf->eard.port);
            if (rc<0){
//               debug("data_all_nodes: Error connecting with node %s, trying to correct it", next_ip);
//               debug("data_all_nodes: (node %s was %d in the current list of ips", next_ip, j);
                head = correct_data_prop(j, ip_counts[i], ips[i], command, my_cluster_conf->eard.port, (void **)&temp_data);

                if (head.size > 0 && head.type != EAR_ERROR)
                {
                    head = process_data(head, (char **)&temp_data, (char **)&all_data, final_size);
                    free(temp_data);
                    final_size = head.size;
                    default_type = head.type;
                }
            }
            else{
                debug("Node %s with distance %d contacted (fd %d)!", next_ip, command->node_dist, rc);
                sfds[offset] = rc;
                send_command(command);
            }
        }
    }

    debug("Finished sending to eards, now reading data\n\n");

    // reset offset to iterate through all the fds again
    offset = 0;
    for (i = 0; i < total_ranges; i++)
    {
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++, offset++)
        {
            if (sfds[offset] == 0) continue; // if the connection had failed we should have already corrected that in the previous loop 

            head = receive_data(sfds[offset], (void **)&temp_data);
            debug("received data from fd %d contacted with type %d and size %d", sfds[offset], head.type, head.size);
            if (head.size < 1) 
            {
                if (head.type == EAR_TYPE_APP_STATUS)
                    default_type = head.type;
                else
                    head.type = EAR_ERROR;
            }
            if (head.type == EAR_ERROR)
            {
                debug("data_all_nodes:Error reading data from node %d in the list, trying to correct it", j);
                eards_remote_disconnect_fd(sfds[offset]);
                head = correct_data_prop(j, ip_counts[i], ips[i], command, my_cluster_conf->eard.port, (void **)&temp_data);
            }
            else eards_remote_disconnect_fd(sfds[offset]);
        
            if (head.size > 0 && head.type != EAR_ERROR)
            {
                head = process_data(head, (char **)&temp_data, (char **)&all_data, final_size);
                free(temp_data);
                final_size = head.size;
                default_type = head.type;
            }
            
        }
    }
    debug("exiting data_all_nodes with %d num_sfds and %d offset\n", num_sfds, offset);
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
        free(sfds);
    }

    return head;
}

powercap_status_t *mem_alloc_powercap_status(char *final_data)
{
    int final_size;
    powercap_status_t *status = calloc(1, sizeof(powercap_status_t));

    final_size = sizeof(powercap_status_t);
    memcpy(status, final_data, final_size);

    status->greedy_nodes = calloc(status->num_greedy, sizeof(int));
    status->greedy_data  = calloc(status->num_greedy, sizeof(greedy_bytes_t));

    memcpy(status->greedy_nodes, &final_data[final_size], sizeof(int)*status->num_greedy);
    final_size += status->num_greedy*sizeof(int);

    memcpy(status->greedy_data, &final_data[final_size], sizeof(greedy_bytes_t)*status->num_greedy);
    final_size += status->num_greedy*sizeof(greedy_bytes_t);

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
    
    memcpy(&data[size], status->greedy_data, sizeof(greedy_bytes_t) * status->num_greedy);
    size += sizeof(greedy_bytes_t) * status->num_greedy;

    return data;
}

powercap_status_t *memmap_powercap_status(char *final_data, int *size)
{
    int final_size;
    powercap_status_t *status = (powercap_status_t *) final_data;

    final_size = sizeof(powercap_status_t);
    status->greedy_nodes = (int *)&final_data[final_size];
    final_size += status->num_greedy*sizeof(int);
               
    status->greedy_data = (greedy_bytes_t *)&final_data[final_size];
    final_size += status->num_greedy*sizeof(greedy_bytes_t);

    *size = final_size;
    return status;

}

request_header_t process_data(request_header_t data_head, char **temp_data_ptr, char **final_data_ptr, int final_size)
{
    char *temp_data = *temp_data_ptr;
    char *final_data = *final_data_ptr;
    request_header_t head;
    head.type = data_head.type;
    switch(data_head.type)
    {
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

                memcpy(&final_status[final_size], original_status->greedy_data, sizeof(greedy_bytes_t)*original_status->num_greedy);
                final_size += sizeof(greedy_bytes_t)*original_status->num_greedy;
                memcpy(&final_status[final_size], new_status->greedy_data, sizeof(greedy_bytes_t)*new_status->num_greedy);
                final_size += sizeof(greedy_bytes_t)*new_status->num_greedy;

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
        default:
            final_data = realloc(final_data, final_size + data_head.size);
            memcpy(&final_data[final_size], temp_data, data_head.size);
            head.size = final_size + data_head.size;
        break;
    }

    *final_data_ptr = final_data;

    return head;
}

