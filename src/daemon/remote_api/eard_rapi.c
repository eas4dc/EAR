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
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>
#include <common/output/verbose.h>
#include <common/messaging/msg_conf.h>

#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_server_api.h>

extern int eards_sfd;

/** Asks application status for a single node */
int eards_get_app_master_status(cluster_conf_t *my_cluster_conf, app_status_t **status)
{
	request_t command;
    memset(&command, 0, sizeof(request_t));
    request_header_t head;

    debug("eards_status");
	command.req = EAR_RC_APP_MASTER_STATUS;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);

    send_command(&command);

    head = receive_data(eards_sfd, (void **)status);
    //we don't need to check if size is bigger than app_status since an empty message can be returned (no apps running)
    if (head.type != EAR_TYPE_APP_STATUS) {
        debug("Error sending command to node");
        if (head.size > 0 && head.type != EAR_ERROR) free(status);
        return 0;
    }

    return head.size >= sizeof(app_status_t);
}

/** Asks application status for a single node */
int eards_get_app_node_status(cluster_conf_t *my_cluster_conf, app_status_t **status)
{
	request_t command;
    memset(&command, 0, sizeof(request_t));
    request_header_t head;

    debug("eards_status");
	command.req = EAR_RC_APP_NODE_STATUS;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);

    send_command(&command);

    head = receive_data(eards_sfd, (void **)status);
    //we don't need to check if size is bigger than app_status since an empty message can be returned (no apps running)
    if (head.type != EAR_TYPE_APP_STATUS) {
        debug("Error sending command to node");
        if (head.size > 0 && head.type != EAR_ERROR) free(status);
        return 0;
    }

    return head.size >= sizeof(app_status_t);
}

int send_status(request_t *command, status_t **status)
{
    request_header_t head;
    send_command(command);
    head = receive_data(eards_sfd, (void**)status);
    debug("receive_data with type %d and size %u", head.type, head.size);
    if (head.type != EAR_TYPE_STATUS) {
        error("Invalid type error, got type %d expected %d", head.type, EAR_TYPE_STATUS);
        if (head.size > 0 && head.type != EAR_ERROR) free(status);
        return EAR_ERROR;
    }

    return (head.size/sizeof(status_t));

}

int eards_get_status(cluster_conf_t *my_cluster_conf, status_t **status)
{
	request_t command;
    memset(&command, 0, sizeof(request_t));
    request_header_t head;

    debug("eards_status");
	command.req=EAR_RC_STATUS;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);

    send_command(&command);

    head = receive_data(eards_sfd, (void **)status);
    if (head.size < sizeof(status_t) || head.type != EAR_TYPE_STATUS) {
        debug("Error sending command to node");
        if (head.size > 0 && head.type != EAR_ERROR) free(status);
        return 0;
    }

    return head.size >= sizeof(status_t);
}

/** REMOTE FUNCTIONS FOR SINGLE NODE COMMUNICATION */
int eards_new_job(new_job_req_t *new_job)
{
	request_t command;
    memset(&command, 0, sizeof(request_t));

    debug("eards_new_job");
	command.req=EAR_RC_NEW_JOB;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	memcpy(&command.my_req.new_job,new_job,sizeof(new_job_req_t));
	debug("command %u job_id %lu,%lu",command.req,command.my_req.new_job.job.id,command.my_req.new_job.job.step_id);
	return send_command(&command);
}

int eards_end_job(job_id jid,job_id sid)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));

    debug("eards_end_job");
    command.req=EAR_RC_END_JOB;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	command.my_req.end_job.jid=jid;
	command.my_req.end_job.sid=sid;
	debug("command %u job_id %lu step_id %lu ",command.req,command.my_req.end_job.jid,command.my_req.end_job.sid);
	return send_command(&command);
}

int eards_set_max_freq(unsigned long freq)
{
	request_t command;
    memset(&command, 0, sizeof(request_t));
	command.req=EAR_RC_MAX_FREQ;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.max_freq=freq;
	return send_command(&command);
}

int eards_set_freq(unsigned long freq)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_FREQ;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.max_freq=freq;
    return send_command(&command);
}
int eards_set_def_freq(unsigned long freq)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_DEF_FREQ;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.max_freq=freq;
    return send_command(&command);
}

int eards_red_max_and_def_freq(uint p_states)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_RED_PSTATE;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.p_states=p_states;
    return send_command(&command);
}

int eards_restore_conf()
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_REST_CONF;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    return send_command(&command);
}

// New th must be passed as % th=0.75 --> 75
int eards_set_th(unsigned long th)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
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
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_INC_TH;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    command.my_req.ear_conf.th=th;
    return send_command(&command);
}

int eards_ping()
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_PING;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    return send_command(&command);
}

#if POWERCAP
/* Power management */
int eards_set_powerlimit(unsigned long limit)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
  	command.node_dist = INT_MAX;
    command.req=EAR_RC_SET_POWER;
    command.time_code = time(NULL);
    command.my_req.pc.limit=limit;
    return send_command(&command);
}

void cluster_set_powerlimit_all_nodes(unsigned long limit, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_POWER;
    command.my_req.pc.limit=limit;
    send_command_all(command, my_cluster_conf);
}

int eards_red_powerlimit(unsigned int type, unsigned long limit)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.node_dist = INT_MAX;
    command.req=EAR_RC_RED_POWER;
    command.time_code = time(NULL);
    command.my_req.pc.limit=limit;
    command.my_req.pc.type=type;
    return send_command(&command);
}

void cluster_red_powerlimit_all_nodes(unsigned int type, unsigned long limit, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_RED_POWER;
    command.my_req.pc.limit=limit;
    command.my_req.pc.type=type;
    send_command_all(command, my_cluster_conf);
}

int eards_inc_powerlimit(unsigned int type, unsigned long limit)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.node_dist = INT_MAX;
    command.req=EAR_RC_INC_POWER;
    command.time_code = time(NULL);
    command.my_req.pc.limit=limit;
    command.my_req.pc.type=type;
    return send_command(&command);
}   

void cluster_inc_powerlimit_all_nodes(unsigned int type, unsigned long limit, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_INC_POWER;
    command.my_req.pc.limit=limit;
    command.my_req.pc.type=type;
    send_command_all(command, my_cluster_conf);
}

#endif

int eards_set_risk(risk_t risk,unsigned long target)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.node_dist = INT_MAX;
    command.req=EAR_RC_SET_RISK;
    command.time_code = time(NULL);
	command.my_req.risk.level=risk;
	command.my_req.risk.target=target;
    return send_command(&command);
}

/* End new functions for power limit management */

int eards_set_policy_info(new_policy_cont_t *p)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_POLICY;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
	memcpy(&command.my_req.pol_conf,p,sizeof(new_policy_cont_t));
    return send_command(&command);
}

int eards_reset_default_powercap()
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_DEF_POWERCAP;
    command.node_dist = INT_MAX;
    command.time_code = time(NULL);
    return send_command(&command);
}



/* END OF SINGLE NODE COMMUNICATION */


/*
*	SAME FUNCTIONALLITY BUT SENT TO ALL NODES
*/
/** Asks application status for master nodes */
int get_app_master_status_all_nodes(cluster_conf_t *my_cluster_conf, app_status_t **status)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    app_status_t *temp_status;
    request_header_t head;
    time_t ctime = time(NULL);
    int num_status = 0;

    command.time_code = ctime;
    command.req = EAR_RC_APP_MASTER_STATUS;
    command.node_dist = 0;

    head = data_all_nodes(&command, my_cluster_conf, (void **)&temp_status);
    num_status = head.size / sizeof(app_status_t);

    debug("appstatus: received %d size of %d type", head.size, head.type);

    if (head.type != EAR_TYPE_APP_STATUS || head.size < sizeof(app_status_t))
    {
        if (head.size > 0) free (temp_status);
        *status = temp_status;
        return 0;
    }

    *status = temp_status;

    return num_status;

}

/** Asks application status for all single nodes */
int get_app_node_status_all_nodes(cluster_conf_t *my_cluster_conf, app_status_t **status)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    app_status_t *temp_status;
    request_header_t head;
    time_t ctime = time(NULL);
    int num_status = 0;

    command.time_code = ctime;
    command.req = EAR_RC_APP_NODE_STATUS;
    command.node_dist = 0;

    head = data_all_nodes(&command, my_cluster_conf, (void **)&temp_status);
    num_status = head.size / sizeof(app_status_t);

    debug("appstatus: received %d size of %d type", head.size, head.type);

    if (head.type != EAR_TYPE_APP_STATUS || head.size < sizeof(app_status_t))
    {
        if (head.size > 0) free (temp_status);
        *status = temp_status;
        return 0;
    }

    *status = temp_status;

    return num_status;

}

void set_risk_all_nodes(risk_t risk, unsigned long target, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_RISK;
    command.time_code = time(NULL);
	command.my_req.risk.level=risk;
	command.my_req.risk.target=target;
    send_command_all(command, my_cluster_conf);
}

void increase_th_all_nodes(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_INC_TH;
    command.my_req.ear_conf.th=th;
    command.my_req.ear_conf.p_id=p_id;
    send_command_all(command, my_cluster_conf);
}

#if NODE_PROP
void eards_new_job_nodelist(new_job_req_t *new_job, uint port, char *net_ext, char **nodes, int num_nodes)
{
	request_t command;
    int *ips;
    cluster_conf_t conf;
    memset(&conf, 0, sizeof(cluster_conf_t));
    conf.eard.port = port;
    if (net_ext != NULL)
        strcpy(conf.net_ext, net_ext);
    else
        strcpy(conf.net_ext, "");

    get_ip_nodelist(&conf, nodes, num_nodes, &ips);

    memset(&command, 0, sizeof(request_t));

    debug("eards_new_job");
    command.nodes = ips;
    command.num_nodes = num_nodes;
	command.req=EAR_RC_NEW_JOB;
    command.time_code = time(NULL);
	memcpy(&command.my_req.new_job,new_job,sizeof(new_job_req_t));
    send_command_nodelist(&command, &conf);
    free(ips);
}

void eards_new_job_iplist(new_job_req_t *new_job, cluster_conf_t *my_cluster_conf, int *ips, int num_ips)
{
	request_t command;
    memset(&command, 0, sizeof(request_t));

    debug("eards_new_job");
    command.nodes = ips;
    command.num_nodes = num_ips;
	command.req=EAR_RC_NEW_JOB;
    command.time_code = time(NULL);
	memcpy(&command.my_req.new_job,new_job,sizeof(new_job_req_t));
    send_command_nodelist(&command, my_cluster_conf);
}

void eards_end_job_nodelist(job_id jid, job_id sid, uint port, char *net_ext, char **nodes, int num_nodes)
{
    request_t command;
    int *ips;
    cluster_conf_t conf;
    memset(&conf, 0, sizeof(cluster_conf_t));
    conf.eard.port = port;
    if (net_ext != NULL)
        strcpy(conf.net_ext, net_ext);
    else
        strcpy(conf.net_ext, "");

    get_ip_nodelist(&conf, nodes, num_nodes, &ips);

    memset(&command, 0, sizeof(request_t));

    debug("eards_end_job");
    command.nodes = ips;
    command.num_nodes = num_nodes;
    command.req=EAR_RC_END_JOB;
    command.time_code = time(NULL);
	command.my_req.end_job.jid=jid;
	command.my_req.end_job.sid=sid;
    send_command_nodelist(&command, &conf);
    free(ips);
}

void eards_end_job_iplist(job_id jid, job_id sid, cluster_conf_t *my_cluster_conf, int *ips, int num_ips)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));

    debug("eards_end_job");
    command.nodes = ips;
    command.num_nodes = num_ips;
    command.req=EAR_RC_END_JOB;
    command.time_code = time(NULL);
	command.my_req.end_job.jid=jid;
	command.my_req.end_job.sid=sid;
    send_command_nodelist(&command, my_cluster_conf);
}

void increase_th_nodelist(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf, int *ips, int num_ips)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.nodes = ips;
    command.num_nodes = num_ips;
    command.req=EAR_RC_INC_TH;
    command.my_req.ear_conf.th=th;
    command.my_req.ear_conf.p_id=p_id;
    send_command_nodelist(&command, my_cluster_conf);
}

void restore_conf_nodelist(cluster_conf_t *my_cluster_conf, int *ips, int num_ips)
{
    request_t command;
    command.nodes = ips;
    command.num_nodes = num_ips;
    command.req=EAR_RC_REST_CONF;
    send_command_nodelist(&command, my_cluster_conf);
}

void set_risk_nodelist(risk_t risk, unsigned long target, cluster_conf_t *my_cluster_conf, int *ips, int num_ips)
{
    request_t command;
    command.nodes = ips;
    command.num_nodes = num_ips;
    command.req=EAR_RC_SET_RISK;
    command.my_req.risk.level=risk;
    command.my_req.risk.target=target;
    send_command_nodelist(&command, my_cluster_conf);
}

void set_default_powercap_nodelist(cluster_conf_t *my_cluster_conf, int *ips, int num_ips)
{
    request_t command;
    command.nodes = ips;
    command.num_nodes = num_ips;
    command.req=EAR_RC_DEF_POWERCAP;
    send_command_nodelist(&command, my_cluster_conf);
}

int status_nodelist(cluster_conf_t *my_cluster_conf, status_t **status, int *ips, int num_ips)
{
    request_header_t head;
    request_t command;
    memset(&command, 0, sizeof(request_t));
    status_t *temp_status;
    time_t ctime = time(NULL);
    int num_status = 0;

    command.time_code = ctime;
    command.req = EAR_RC_STATUS;
    command.nodes = ips;
    command.num_nodes = num_ips;

    head = data_nodelist(&command, my_cluster_conf, (void **)&temp_status);
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
#else

void increase_th_nodelist(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf, int *ips, int num_ips) {}

void restore_conf_nodelist(cluster_conf_t *my_cluster_conf, int *ips, int num_ips) {}

void set_risk_nodelist(risk_t risk, unsigned long target, cluster_conf_t *my_cluster_conf, int *ips, int num_ips) {}

void set_default_powercap_nodelist(cluster_conf_t *my_cluster_conf, int *ips, int num_ips) {}

int status_nodelist(cluster_conf_t *my_cluster_conf, status_t **status, int *ips, int num_ips) {return 0;}
#endif

void set_th_all_nodes(ulong th, ulong p_id, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_NEW_TH;
    command.my_req.ear_conf.th=th;
    command.my_req.ear_conf.p_id=p_id;
    send_command_all(command, my_cluster_conf);
}


void ping_all_nodes_propagated(cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req = EAR_RC_PING;
    send_command_all(command, my_cluster_conf);
}

void set_max_freq_all_nodes(ulong max_freq, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_MAX_FREQ;
    command.my_req.ear_conf.max_freq = max_freq;
    send_command_all(command, my_cluster_conf);
}

void set_freq_all_nodes(ulong freq, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_FREQ;
    command.my_req.ear_conf.max_freq = freq;
    send_command_all(command, my_cluster_conf);
}

void red_def_max_pstate_all_nodes(ulong pstate, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_RED_PSTATE;
    command.my_req.ear_conf.p_states = pstate;
    send_command_all(command, my_cluster_conf);
}

/** Sets the default pstate for a given policy in all nodes */
void set_def_pstate_all_nodes(uint pstate,ulong pid,cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_DEF_PSTATE;
    command.my_req.ear_conf.p_states = pstate;
		command.my_req.ear_conf.p_id = pid;
    send_command_all(command, my_cluster_conf);
}

/** Sets the maximum pstate in all the nodes */
void set_max_pstate_all_nodes(uint pstate,cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_MAX_PSTATE;
    command.my_req.ear_conf.p_states = pstate;
    send_command_all(command, my_cluster_conf);
}


void reduce_frequencies_all_nodes(ulong freq, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_DEF_FREQ;
    command.my_req.ear_conf.max_freq=freq;
    send_command_all(command, my_cluster_conf);
}

void restore_conf_all_nodes(cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_REST_CONF;
    send_command_all(command, my_cluster_conf);
}

void set_def_freq_all_nodes(ulong freq, ulong policy, cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_DEF_FREQ;
    command.my_req.ear_conf.max_freq = freq;
	command.my_req.ear_conf.p_id = policy;
    send_command_all(command, my_cluster_conf);
}

void cluster_reset_default_powercap_all_nodes(cluster_conf_t *my_cluster_conf)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_DEF_POWERCAP;
    send_command_all(command, my_cluster_conf);
}

int status_all_nodes(cluster_conf_t *my_cluster_conf, status_t **status)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    status_t *temp_status;
    request_header_t head;
    time_t ctime = time(NULL);
    int num_status = 0;

    command.time_code = ctime;
    command.req = EAR_RC_STATUS;
    command.node_dist = 0;

    head = data_all_nodes(&command, my_cluster_conf, (void **)&temp_status);
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

#if POWERCAP
request_header_t send_powercap_status(request_t *command, powercap_status_t **status)
{
    request_header_t head;
    send_command(command);
    head = receive_data(eards_sfd, (void**)status);
    debug("receive_data with type %d and size %u", head.type, head.size);
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

int eards_get_powercap_status(cluster_conf_t *my_cluster_conf, powercap_status_t **pc_status) 
{
    powercap_status_t *temp_status;
    request_t command;
    memset(&command, 0, sizeof(request_t));

    command.node_dist = INT_MAX;
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
    memset(&command, 0, sizeof(request_t));
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
    memset(&command, 0, sizeof(request_t));
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
int cluster_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt)
{
    request_t command;
    memset(&command, 0, sizeof(request_t));
    command.req=EAR_RC_SET_POWERCAP_OPT;
    command.time_code = time(NULL);
    command.my_req.pc_opt=*pc_opt;
    send_command_all(command, my_cluster_conf);
	return EAR_SUCCESS;
}
#endif

/* pings all nodes */
void ping_all_nodes(cluster_conf_t *my_cluster_conf)
{
    int i, j, k, rc; 
    char node_name[256];
    debug("Sendind ping_all_nodes");
    //it is always secuential as it is only used for debugging purposes
    for (i=0;i< my_cluster_conf->num_islands;i++){
        for (j = 0; j < my_cluster_conf->islands[i].num_ranges; j++)
        {   
            for (k = my_cluster_conf->islands[i].ranges[j].start; k <= my_cluster_conf->islands[i].ranges[j].end; k++)
            {   
                if (k == -1) 
                    sprintf(node_name, "%s", my_cluster_conf->islands[i].ranges[j].prefix);
                else if (my_cluster_conf->islands[i].ranges[j].end == my_cluster_conf->islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf->islands[i].ranges[j].prefix, k); 
                else {
                    if (k < 10 && my_cluster_conf->islands[i].ranges[j].end > 10) 
                        sprintf(node_name, "%s0%u", my_cluster_conf->islands[i].ranges[j].prefix, k); 
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf->islands[i].ranges[j].prefix, k); 
                }   
                rc=eards_remote_connect(node_name,my_cluster_conf->eard.port);
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
