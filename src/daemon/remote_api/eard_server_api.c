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

#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/limits.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>
#include <common/output/verbose.h>
#include <common/messaging/msg_conf.h>

#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_server_api.h>


int *ips = NULL;
int *temp_ips = NULL;
int total_ips = -1;
int self_id = -1;
int self_ip = -1;
int init_ips_ready=0;


int init_ips(cluster_conf_t *my_conf)
{
    int ret, i;
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
    self_ip = get_ip(buff, my_conf);
    for (i = 0; i < ret; i++)
    {
        if (ips[i] == self_ip)
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
    return EAR_SUCCESS;
  
}

void close_ips()
{
    if (total_ips > 0)
    {
        free(ips);
        total_ips = -1;
        self_id = -1;
        self_ip = -1;
    }
    else
    {
        warning("IPS were not initialised.\n");
    }
}

#if NODE_PROP
char can_propagate(request_t *command)
{

    // if there is less than 2 nodes (one could mean there is our own ip) we don't need to propagate
    // but we still check when there's one just in case we don't receive our own ip and the one
    // received has to be propagated to
    if (command->num_nodes < 1)
    {
        // the following are separate because they come from different ideas
        // the first is that the node does not need to propagate, while the 
        // second is that it cannot propagate due to lack of an IP list
        if (command->node_dist > total_ips) return 0;
        if (self_ip == -1 || ips == NULL || total_ips < 1) return 0;
    }
    //if there is only one node and it is the current one nothing needs to be done
    else if (command->num_nodes == 1 && command->nodes[0] == self_ip) 
    {
        debug("can_propagate: only self node in the list to propagate, returning");
        free(command->nodes); //since nodes will not be freed in propagate we free here 
        return 0;
    }

    return 1;
}
#else
char can_propagate(request_t *command)
{
    // the following are separate because they come from different ideas
    // the first is that the node does not need to propagate, while the 
    // second is that it cannot propagate due to lack of an IP list
    if (command->node_dist > total_ips) return 0;
    if (self_ip == -1 || ips == NULL || total_ips < 1) return 0;

    return 1;
}
#endif

#if NODE_PROP
void propagate_req(request_t *command, uint port)
{
     
    struct sockaddr_in temp;

    int i, rc, off_ip, current_dist, current_idx;
    char next_ip[50]; 

    //if the command can't be propagated we return early
    if (!can_propagate(command)) return;
   
    //if we are propagating via specific nodes we need to remove ours from the list
    if (command->num_nodes > 0)
    {
        debug("propagate_req with num_nodes > 0 (%d)", command->num_nodes);
        for (i = 0; i < command->num_nodes; i++)
        {
            debug("propagate_req: node%d %d (self_ip=%d)", i, command->nodes[i], self_ip);
            if (command->nodes[i] == self_ip)
            {
                command->num_nodes --;
                command->nodes[i] = command->nodes[command->num_nodes];
                command->nodes = realloc(command->nodes, sizeof(int) * command->num_nodes);
            }
        }
    }

    current_dist = self_id - self_id%NUM_PROPS;
    off_ip = self_id%NUM_PROPS;

    if (command->num_nodes < 1)
    {
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
                error("propagate_req:Error connecting to node: %s\n", next_ip);
                correct_error(current_idx, total_ips, ips, command, port);
            }
            else
            {
                if (!send_command(command)) 
                {
                    error("propagate_req: Error propagating command to node %s\n", next_ip);
                    eards_remote_disconnect();
                    correct_error(current_idx, total_ips, ips, command, port);
                }
                else eards_remote_disconnect();
            }
        }
    }
    else
    {
        int base_distance = command->num_nodes/NUM_PROPS;
        if (base_distance < 1) base_distance = 1; //in case there are less nodes than NUM_PROPS

        internal_send_command_nodes(command, port, base_distance, NUM_PROPS);

        // nodes are allocated on read_command and freed here since they won't be needed anymore
        // we set num_nodes to 0 to prevent unknown errors
        command->num_nodes = 0;
        free(command->nodes);
    } 

    
}
#else
void propagate_req(request_t *command, uint port)
{
     
    struct sockaddr_in temp;

    int i, rc, off_ip, current_dist, current_idx;
    char next_ip[50]; 

    //if the command can't be propagated we return early
    if (!can_propagate(command)) return;

    current_dist = self_id - self_id%NUM_PROPS;
    off_ip = self_id%NUM_PROPS;

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
            error("propagate_req:Error connecting to node: %s\n", next_ip);
            correct_error(current_idx, total_ips, ips, command, port);
        }
        else
        {
            if (!send_command(command)) 
            {
                error("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                correct_error(current_idx, total_ips, ips, command, port);
            }
            else eards_remote_disconnect();
        }
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

            return saddr->sin_addr.s_addr;
        }
    }
	return EAR_ERROR;
}

#if NODE_PROP
request_header_t propagate_data(request_t *command, uint port, void **data)
{
    char *temp_data, *final_data = NULL;
    int rc, i, current_dist, off_ip, current_idx, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    request_header_t head;
    char next_ip[64];

    current_dist = self_id - self_id%NUM_PROPS;
    off_ip = self_id%NUM_PROPS;

    if (command->num_nodes < 1)
    {
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
            debug("Propagating to %s with distance %d\n", next_ip, command->node_dist);

            //connect and send data
            rc = eards_remote_connect(next_ip, port);
            if (rc < 0)
            {
                error("propagate_data: Error connecting to node: %s\n", next_ip);
                head = correct_data_prop(current_idx, total_ips, ips, command, port, (void **)&temp_data);
            }
            else
            {
                send_command(command);
                head = receive_data(rc, (void **)&temp_data);
                if ((head.size < 1 && head.type != EAR_TYPE_APP_STATUS) || head.type == EAR_ERROR) 
                {
                    error("propagate_data: Error propagating command to node %s\n", next_ip);
                    eards_remote_disconnect();
                    head = correct_data_prop(current_idx, total_ips, ips, command, port, (void **)&temp_data);
                }
                else eards_remote_disconnect();
            }

            if (head.size > 0 && head.type != EAR_ERROR)
            {
                head = process_data(head, &temp_data, &final_data, final_size);
                free(temp_data);
                default_type = head.type;
                final_size = head.size;
            }
        }
    }
    else
    {
        request_t tmp_command;
        memcpy(&tmp_command, command, sizeof(request_t));
        int base_distance = command->num_nodes/NUM_PROPS;
        if (base_distance < 1) base_distance = 1; //in case there are only 2 nodes
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
                error("propagate_data: Error connecting to node: %s\n", next_ip);
                head = correct_data_prop_nodes(&tmp_command, self_ip, port, (void **)&temp_data);
            }
            else
            {
                send_command(&tmp_command);
                head = receive_data(rc, (void **)&temp_data);
                if ((head.size < 1 && head.type != EAR_TYPE_APP_STATUS) || head.type == EAR_ERROR) 
                {
                    error("propagate_data: Error propagating command to node %s\n", next_ip);
                    eards_remote_disconnect();
                    head = correct_data_prop_nodes(&tmp_command, self_ip, port, (void **)&temp_data);
                }
                else eards_remote_disconnect();
            }

            if (head.type == EAR_TYPE_APP_STATUS)
                default_type = head.type;
            if (head.size > 0  && head.type != EAR_ERROR)
            {
                head = process_data(head, &temp_data, &final_data, final_size);
                free(temp_data);
                default_type = head.type;
                final_size = head.size;
            }

            free(tmp_command.nodes);

        }

        // nodes are allocated on read_command and freed here since they won't be needed anymore
        // we set num_nodes to 0 to prevent unknown errors
        command->num_nodes = 0;
        free(command->nodes);
    }

    head.size = final_size;
    head.type = default_type;
    *data = final_data;

    if (default_type == EAR_ERROR && final_size > 0)
    {
        free(final_data);
        head.size = 0;
    }
    else if (final_size < 1 && (default_type != EAR_TYPE_APP_STATUS || default_type != EAR_ERROR)) head.type = EAR_ERROR;

    return head;
	
}
#else
request_header_t propagate_data(request_t *command, uint port, void **data)
{
    char *temp_data, *final_data = NULL;
    int rc, i, current_dist, off_ip, current_idx, final_size = 0, default_type = EAR_ERROR;
    struct sockaddr_in temp;
    request_header_t head;
    char next_ip[64];

    current_dist = self_id - self_id%NUM_PROPS;
    off_ip = self_id%NUM_PROPS;
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
        verbose(VCONNECT+2, "Propagating to %s with distance %d\n", next_ip, command->node_dist);

        //connect and send data
        rc = eards_remote_connect(next_ip, port);
        if (rc < 0)
        {
            error("propagate_req: Error connecting to node: %s\n", next_ip);
            head = correct_data_prop(current_idx, total_ips, ips, command, port, (void **)&temp_data);
        }
        else
        {
            send_command(command);
            head = receive_data(rc, (void **)&temp_data);
            if ((head.size < 1 && head.type != EAR_TYPE_APP_STATUS) || head.type == EAR_ERROR) 
            {
                error("propagate_req: Error propagating command to node %s\n", next_ip);
                eards_remote_disconnect();
                head = correct_data_prop(current_idx, total_ips, ips, command, port, (void **)&temp_data);
            }
            else eards_remote_disconnect();
        }

        if (head.type == EAR_TYPE_APP_STATUS)
            default_type = head.type;

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
    else if (final_size < 1 && (default_type != EAR_TYPE_APP_STATUS || default_type != EAR_ERROR)) head.type = EAR_ERROR;

    return head;
	
}
#endif

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

int propagate_and_cat_data(request_t *command, uint port, void **status, size_t size,uint type)
{
    char *temp_status, *final_status;
    int *ip;
    int num_status = 0;

    // if the current node is a leaf node (either last node or ip_init had failed)
    // we set the status for this node and return
    if (command->node_dist > total_ips || self_id < 0 || ips == NULL || total_ips < 1)
    {
        final_status = (char *)calloc(1, size);
        ip=(int *)final_status;
        if (self_id < 0 || ips == NULL)
            *ip = get_self_ip();
        else
            *ip = ips[self_id];
	    	debug("generic_status has 1 status\n");
        *status = final_status;
        return 1;
    }

    // propagate request and receive the data
    request_header_t head = propagate_data(command, port, (void **)&temp_status);
    num_status = head.size / size;
    
    if (head.type != type || head.size < size)
    {
        final_status = (char *)calloc(1, size);
        ip=(int *)final_status;
        *ip = ips[self_id];
        if (head.size > 0) free(temp_status);
        *status = final_status;
        return 1;
    }

    //memory allocation with the current node
    final_status = (char *)calloc(num_status + 1, size);
    memcpy(final_status, temp_status, head.size);

    //current node info
    ip = (int *)&final_status[num_status*size];
    *ip = ips[self_id];
    *status = final_status;
    num_status++;   //we add the original status

    free(temp_status);

    return num_status;

}

int propagate_status(request_t *command, uint port, status_t **status)
{
    int num_status;
    status_t *temp_status;
    num_status = propagate_and_cat_data(command, port, (void **)&temp_status, sizeof(status_t), EAR_TYPE_STATUS);
    temp_status[num_status-1].ok = STATUS_OK;
    *status=temp_status;
    return num_status;
}

int propagate_app_status(request_t *command, uint port, app_status_t **status)
{
    int num_status;
    app_status_t *temp_status;
    num_status = propagate_and_cat_data(command, port, (void **)&temp_status, sizeof(app_status_t), EAR_TYPE_APP_STATUS);
    *status=temp_status;
    return num_status;
}


