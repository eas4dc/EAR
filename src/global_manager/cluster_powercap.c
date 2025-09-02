/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE 

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// #define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/colors.h>
#include <common/types/log.h>
#include <common/system/execute.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>

#include <daemon/remote_api/eard_rapi_internals.h>
#include <daemon/remote_api/eard_rapi.h>

#include <global_manager/cluster_powercap.h>
#include <global_manager/log_eargmd.h>

#include <report/report.h>



extern cluster_conf_t my_cluster_conf;
extern int64_t default_cluster_powercap;
extern uint  cluster_powercap_period;

extern uint process_created;
extern pthread_mutex_t plocks;
void check_pending_processes();

uint current_cluster_powercap;
uint current_cluster_power;
uint min_cluster_powercap;
uint powercap_unlimited = 0;
uint risk = 0;
uint max_num_nodes = 0;
/* Shared data with eargm_ext thread to sent to meta_eargms */
shared_powercap_data_t ext_powercap_data;
pthread_mutex_t ext_mutex = PTHREAD_MUTEX_INITIALIZER;

uint num_nodes;
uint actions_executed = 0;
uint total_req_new,total_req_greedy,req_no_extra,num_no_extra,num_greedy,num_extra,extra_power_alloc,total_extra_power,greedy_allocated;
cluster_powercap_status_t *my_cluster_power_status;
int num_power_status;
powercap_opt_t cluster_options;
extern uint policy;
extern char **nodes;
extern int num_eargm_nodes;
extern char host[GENERIC_NAME];
static uint must_send_pc_options;
static ulong current_extra_power; 
static uint total_free;
#define min(a,b) (a<b?a:b)

void check_powercap_actions(uint cluster_powercap);
void write_shared_data(ulong current_power, ulong freeable_power, ulong requested);

void eargm_report_event(uint event_type, ulong value)
{
    ear_event_t event;

    event.timestamp = time(NULL);
    event.jid 			= 0;
    event.step_id		= 0;

    char *gm_name = ear_getenv("EARGMNAME");
    int ret;
    if (gm_name)
        ret = snprintf(event.node_id, sizeof(event.node_id), "eargm-%s-%s", gm_name, host);
    else
        ret = snprintf(event.node_id, sizeof(event.node_id), "eargm-%s", host);

    if (ret < 0) {
        verbose(VGM_PC, "%sERROR%s Writing the hostname to the event.", COL_RED, COL_CLR);
    } else if (ret >= sizeof(event.node_id)) {
        verbose(VGM_PC, "%sWARNING%s The node id field of the event was truncated. Required size: %d", COL_YLW, COL_CLR, ret);
    }

    event.event     = event_type;
    event.value     = value;
    report_events(NULL, &event, 1);
    //db_insert_ear_event(&event);
}



void aggregate_data(powercap_status_t *cs)
{
    int i;
    total_req_greedy=0;
    total_extra_power=0;
    req_no_extra=0;
    num_no_extra=0;
    num_greedy=0;
    num_extra=0;
    extra_power_alloc=0;
    for (i=0;i<cs->num_greedy;i++){
        total_req_greedy+=cs->greedy_data[i].requested;
        total_extra_power+=cs->greedy_data[i].extra_power;
        if (cs->greedy_data[i].extra_power){ 
            num_extra++;
            extra_power_alloc+=cs->greedy_data[i].extra_power;
        }
        if (cs->greedy_data[i].requested) num_greedy++;
        if ((cs->greedy_data[i].requested) && (!cs->greedy_data[i].extra_power)){
            req_no_extra+=cs->greedy_data[i].requested;
            num_no_extra++;
        }
    }
}

void print_powercap_opt(powercap_opt_t *opt)
{
    int i;
    verbose(VGM_PC,"%s",COL_GRE);
    verbose(VGM_PC,"num_greedy %u",opt->num_greedy);
    for (i=0;i<opt->num_greedy;i++){
        verbose(VGM_PC,"greedy_node %d extra power %d",opt->greedy_nodes[i],opt->extra_power[i]);
    }
    verbose(VGM_PC,"Cluster power allocation percentage %u",opt->cluster_perc_power);
    verbose(VGM_PC,"%s",COL_CLR);
}


void allocate_free_power_to_greedy_nodes(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint *total_free)
{ 
    int i,more_power;
    uint pending=*total_free;
    if (num_greedy==0){
        debug("NO greedy nodes, returning");
        return;
    }
    if (pending==0){
        debug("NO free power, returning");
        return;
    }
    must_send_pc_options=1;
    debug("allocate_free_power_to_greedy_nodes----------------");
    verbose(VGM_PC+1,"Total extra power for nodes with NO current extra power %u W, %u nodes",req_no_extra,num_no_extra);
    verbose(VGM_PC+1,"Nodes with extra power already allocated %u",num_greedy);
    greedy_allocated=0;
    if (num_no_extra>0){
        if (req_no_extra<pending) more_power=-1;
        else more_power=pending/num_no_extra;
        verbose(VGM_PC+1,"STAGE_1-Allocating %d watts to new greedy nodes first (-1 => alloc=req)",more_power);
        for (i=0;i<cluster_status->num_greedy;i++){
            if ((cluster_status->greedy_data[i].requested) && (!cluster_status->greedy_data[i].extra_power)){
                if (more_power<0) cluster_options->extra_power[i]=cluster_status->greedy_data[i].requested;
                else cluster_options->extra_power[i]=min(more_power,cluster_status->greedy_data[i].requested);
                pending-=cluster_options->extra_power[i];
                greedy_allocated += cluster_options->extra_power[i];
            }
        }
    }
    /* If there is pending power to allocate */
    verbose(VGM_PC+1,"STAGE-2 allocating %uW to the rest of greedy nodes",pending);
    if (pending){
        verbose(VGM_PC+1,"Allocating %u watts to greedy nodes ",pending);
        more_power=pending/num_greedy;
        for (i=0;i<cluster_status->num_greedy;i++){
            if ((cluster_status->greedy_data[i].requested) && (!cluster_options->extra_power[i])){
                cluster_options->extra_power[i]=min(cluster_status->greedy_data[i].requested,more_power);
                pending-=cluster_options->extra_power[i];
                greedy_allocated += cluster_options->extra_power[i];
            }
        }
    }
    *total_free=pending;
}

/* This function is executed when there is not enough power for new running nodes */
void reduce_allocation(cluster_powercap_status_t *cs,powercap_opt_t *cluster_options,uint min_reduction)
{
    int i=0;
    uint red1,red,red_node;
    int new_extra;
    if (num_extra==0){
        error("We need to reallocated power and there is no extra power ");
        return;
    }
    red_node=min_reduction/num_extra;
    verbose(VGM_PC+1,"SEQ reduce_allocation implementation avg red=%uW",red_node);
    debug("ROUND 1- reducing avg power ");
    while((min_reduction>0) && (i<cs->num_greedy)){
        if (cs->greedy_data[i].extra_power){
            red1=min(red_node,cs->greedy_data[i].extra_power);
            red=min(red1,min_reduction);
            new_extra=-red;
            verbose(VGM_PC+1,"%sreducing %d W to node %d %s",COL_RED,new_extra,i,COL_CLR);
            cluster_options->extra_power[i]=new_extra;
            min_reduction-=red;
        }
        i++;
    }
    debug("ROUND 2- Reducing remaining power ");
    i=0;
    while((min_reduction>0) && (i<cs->num_greedy)){
        /* cluster_options->extra_power[i] is a negative value */
        if ((cs->greedy_data[i].extra_power+cluster_options->extra_power[i])>0){
            red1=cs->greedy_data[i].extra_power+cluster_options->extra_power[i];
            red=min(red1,min_reduction);
            new_extra=-red;
            verbose(VGM_PC+1,"%sreducing %d W to node %d %s",COL_RED,new_extra,i,COL_CLR);
            cluster_options->extra_power[i]+=new_extra;
            min_reduction-=red;
        }
        i++;
    }

}

/* This function is executed when there is enough power for new running nodes but not for all the greedy nodes */
uint powercap_reallocation(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options)
{
    int i;
    uint num_nodes = cluster_status->total_nodes;
    uint min_reduction;
    verbose(VGM_PC+1,"There are %u nodes  %u idle nodes %u greedy nodes ", num_nodes,cluster_status->idle_nodes,cluster_status->num_greedy);
    if (cluster_options->greedy_nodes == NULL){
        cluster_options->greedy_nodes  = calloc(cluster_status->num_greedy,sizeof(int));
        cluster_options->extra_power   = calloc(cluster_status->num_greedy,sizeof(uint));
    }
    cluster_options->num_greedy    = cluster_status->num_greedy;
    cluster_options->cluster_perc_power = (cluster_status->total_powercap * 100)/current_cluster_powercap;
    memcpy(cluster_options->greedy_nodes,cluster_status->greedy_nodes,cluster_status->num_greedy*sizeof(int));
    total_free = current_cluster_powercap - cluster_status->total_powercap;
    verbose(VGM_PC+1,"Total power %u , requested for new %u (potential) released %u extra_req %u extra_used %u",current_cluster_powercap,cluster_status->requested,cluster_status->released,total_req_greedy,total_extra_power);
    verbose(VGM_PC+1,"Free power before reallocation %d",total_free);

    if (cluster_status->requested) must_send_pc_options=1;

    /* Allocated power + default requested must be less that maximum: ASK_DEF */
    if ((cluster_status->total_powercap + cluster_status->requested) <= current_cluster_powercap){
        if (cluster_status->requested){
            verbose(VGM_PC+1,"There is enough power for new running jobs");
            total_free = current_cluster_powercap - (cluster_status->total_powercap + cluster_status->requested);
            verbose(VGM_PC+1,"Free power after allocating power to new jobs %d",total_free);
        }
        if (total_req_greedy == 0){
            return 1;
        }
        /* At this point we know there is greedy power requested: GREEDY */
        if (total_req_greedy <= total_free) {
            verbose(VGM_PC+1,"There is more free power than requested by greedy nodes=> allocating all the req power");
            total_free -= total_req_greedy;
            for (i = 0; i < cluster_status->num_greedy; i++)
                cluster_options->extra_power[i] = cluster_status->greedy_data[i].requested;
            must_send_pc_options = 1;
        }else{
            verbose(VGM_PC+1,"There is not enough power for all the greedy nodes (free %d req %u)(used %u allocated %u)",total_free,total_req_greedy,cluster_status->current_power,cluster_status->total_powercap);
            if (cluster_status->released == 0 ){
                verbose(VGM_PC+1,"Anyway there is not enough power ");
                cluster_options->num_greedy = cluster_status->num_greedy;
                allocate_free_power_to_greedy_nodes(cluster_status,cluster_options,(uint *)&total_free);
            }else if (cluster_status->released){ 
                return 0; // We try to release power to have power for greedy nodes
            }
        }
    }else{
        /* There is not enough power for new jobs (ASK_DEF), we must reduce the extra allocation */
        if (cluster_status->released == 0){
            verbose(VGM_PC+1,"We must reduce the extra allocation (used %u allocated %u)",cluster_status->current_power,cluster_status->total_powercap);
            min_reduction = (cluster_status->total_powercap+cluster_status->requested)-current_cluster_powercap;
            total_free = 0;
            reduce_allocation(cluster_status,cluster_options,min_reduction);
            must_send_pc_options = 1;
        }else if (cluster_status->released) return 0; // We try to release power to have power for greedy nodes
    }
    return 1;
}

void send_powercap_options_to_cluster(powercap_opt_t *cluster_options)
{
    ear_cluster_set_powercap_opt(&my_cluster_conf,cluster_options);
}


void print_cluster_power_status(powercap_status_t *my_cluster_power_status)
{
    int i;
    powercap_status_t *cs=my_cluster_power_status;
    if (cs==NULL){
        debug("cluster status is NULL");
        return;
    }
    verbose(VGM_PC,"%sTotal %u Idle  %u power (released %u requested %u consumed %u allocated %u allocated_to_idle %u) total_greedy_nodes %u%s",COL_CLR,cs->total_nodes,cs->idle_nodes, cs->released,cs->requested,cs->current_power,cs->total_powercap,cs->num_greedy,cs->total_idle_power,COL_CLR);
    debug("%d power_status received",num_power_status);
    for (i=0;i<cs->num_greedy;i++){
        verbose(VGM_PC,"%s\t[%d] ip %d greedy_req %u extra_power %u%s", COL_CLR, i, cs->greedy_nodes[i], cs->greedy_data[i].requested, cs->greedy_data[i].extra_power,COL_CLR);
    }
}

pthread_t cluster_powercap_th;

/* Cluster Soft powercap */
void cluster_only_powercap()
{

    power_check_t power;

		my_cluster_power_status = calloc(1, sizeof(cluster_powercap_status_t));

    memset(&power, 0, sizeof(power_check_t));
    ear_get_power(&my_cluster_conf, &power, nodes, num_eargm_nodes);

		eargm_report_event(CLUSTER_POWER, (ulong)power.power);

    float upper_limit = ((float)(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_limit)/100.0);
    float lower_limit = ((float)(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_lower)/100.0);


    verbose(VGM_PC, "cluster_only_powercap, power %ld and nodes %ld", power.power, power.num_nodes);
    if (current_cluster_powercap == POWER_CAP_UNLIMITED) {
        verbose(VGM_PC, "cluster_powercap is unlimited, nothing to do");
        write_shared_data(power.power, 0, 0);
				free(my_cluster_power_status);
        return;
    }

    uint64_t freeable_power = 0;
    uint64_t requested = 0;
    uint64_t new_limit = 0;
    if (power.power > upper_limit && risk == 0) {
        /* For heterogeneous clusters we set the max powercap for each node, which should be set by the admin in ear.conf
         * For homogeneous clusters we can simply divide the current power between the number of active nodes. */
        if (max_num_nodes > 0) {
#if EARGM_POWERCAP_SOFTPC_HOMOGENEOUS
            new_limit = current_cluster_powercap/max_num_nodes; 
#else
            new_limit = UINT_MAX; 
#endif
            ear_set_powerlimit(&my_cluster_conf, new_limit, nodes, num_eargm_nodes); 
            verbose(VGM_PC, "cluster_only_powercap: sending new limit %lu", (ulong) new_limit);
            eargm_report_event(NODE_POWERCAP, (ulong) new_limit);
        } else {
            warning("No nodes detected for this EARGM");
        }
        requested = power.power - upper_limit;
        risk = 1;
    } else {
        if (risk == 1 && power.power <= lower_limit) {
            verbose(VGM_PC, "cluster_only_powercap: sending POWER_CAP_UNLIMTED to nodes");
            ear_set_powerlimit(&my_cluster_conf, 1, nodes, num_eargm_nodes); //set powercap unlimited
            risk = 0;
            eargm_report_event(POWER_UNLIMITED, 1);
        }
        freeable_power = (uint64_t) (upper_limit - power.power) / 2; //for now, we will free half of whatever extra power
    }
    write_shared_data(power.power, freeable_power, requested);

    /* These values in my_cluster_power_status are used automatically by the actions */
    my_cluster_power_status->current_power = power.power;
    my_cluster_power_status->idle_nodes    = 0;
    my_cluster_power_status->total_idle_power = 0;


    check_powercap_actions(power.power);
		
		free(my_cluster_power_status);
}


void *eargm_powercap_th(void *noarg)
{
    if (pthread_setname_np(pthread_self(), "cluster_powercap")!=0) error("Setting name forcluster_powercap thread %s", strerror(errno));

    if (my_cluster_conf.eargm.powercap_mode == SOFT_POWERCAP)
        ear_set_powerlimit(&my_cluster_conf, 1, nodes, num_eargm_nodes); // set powercap to unlimited at the start
    while(1)
    {
        sleep(cluster_powercap_period);
        verbose(VGM_PC, "new powercap period");
        /* we check if there are zombie processes */
        if (process_created){
            check_pending_processes();
        }
        if (cluster_power_limited()){
			switch(my_cluster_conf.eargm.powercap_mode) {
				case SOFT_POWERCAP:
					cluster_only_powercap();
					break;
				case HARD_POWERCAP:
					cluster_check_powercap();
					break;
				case MONITOR:
				default:
					cluster_power_monitor();
					break;
			}
        }

    }

}

void cluster_powercap_reset_pc()
{
    verbose(VGM_PC, "Resetting cluster powercap to %ld", default_cluster_powercap);
    current_cluster_powercap = default_cluster_powercap;
    current_extra_power = 0;
}

void cluster_powercap_set_pc(uint limit)
{
    verbose(VGM_PC, "Setting cluster powercap to %u", limit);

	current_cluster_powercap = limit;

	if (default_cluster_powercap < current_cluster_powercap)
		current_extra_power = current_cluster_powercap - default_cluster_powercap;
	else
		current_extra_power = 0;
#if 0
    pthread_mutex_lock(&ext_mutex);
    ext_powercap_data.current_powercap = current_cluster_powercap;
    pthread_mutex_unlock(&ext_mutex);
#endif

}

void cluster_powercap_red_pc(uint red)
{
    verbose(VGM_PC, "Reducing cluster powercap by %u (from %u to %u)", red, current_cluster_powercap, current_cluster_powercap-red);
    if (red > current_cluster_powercap) {
        verbose(VGM_PC, "Cannot reduce powercap by %u (would overflow)", red);
    } else {
        current_cluster_powercap -= red;
    }

    if (default_cluster_powercap < current_cluster_powercap)
        current_extra_power = current_cluster_powercap - default_cluster_powercap;
    else
        current_extra_power = 0;
}

void cluster_powercap_inc_pc(uint inc)
{
    verbose(VGM_PC, "Increasing cluster powercap by %u (from %u to %u)", inc, current_cluster_powercap, current_cluster_powercap+inc);
    current_cluster_powercap += inc;
    if (default_cluster_powercap < current_cluster_powercap)
        current_extra_power = current_cluster_powercap - default_cluster_powercap;
    else
        current_extra_power = 0;
}

uint cluster_get_min_power(cluster_conf_t *conf, char type)
{
    uint total = 0;
    int i, j, k;
    int tag_id, def_tag_id;
    def_tag_id = get_default_tag_id(conf);

    for (i = 0; i < conf->num_islands; i++)
    {
        for (j = 0; j < conf->islands[i].num_ranges; j++)
        {
            node_range_t *current_range = &conf->islands[i].ranges[j];
            tag_id = -1;
            for (k = 0; k < current_range->num_tags; k++)
            {
                tag_id = tag_id_exists(conf, conf->islands[i].specific_tags[current_range->specific_tags[k]]);
                if (tag_id >= 0) {
                    break;
                }
            }
            if (tag_id < 0) {
                tag_id = def_tag_id;
            }
            if (type == ENERGY_TYPE) {
                total += conf->tags[tag_id].max_power * (current_range->end - current_range->start + 1);

            }
            else {
                total += conf->tags[tag_id].powercap * (current_range->end - current_range->start + 1);
            }
        }
    }

    return total;
}

void cluster_powercap_init(cluster_conf_t *cc)
{
    int ret;
    max_num_nodes = get_num_nodes(cc);
    current_cluster_powercap = default_cluster_powercap;
    min_cluster_powercap = cluster_get_min_power(cc, POWER_TYPE);
    if (current_cluster_powercap == -1) current_cluster_powercap = min_cluster_powercap;

    if (current_cluster_powercap>0){
        verbose(VGM_PC,"Power cap limit set to %u",current_cluster_powercap);
    }else{
        verbose(VGM_PC,"Power cap unlimited");
    }

    if (current_cluster_powercap == 0) return;

    /* This thread accepts external commands */
    if ((ret=pthread_create(&cluster_powercap_th, NULL, eargm_powercap_th, NULL))){
        errno=ret;
        error("error creating eargm_server for external api %s",strerror(errno));
    }
    powercap_unlimited = tags_are_unlimited(cc); 
}

int cluster_power_limited()
{
    return (current_cluster_powercap);
}

/* This function is executed when we are in a good position after executing the higher limit action*/
void execute_powercap_lower_action()
{
    char cmd[300];
    /* If action is different than no_action we run the command */
    if (strcmp(my_cluster_conf.eargm.powercap_lower_action,"no_action")){ 
        /* Format is: command current_power current_limit total_idle_nodes allocated_idle_power*/
        sprintf(cmd,"%s %u %u %u %u", my_cluster_conf.eargm.powercap_lower_action,my_cluster_power_status->current_power,current_cluster_powercap,my_cluster_power_status->idle_nodes,my_cluster_power_status->total_idle_power);
        verbose(VGM_PC,"%sExecuting powercap resume action: %s %s",COL_GRE,cmd,COL_CLR);
        execute_with_fork(cmd);
        pthread_mutex_lock(&plocks);
        process_created++;
        pthread_mutex_unlock(&plocks);
        actions_executed = 0;
    } else {
        verbose(VGM_PC, "No action for powercap lower limit");
    }
}

/* This function is executed when we are close to the limit , consumed or allocated, depending of powercap_mode*/
void execute_powercap_limit_action()
{
    char cmd[300];
    /* If action is different than no_action we run the command */
    if (strcmp(my_cluster_conf.eargm.powercap_limit_action,"no_action")){ 
        /* Format is: command current_power current_limit total_idle_nodes allocated_idle_power*/
        sprintf(cmd,"%s %u %u %u %u", my_cluster_conf.eargm.powercap_limit_action,my_cluster_power_status->current_power,current_cluster_powercap,my_cluster_power_status->idle_nodes,my_cluster_power_status->total_idle_power);
        verbose(VGM_PC,"%sExecuting powercap suspend action: %s%s",COL_RED,cmd,COL_CLR);
        execute_with_fork(cmd);
        pthread_mutex_lock(&plocks);
        process_created++;
        pthread_mutex_unlock(&plocks);
        actions_executed = 1;
    } else {
        verbose(VGM_PC, "No action for powercap limit");
    }
}

void check_powercap_actions(uint cluster_powercap)
{
    if (cluster_powercap >= ((float)(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_limit)/100.0) && !actions_executed)
    {   
        execute_powercap_limit_action();
    }   
    else if (cluster_powercap <= ((float)(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_lower)/100.0) && actions_executed)
    {   
        execute_powercap_lower_action();
    }   
}



void write_shared_data(ulong current_power, ulong freeable_power, ulong requested)
{
    pthread_mutex_lock(&ext_mutex);
    memset(&ext_powercap_data, 0, sizeof(shared_powercap_data_t));
    ext_powercap_data.status = PC_STATUS_OK;
    if (requested > 0 && default_cluster_powercap > current_cluster_powercap)
    {
        if (default_cluster_powercap < (current_cluster_powercap + requested)) {
            requested = default_cluster_powercap - current_cluster_powercap;
        }
        freeable_power = 0; //if pc_status_ask_def and requesting we cannot free
        ext_powercap_data.status = PC_STATUS_ASK_DEF;
    }
    else if (requested) {
        freeable_power = 0; //if greedy and requesting, we cannot free more than our extra_power
        ext_powercap_data.status = PC_STATUS_GREEDY;
    }

    ext_powercap_data.requested = requested; 
    ext_powercap_data.current_power = current_power;
    ext_powercap_data.extra_power = current_extra_power;
    ext_powercap_data.available_power = freeable_power;
    ext_powercap_data.current_powercap = current_cluster_powercap;
    ext_powercap_data.def_power = default_cluster_powercap;
    debug("Written shared data with current power %lu and default power %lu", current_cluster_powercap, default_cluster_powercap);
    pthread_mutex_unlock(&ext_mutex);
}

void cluster_power_monitor()
{
    debug("%sGlobal POWER monitoring INIT----%s",COL_BLU,COL_CLR);
    /* We use this function for now, we must use a new light one */
    num_power_status = ear_get_powercap_status(&my_cluster_conf, &my_cluster_power_status, 1, nodes, num_eargm_nodes);
    if (num_power_status==0){
        verbose(VGM_PC+1,"num_power_status in cluster_check_powercap is 0");
        write_shared_data(0, 0, 0);
        return;
    }
    write_shared_data(my_cluster_power_status->current_power, 0, 0);
	eargm_report_event(CLUSTER_POWER, (ulong)my_cluster_power_status->current_power);
    if (my_cluster_power_status->current_power >= ((float)(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_limit)/100.0) && !actions_executed){
        execute_powercap_limit_action();
    } else if (my_cluster_power_status->current_power <= ((float)(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_lower)/100.0) && actions_executed) {
        execute_powercap_lower_action();
    } else {
        verbose(VGM_PC+1,"Total power %u limit for action %lu",my_cluster_power_status->current_power,(current_cluster_powercap*my_cluster_conf.eargm.defcon_power_limit)/100);
    }
    free(my_cluster_power_status);
    debug("%sGlobal POWER monitoring END-----%s",COL_BLU,COL_CLR);

}

#define CLUSTER_PC_TO_SHARE 0.5

/* Cluster HARD powercap */
void cluster_check_powercap()
{
    debug("%sSTART cluster_check_powercap---------%s",COL_BLU,COL_CLR);
    num_power_status = ear_get_powercap_status(&my_cluster_conf, &my_cluster_power_status, 1, nodes, num_eargm_nodes);
    if (num_power_status==0){
        verbose(VGM_PC+1,"num_power_status in cluster_check_powercap is 0");
        write_shared_data(0, 0, 0); 
        return;
    }
    memset(&cluster_options,0,sizeof(powercap_opt_t));
    must_send_pc_options=0;
	eargm_report_event(CLUSTER_POWER, (ulong)my_cluster_power_status->current_power);
    print_cluster_power_status(my_cluster_power_status);
    aggregate_data(my_cluster_power_status);
    if (powercap_reallocation(my_cluster_power_status, &cluster_options) == 0){
        pc_release_data_t rel_power;
        if (ear_cluster_release_idle_power(&my_cluster_conf,&rel_power)==0){
            error("Error in cluster_release_idle_power");
        } else{
            verbose(VGM_PC,"%s%u Watts from idle nodes released%s",COL_GRE,rel_power.released,COL_CLR);
        }
        my_cluster_power_status->released = 0;
        my_cluster_power_status->total_powercap = my_cluster_power_status->total_powercap - rel_power.released;
        powercap_reallocation(my_cluster_power_status,&cluster_options);
    }
    uint power_to_free = ear_min(current_cluster_powercap - min_cluster_powercap, total_free + my_cluster_power_status->released * CLUSTER_PC_TO_SHARE);
    uint power_to_request = 0;
    if (total_req_greedy > greedy_allocated) //to prevent overflow
        power_to_request = total_req_greedy - greedy_allocated;


    verbose(VGM_PC, "current powercap %u total_req_greedy %u and greedy_allocated %u", current_cluster_powercap,
                                                                            total_req_greedy, greedy_allocated);
    write_shared_data(my_cluster_power_status->current_power, power_to_free, power_to_request); //TODO: fill the extra power and freeable_power

    if (must_send_pc_options){
        print_powercap_opt(&cluster_options);
        send_powercap_options_to_cluster(&cluster_options);
    }else{
        verbose(VGM_PC+1,"There is no need to send the pc_options");
    }

    check_powercap_actions(my_cluster_power_status->total_powercap);

    free(my_cluster_power_status);
    free(cluster_options.greedy_nodes);
    free(cluster_options.extra_power);
    debug("%sEND cluster_check_powercap----------%s",COL_BLU,COL_CLR);
}
