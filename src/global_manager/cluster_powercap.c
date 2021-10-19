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

#define _GNU_SOURCE 

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <common/config.h>
#include <common/system/execute.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/colors.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/remote_api/eard_rapi.h>
#include <global_manager/cluster_powercap.h>


#if POWERCAP

extern cluster_conf_t my_cluster_conf;
extern uint  max_cluster_power;
extern uint  cluster_powercap_period;

extern uint process_created;
extern pthread_mutex_t plocks;
void check_pending_processes();


uint num_nodes;
uint actions_executed = 0;
uint total_req_new,total_req_greedy,req_no_extra,num_no_extra,num_greedy,num_extra,extra_power_alloc,total_extra_power;
cluster_powercap_status_t *my_cluster_power_status;
int num_power_status;
powercap_opt_t cluster_options;
extern uint policy;
static uint must_send_pc_options;
#define min(a,b) (a<b?a:b)

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
    debug("%s",COL_GRE);
    debug("num_greedy %u",opt->num_greedy);
    for (i=0;i<opt->num_greedy;i++){
        debug("greedy_node %d extra power %d",opt->greedy_nodes[i],opt->extra_power[i]);
    }
    debug("Cluster power allocation percentage %u",opt->cluster_perc_power);
    debug("%s",COL_CLR);
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
    verbose(0,"Total extra power for nodes with NO current extra power %u W, %u nodes",req_no_extra,num_no_extra);
    verbose(0,"Nodes with extra power already allocated %u",num_greedy);
    if (num_no_extra>0){
        if (req_no_extra<pending) more_power=-1;
        else more_power=pending/num_no_extra;
        verbose(0,"STAGE_1-Allocating %d watts to new greedy nodes first (-1 => alloc=req)",more_power);
        for (i=0;i<cluster_status->num_greedy;i++){
            if ((cluster_status->greedy_data[i].requested) && (!cluster_status->greedy_data[i].extra_power)){
                if (more_power<0) cluster_options->extra_power[i]=cluster_status->greedy_data[i].requested;
                else cluster_options->extra_power[i]=min(more_power,cluster_status->greedy_data[i].requested);
                pending-=cluster_options->extra_power[i];
            }
        }
    }
    /* If there is pending power to allocate */
    verbose(0,"STAGE-2 allocating %uW to the rest of greedy nodes",pending);
    if (pending){
        verbose(0,"Allocating %u watts to greedy nodes ",pending);
        more_power=pending/num_greedy;
        for (i=0;i<cluster_status->num_greedy;i++){
            if ((cluster_status->greedy_data[i].requested) && (!cluster_options->extra_power[i])){
                cluster_options->extra_power[i]=min(cluster_status->greedy_data[i].requested,more_power);
                pending-=cluster_options->extra_power[i];
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
    verbose(0,"SEQ reduce_allocation implementation avg red=%uW",red_node);
    debug("ROUND 1- reducing avg power ");
    while((min_reduction>0) && (i<cs->num_greedy)){
        if (cs->greedy_data[i].extra_power){
            red1=min(red_node,cs->greedy_data[i].extra_power);
            red=min(red1,min_reduction);
            new_extra=-red;
            verbose(0,"%sreducing %d W to node %d %s",COL_RED,new_extra,i,COL_CLR);
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
            verbose(0,"%sreducing %d W to node %d %s",COL_RED,new_extra,i,COL_CLR);
            cluster_options->extra_power[i]+=new_extra;
            min_reduction-=red;
        }
        i++;
    }

}

/* This function is executed when there is enough power for new running nodes but not for all the greedy nodes */
uint powercap_reallocation(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint released)
{
    uint total_free;
		int i;
    uint num_nodes=cluster_status->total_nodes;
    uint min_reduction;
    verbose(0,"There are %u nodes  %u idle nodes %u greedy nodes ", num_nodes,cluster_status->idle_nodes,cluster_status->num_greedy);
    if (cluster_options->greedy_nodes == NULL) {
        cluster_options->greedy_nodes=calloc(cluster_status->num_greedy,sizeof(int));
        cluster_options->extra_power=calloc(cluster_status->num_greedy,sizeof(uint));
    }
    cluster_options->num_greedy=cluster_status->num_greedy;
    cluster_options->cluster_perc_power = (cluster_status->total_powercap * 100)/max_cluster_power;
    memcpy(cluster_options->greedy_nodes,cluster_status->greedy_nodes,cluster_status->num_greedy*sizeof(int));
    total_free=max_cluster_power-cluster_status->total_powercap;
    verbose(0,"Total power %u , requested for new %u (potential) released %u extra_req %u extra_used %u",max_cluster_power,cluster_status->requested,cluster_status->released,total_req_greedy,total_extra_power);
    verbose(0,"Free power before reallocation %d",total_free);
    if (cluster_status->requested) must_send_pc_options=1;
    /* Allocated power + default requested must be less that maximum */
    if ((cluster_status->total_powercap+cluster_status->requested)<=max_cluster_power){
        verbose(0,"There is enough power for new running jobs");
        total_free=max_cluster_power-(cluster_status->total_powercap+cluster_status->requested);
        verbose(0,"Free power after allocating power to new jobs %d",total_free);
        if (total_req_greedy==0){
            return 1;
        }
        /* At this point we know there is greedy power requested */
        if (total_req_greedy<=total_free) {
            verbose(0,"There is more free power than requested by greedy nodes=> allocating all the req power");
            total_free-=total_req_greedy;
            for (i = 0; i < cluster_status->num_greedy; i++)
                cluster_options->extra_power[i] = cluster_status->greedy_data[i].requested;
            must_send_pc_options=1;
        }else{
            verbose(0,"There is not enough power for all the greedy nodes (free %d req %u)(used %u allocated %u)",total_free,total_req_greedy,cluster_status->current_power,cluster_status->total_powercap);
            if (cluster_status->released==0 ){
                verbose(0,"Anyway there is not enough power ");
                cluster_options->num_greedy=cluster_status->num_greedy;
                allocate_free_power_to_greedy_nodes(cluster_status,cluster_options,(uint *)&total_free);
            }else if (cluster_status->released) return 0;
        }
    }else{
        /* There is not enough power for new jobs, we must reduce the extra allocation */
        if (cluster_status->released==0){
            verbose(0,"We must reduce the extra allocation (used %u allocated %u)",cluster_status->current_power,cluster_status->total_powercap);
            min_reduction=(cluster_status->total_powercap+cluster_status->requested)-max_cluster_power;
            total_free=0;
            reduce_allocation(cluster_status,cluster_options,min_reduction);
            must_send_pc_options=1;
        }else if (cluster_status->released) return 0;
    }
    return 1;
}

void send_powercap_options_to_cluster(powercap_opt_t *cluster_options)
{
    cluster_set_powercap_opt(&my_cluster_conf,cluster_options);
}


void print_cluster_power_status(powercap_status_t *my_cluster_power_status)
{
    int i;
    powercap_status_t *cs=my_cluster_power_status;
    if (cs==NULL){
        debug("cluster status is NULL");
        return;
    }
    debug("Total %u Idle  %u power (released %u requested %u consumed %u allocated %u allocated_to_idle %u) total_greedy_nodes %u",cs->total_nodes,cs->idle_nodes, cs->released,cs->requested,cs->current_power,cs->total_powercap,cs->num_greedy,cs->total_idle_power);
    debug("%d power_status received",num_power_status);
    for (i=0;i<cs->num_greedy;i++){
        debug("\t[%d] ip %d greedy_req %u extra_power %u", i, cs->greedy_nodes[i], cs->greedy_data[i].requested, cs->greedy_data[i].extra_power);
    }
}

pthread_t cluster_powercap_th;

void * eargm_powercap_th(void *noarg)
{
    if (pthread_setname_np(pthread_self(), "cluster_powercap")!=0) error("Setting name forcluster_powercap thread %s", strerror(errno));

#if POWERCAP
    while(1)
    {
        sleep(cluster_powercap_period);
        /* we check if there are zombie processes */
        if (process_created){
            check_pending_processes();
        }
        if (cluster_power_limited()){
            if (my_cluster_conf.eargm.powercap_mode){
                cluster_check_powercap();
            }else{
                cluster_power_monitor();
            }
        }

    }
#endif

}

void cluster_powercap_init(cluster_conf_t *cc)
{
    int ret;
    if (max_cluster_power>0){
        verbose(0,"Power cap limit set to %u",max_cluster_power);
    }else{
        verbose(0,"Power cap unlimited");
    }

    if (max_cluster_power==0) return;

    /* This thread accepts external commands */
    if ((ret=pthread_create(&cluster_powercap_th, NULL, eargm_powercap_th, NULL))){
        errno=ret;
        error("error creating eargm_server for external api %s",strerror(errno));
    }
}

int cluster_power_limited()
{
    return (max_cluster_power);
}

/* This function is executed when we are in a good position after executing the higher limit action*/
void execute_powercap_lower_action()
{
    char cmd[300];
    /* If action is different than no_action we run the command */
    if (strcmp(my_cluster_conf.eargm.powercap_lower_action,"no_action")){ 
        /* Format is: command current_power current_limit total_idle_nodes allocated_idle_power*/
        sprintf(cmd,"%s %u %u %u %u", my_cluster_conf.eargm.powercap_lower_action,my_cluster_power_status->current_power,max_cluster_power,my_cluster_power_status->idle_nodes,my_cluster_power_status->total_idle_power);
        verbose(0,"%sExecuting powercap resume action: %s %s",COL_GRE,cmd,COL_CLR);
        execute_with_fork(cmd);
        pthread_mutex_lock(&plocks);
        process_created++;
        pthread_mutex_unlock(&plocks);
        actions_executed = 0;
    } else {
        verbose(0, "No action for powercap lower limit");
    }
}

/* This function is executed when we are close to the limit , consumed or allocated, depending of powercap_mode*/
void execute_powercap_limit_action()
{
    char cmd[300];
    /* If action is different than no_action we run the command */
    if (strcmp(my_cluster_conf.eargm.powercap_limit_action,"no_action")){ 
        /* Format is: command current_power current_limit total_idle_nodes allocated_idle_power*/
        sprintf(cmd,"%s %u %u %u %u", my_cluster_conf.eargm.powercap_limit_action,my_cluster_power_status->current_power,max_cluster_power,my_cluster_power_status->idle_nodes,my_cluster_power_status->total_idle_power);
        verbose(0,"%sExecuting powercap suspend action: %s%s",COL_RED,cmd,COL_CLR);
        execute_with_fork(cmd);
        pthread_mutex_lock(&plocks);
        process_created++;
        pthread_mutex_unlock(&plocks);
        actions_executed = 1;
    } else {
        verbose(0, "No action for powercap limit");
    }
}

void cluster_power_monitor()
{
    debug("%sGlobal POWER monitoring INIT----%s",COL_BLU,COL_CLR);
    /* We use this function for now, we must use a new light one */
    num_power_status=cluster_get_powercap_status(&my_cluster_conf,&my_cluster_power_status);
    if (num_power_status==0){
        verbose(1,"num_power_status in cluster_check_powercap is 0");
        return;
    }
    if (my_cluster_power_status->current_power >= ((float)(max_cluster_power*my_cluster_conf.eargm.defcon_power_limit)/100.0) && !actions_executed){
        execute_powercap_limit_action();
    } else if (my_cluster_power_status->current_power <= ((float)(max_cluster_power*my_cluster_conf.eargm.defcon_power_lower)/100.0) && actions_executed) {
        execute_powercap_lower_action();
    } else {
        verbose(1,"Total power %u limit for action %lu",my_cluster_power_status->current_power,(max_cluster_power*my_cluster_conf.eargm.defcon_power_limit)/100);
    }
    free(my_cluster_power_status);
    debug("%sGlobal POWER monitoring END-----%s",COL_BLU,COL_CLR);

}


void cluster_check_powercap()
{
    debug("%sSTART cluster_check_powercap---------%s",COL_BLU,COL_CLR);
    num_power_status=cluster_get_powercap_status(&my_cluster_conf,&my_cluster_power_status);
    if (num_power_status==0){
        verbose(1,"num_power_status in cluster_check_powercap is 0");
        return;
    }
    memset(&cluster_options, 0, sizeof(powercap_opt_t));
    must_send_pc_options=0;
    print_cluster_power_status(my_cluster_power_status);
    aggregate_data(my_cluster_power_status);
    if (powercap_reallocation(my_cluster_power_status,&cluster_options,0)==0){
        pc_release_data_t rel_power;
        if (cluster_release_idle_power(&my_cluster_conf,&rel_power)==0){
            error("Error in cluster_release_idle_power");
        } else{
            verbose(0,"%s%u Watts from idle nodes released%s",COL_GRE,rel_power.released,COL_CLR);
        }
        my_cluster_power_status->released = 0;
        my_cluster_power_status->total_powercap = my_cluster_power_status->total_powercap - rel_power.released;
        powercap_reallocation(my_cluster_power_status,&cluster_options,1);
    }
    if (must_send_pc_options){
        print_powercap_opt(&cluster_options);
        send_powercap_options_to_cluster(&cluster_options);
    }else{
        verbose(1,"There is no need to send the pc_options");
    }

    if (my_cluster_power_status->total_powercap>=((float)(max_cluster_power*my_cluster_conf.eargm.defcon_power_limit)/100.0) && !actions_executed)
    {
        execute_powercap_limit_action();
    }
    else if (my_cluster_power_status->total_powercap<=((float)(max_cluster_power*my_cluster_conf.eargm.defcon_power_lower)/100.0) && actions_executed)
    {
        execute_powercap_lower_action();
    }

    free(my_cluster_power_status);
    free(cluster_options.greedy_nodes);
    free(cluster_options.extra_power);
    debug("%sEND cluster_check_powercap----------%s",COL_BLU,COL_CLR);
}
#endif
