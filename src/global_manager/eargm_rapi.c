/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <common/states.h>
#include <common/output/verbose.h>
#include <common/messaging/msg_internals.h>

#include <global_manager/eargm_rapi.h>

// Verbosity
static int eargm_sfd;
static uint eargm_remote_connected=0;

int eargm_get_status(cluster_conf_t *conf, eargm_status_t **status, char **hosts, int num_hosts)
{
	if (hosts == NULL || num_hosts < 1) return eargm_cluster_get_status(conf, status);
	return eargm_nodelist_get_status(conf, status, hosts, num_hosts);

}

void eargm_set_powerlimit(cluster_conf_t *conf, uint limit, char **hosts, int num_hosts)
{
	if (hosts == NULL || num_hosts < 1) return eargm_cluster_set_powerlimit(conf, limit);
	return eargm_nodelist_set_powerlimit(conf, limit, hosts, num_hosts);
}

void eargm_nodelist_set_powerlimit(cluster_conf_t *conf, uint limit, char **hosts, int num_hosts)
{
	if (hosts == NULL || num_hosts < 1) return;

	int32_t i, j, rc, port;
    char tmp[1024] = { 0 };
    char *hostname = NULL;
    char *port_str = NULL;

	for (i = 0; i < num_hosts; i++) {
        //copy the host string to preserve it
        strncpy(tmp, hosts[i], 1023);
		port = -1;
        //hostname will always be the start
        hostname = tmp;
        port_str = NULL;
        //port_str will point to the ':' char if it exists, or will be NULL if it does not
        port_str = strchr(tmp, ':');

        //if ':' is not present, we search for the port in the eargm list
        if (port_str == NULL) {
            for (j = 0; j < conf->eargm.num_eargms; j++) {
                if (!strcasecmp(hosts[i], conf->eargm.eargms[j].node)) {
                    port = conf->eargm.eargms[j].port;
                    break;
                }
            }
        } else {
            //place a '\0' instead of ':' so the remote_connect still works
            *port_str = '\0';
            //advance the pointer to the first number of the port and parse it
            port_str++;
            port = atoi(port_str);
        }

        //this can only occur if a port was not specified and the eargm is not on the eargm list
        if (port < 0) {
            warning("node %s not found in the configuration file, skipping", hosts[i]);
            continue;
        }
        rc = remote_connect(hostname, port);
        if (rc < 0) {
            error("Error connecting to %s:%u", hosts[i], port);
            continue;
        }
        if (!eargm_send_set_powerlimit(limit)) {
            error("Error sending powerlimit to %s", conf->eargm.eargms[i].node);
        }
       remote_disconnect();

	}

}

void eargm_cluster_set_powerlimit(cluster_conf_t *conf, uint limit)
{
	int i, rc;
	
	for (i = 0; i < conf->eargm.num_eargms; i++) {
        rc = remote_connect(conf->eargm.eargms[i].node, conf->eargm.eargms[i].port);
        if (rc < 0) {
            error("Error connecting to %s:%u", conf->eargm.eargms[i].node, conf->eargm.eargms[i].port);
            continue;
        }
		if (!eargm_send_set_powerlimit(limit)) {
			error("Error sending powerlimit to %s", conf->eargm.eargms[i].node);
		}
        remote_disconnect();

	}
}


//get all eargm status under a nodelist
int eargm_nodelist_get_status(cluster_conf_t *conf, eargm_status_t **status, char **hosts, int num_hosts)
{

	if (hosts == NULL || num_hosts < 1) return 0;

    int i, j, rc, ret, port, num_status = 0;
    eargm_status_t *aux_status, *final_status = NULL;
    char *tmp_eargm = NULL;
    debug("entering eargm_nodelist_get_all_status");
    for (i = 0; i < num_hosts; i++) //simply iterate through all the eargms
    {
		tmp_eargm = hosts[i]; //to reuse the previous code
		port = -1;
		for (j = 0; j < conf->eargm.num_eargms; j++) {
           if (!strcasecmp(tmp_eargm, conf->eargm.eargms[j].node)) {
                port = conf->eargm.eargms[j].port;
				break;
                //debug("Found node on %s on port %u", tmp_eargm, port);
            }
		}
        //debug("contacting node %d", i);
        if (port < 0) {
            debug("node %s not found, skipping", tmp_eargm);
            continue;
        }
        rc = remote_connect(tmp_eargm, port);
        if (rc < 0) {
            debug("Error connecting to %s:%u", tmp_eargm, port);
            continue;
        }

        ret = eargm_status(rc, &aux_status);
        debug("Received %d eargm status", ret);
        if (ret > 0)
        {
            num_status += ret;
            final_status = realloc(final_status, sizeof(eargm_status_t)*num_status);
            memcpy(&final_status[num_status-1], aux_status, ret*sizeof(eargm_status_t));
            free(aux_status);
        }
        tmp_eargm = NULL; //reset the var
        remote_disconnect();
    }

    *status = final_status;

    return num_status;
}

//get all the eargm status in a cluster
int eargm_cluster_get_status(cluster_conf_t *conf, eargm_status_t **status)
{

    int i, rc, ret, port, num_status = 0;
    eargm_status_t *aux_status, *final_status = NULL;
    char *tmp_eargm = NULL;
    debug("entering eargm_cluster_get_all_status");
    for (i = 0; i < conf->eargm.num_eargms; i++) //simply iterate through all the eargms
    {
		tmp_eargm = conf->eargm.eargms[i].node; //to reuse the previous code
		port = conf->eargm.eargms[i].port;
        //debug("contacting node %d", i);
        if (tmp_eargm == NULL) {
            debug("node not found, skipping");
            continue;
        }
        rc = remote_connect(tmp_eargm, port);
        if (rc < 0) {
            debug("Error connecting to %s:%u", tmp_eargm, port);
            continue;
        }

        ret = eargm_status(rc, &aux_status);
        debug("Received %d eargm status", ret);
        if (ret > 0)
        {
            num_status += ret;
            final_status = realloc(final_status, sizeof(eargm_status_t)*num_status);
            memcpy(&final_status[num_status-1], aux_status, ret*sizeof(eargm_status_t));
            free(aux_status);
        }
        tmp_eargm = NULL; //reset the var
        remote_disconnect();
    }

    *status = final_status;

    return num_status;
}


//get all status under an eargmidx
int eargm_get_all_status(cluster_conf_t *conf, eargm_status_t **status, int eargm_idx)
{
    int i, j, rc, ret, port, num_status = 0;
    eargm_status_t *aux_status, *final_status = NULL;
    char *tmp_eargm = NULL;
    debug("entering eargm_get_all_status");

    for (i = 0; i < conf->eargm.eargms[eargm_idx].num_subs; i++)
    {
        //debug("contacting node %d", i);
        for (j = 0; j < conf->eargm.num_eargms; j++) {
            if (conf->eargm.eargms[j].id == conf->eargm.eargms[eargm_idx].subs[i]) {
                tmp_eargm = conf->eargm.eargms[j].node;
                port = conf->eargm.eargms[j].port;
                //debug("Found node on %s on port %u", tmp_eargm, port);
            }
        }
        if (tmp_eargm == NULL) {
            debug("node not found, skipping");
            continue;
        }
        rc = remote_connect(tmp_eargm, port);
        if (rc < 0) {
            error("Error connecting to %s:%u", tmp_eargm, port);
            continue;
        }

        ret = eargm_status(rc, &aux_status);
        debug("Received %d eargm status", ret);
        if (ret > 0)
        {
            num_status += ret;
            final_status = realloc(final_status, sizeof(eargm_status_t)*num_status);
            memcpy(&final_status[num_status-1], aux_status, ret*sizeof(eargm_status_t));
            free(aux_status);
        }
        tmp_eargm = NULL; //reset the var
        remote_disconnect();
    }

    *status = final_status;

    return num_status;

}

int eargm_status(int fd, eargm_status_t **status)
{
    request_t command;

    request_header_t head;
    eargm_status_t *aux_status;

    memset(&command, 0, sizeof(request_t));

    //set command
    command.req = EARGM_STATUS;

    //send command
    send_command(&command);

    //receive data
    head = receive_data(fd, (void **)&aux_status);
    debug("received header with size %u and type %d", head.size, head.type);
    debug("size of eargm_status_t %lu", sizeof(eargm_status_t));
    if (head.size < sizeof(eargm_status_t) || head.type != EAR_TYPE_EARGM_STATUS)
    {
        debug("Error getting eargm status");
        if (head.size > 0) free(status);
        return 0;
    }

    *status = aux_status;

    int num_status = head.size/sizeof(eargm_status_t);

    return num_status;
}

int eargm_send_table_commands(eargm_table_t *egm_table)
{
    int i, rc, count = 0;
    char tmp_eargm[256];
    struct sockaddr_in temp; 
    for (i = 0; i < egm_table->num_eargms; i++)
    {
        count++;
        if (egm_table->actions[i] == NO_COMMAND)  continue; //nothing to do there


        temp.sin_addr.s_addr = egm_table->eargm_ips[i];
        strcpy(tmp_eargm, inet_ntoa(temp.sin_addr));
        rc = remote_connect(tmp_eargm, egm_table->eargm_ports[i]);
        if (rc < 0) {
            error("Error connecting to node %s", tmp_eargm);
            continue;
        }
        switch(egm_table->actions[i])
        {
            case EARGM_INC_PC:
                if (!eargm_inc_powercap(egm_table->action_values[i])) {
                    error("Error sending powercap increase to %s", tmp_eargm);
                    count--; //this is not great, but easy enough
                }
                break;
            case EARGM_RED_PC:
                if (!eargm_red_powercap(egm_table->action_values[i])) {
                    error("Error sending powercap increase to %s", tmp_eargm);
                    count--;
                }
                break;
            case EARGM_RESET_PC:
                if (!eargm_reset_powercap()) {
                    error("Error sending powercap increase to %s", tmp_eargm);
                    count--;
                }
                break;
            default:
                error("Unknown command in EARGM table");
                count--;
                break;
        }
        remote_disconnect();

    }
    return count;
}

int eargm_new_job(uint num_nodes)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EARGM_NEW_JOB;
    command.my_req.eargm_data.num_nodes=num_nodes;
    verbose(2,"command %u num_nodes %u\n", command.req, command.my_req.eargm_data.num_nodes);
    return send_command(&command);
}

int eargm_end_job(uint num_nodes)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EARGM_END_JOB;
    command.my_req.eargm_data.num_nodes=num_nodes;
    verbose(2,"command %u num_nodes %u\n", command.req, command.my_req.eargm_data.num_nodes);
    return send_command(&command);
}

int eargm_inc_powercap(uint increase)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EARGM_INC_PC;
    command.my_req.eargm_data.pc_change=increase;
    verbose(2,"command %u pc_change %u\n", command.req, command.my_req.eargm_data.pc_change);
    return send_command(&command);
}

int eargm_red_powercap(uint decrease)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EARGM_RED_PC;
    command.my_req.eargm_data.pc_change=decrease;
    verbose(2,"command %u pc_change %u\n", command.req, command.my_req.eargm_data.pc_change);
    return send_command(&command);
}

int eargm_reset_powercap()
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EARGM_RESET_PC;
    verbose(2,"command %u\n",command.req);
    return send_command(&command);
}

int eargm_disconnect()
{
    eargm_remote_connected=0;
    close(eargm_sfd);
    return EAR_SUCCESS;
}

int eargm_send_set_powerlimit(uint limit)
{
	request_t command;
	memset(&command, 0, sizeof(request_t));
	command.req = EARGM_SET_PC;
	command.my_req.eargm_data.pc_change = limit;
	verbose(2, "command %u pc_change %u\n", command.req, command.my_req.eargm_data.pc_change);
	return send_command(&command);
}

