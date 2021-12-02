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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <common/config.h>
#include <common/colors.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/execute.h>
#include <common/system/monitor.h>
#include <common/messaging/msg_conf.h>
#include <common/types/log_eard.h>
#include <common/types/configuration/cluster_conf.h>

#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/shared_configuration.h>

#define POWERCAP_MON 1
#define MAX_PERC_POWER 90



node_powercap_opt_t my_pc_opt;
static int my_ip;
extern int *ips;
extern int self_id;
extern volatile int init_ips_ready;
int last_status;
int fd_powercap_values=0;
static pwr_mgt_t *pcmgr;
static uint pc_pid=0;
static uint pc_status_config=AUTO_CONFIG;
static uint current_power=0;

pthread_t powercapmon_th;
unsigned long powercapmon_freq=1;
extern int eard_must_exit;
extern settings_conf_t *dyn_conf;
extern resched_t *resched_conf;
static uint pc_cpu_strategy;
static uint8_t last_cluster_perc = 0;
extern pc_app_info_t *pc_app_info_data;

#if POWERCAP_MON
static suscription_t *sus_powercap_monitor;
#endif


void get_date_str(char *msg,int size)
{
	struct tm *current_t;
  time_t rawtime;
	time(&rawtime);
	current_t = localtime(&rawtime);
  strftime(msg, size, "%c", current_t);
}

/***** These two functions monitors node power for status update *****/
state_t pc_monitor_thread_init(void *p)
{
	return EAR_SUCCESS;
}
state_t pc_monitor_thread_main(void *p)
{
	return EAR_SUCCESS;
}

void  powercap_monitor_init()
{
	#if POWERCAP_MON
  sus_powercap_monitor=suscription();
	sus_powercap_monitor->call_main = pc_monitor_thread_main;
	sus_powercap_monitor->call_init = pc_monitor_thread_init;
	sus_powercap_monitor->time_relax = 1000;
	sus_powercap_monitor->time_burst = 1000;
	sus_powercap_monitor->suscribe(sus_powercap_monitor);
  #endif
}



void update_node_powercap_opt_shared_info()
{
	memcpy(&dyn_conf->pc_opt,&my_pc_opt,sizeof(node_powercap_opt_t));
	resched_conf->force_rescheduling=1;
}


void print_node_powercap_opt(node_powercap_opt_t *my_powercap_opt)
{
	fprintf(stderr,"cuurent %u pc_def %u pc_idle %u th_inc %u th_red %u th_release %u cluster_perc_power %u requested %u released %u\n",
	my_powercap_opt->current_pc, my_powercap_opt->def_powercap, my_powercap_opt->powercap_idle, my_powercap_opt->th_inc, my_powercap_opt->th_red,
            my_powercap_opt->th_release,my_powercap_opt->cluster_perc_power, my_powercap_opt->requested,my_powercap_opt->released);
}

uint powercat_get_value()
{
	return my_pc_opt.current_pc;
}

static int set_powercap_value(uint domain,uint limit)
{
  char c_date[128];
  if (limit==my_pc_opt.current_pc) return EAR_SUCCESS;
  verbose(0,"%spowercap_set_powercap_value domain %u limit %u%s",COL_BLU,domain,limit,COL_CLR);
  get_date_str(c_date,sizeof(c_date));
  if (fd_powercap_values>=0){
    dprintf(fd_powercap_values,"%s domain %u limit %u \n",c_date,domain,limit);
  }
  my_pc_opt.current_pc = limit;
  update_node_powercap_opt_shared_info();
  pmgt_set_app_req_freq(pcmgr,pc_app_info_data);
	dyn_conf->pc_opt.current_pc = powercat_get_value();

  return pmgt_set_powercap_value(pcmgr,pc_pid,domain,(ulong)limit);
}


void set_default_node_powercap_opt(node_powercap_opt_t *my_powercap_opt)
{
	my_powercap_opt->def_powercap = powermon_get_powercap_def();
	my_powercap_opt->powercap_idle = powermon_get_powercap_def()*POWERCAP_IDLE_PERC;
	my_powercap_opt->current_pc = 0;
	my_powercap_opt->last_t1_allocated = powermon_get_powercap_def();
	my_powercap_opt->max_node_power = powermon_get_max_powercap_def();
	my_powercap_opt->released = my_powercap_opt->last_t1_allocated-my_powercap_opt->powercap_idle;
	my_powercap_opt->th_inc = 10;
	my_powercap_opt->th_red = 50;
	my_powercap_opt->th_release = 25;
	my_powercap_opt->powercap_status = PC_STATUS_ERROR;
	my_powercap_opt->cluster_perc_power = 0;
	my_powercap_opt->requested = 0;
	pthread_mutex_init(&my_powercap_opt->lock,NULL);
}

void powercap_end()
{ 
	if (pmgt_disable(pcmgr)!=EAR_SUCCESS){
		error("pmgt_disable");
	}

}

int powercap_init()
{
	debug("powercap init");
	set_default_node_powercap_opt(&my_pc_opt);
	print_node_powercap_opt(&my_pc_opt);
	/* powercap set to 0 means unlimited */
	if (powermon_get_powercap_def()==0){ 
		debug("POWERCAP limit disabled");
		update_node_powercap_opt_shared_info();
		return EAR_SUCCESS;
	}
	while(init_ips_ready==0){ 
		sleep(1);
	}
	if (init_ips_ready>0) my_ip=ips[self_id];
	else my_ip=0;

	/* Low level power cap managemen initialization */
	if (pmgt_init()!=EAR_SUCCESS){
		error("Low level power capping management error");
		return EAR_ERROR;
	}
	if (pmgt_handler_alloc(&pcmgr)!=EAR_SUCCESS){
		error("Allocating memory for powercap handler");
		return EAR_ERROR;
	}
	if (pmgt_enable(pcmgr)!=EAR_SUCCESS){
		error("Initializing powercap manager");
		return EAR_ERROR;
	}

	/* End Low level power cap managemen initialization */
	my_pc_opt.powercap_status=PC_STATUS_IDLE;
	last_status=PC_STATUS_IDLE;
	pc_cpu_strategy=pmgt_get_powercap_cpu_strategy(pcmgr);
	pmgt_set_pc_mode(pcmgr,PC_MODE_TARGET);
	set_powercap_value(DOMAIN_NODE,my_pc_opt.powercap_idle);
	debug("powercap initialization finished");
	powercap_monitor_init();
	update_node_powercap_opt_shared_info();
	return EAR_SUCCESS;	
}



int powercap_idle_to_run()
{
	uint extra;
	if (!is_powercap_on(&my_pc_opt)) return EAR_SUCCESS;
	debug("powercap_idle_to_run");
	while(pthread_mutex_trylock(&my_pc_opt.lock)); /* can we create some deadlock because of status ? */
	pmgt_set_status(pcmgr,PC_STATUS_RUN);
	//debug("pc status modified");
	last_status=PC_STATUS_IDLE;
	extra=0;
	switch(my_pc_opt.powercap_status){
		case PC_STATUS_IDLE:
		debug("%sGoin from idle to run:allocated %u %s ",COL_GRE,my_pc_opt.last_t1_allocated,COL_CLR);
		/* There is enough power for me */
		if ((my_pc_opt.last_t1_allocated+extra)>=my_pc_opt.def_powercap){	
			/* if we already had de power, we just set the status as OK */
			if (my_pc_opt.last_t1_allocated>=my_pc_opt.def_powercap){
				my_pc_opt.powercap_status=PC_STATUS_OK;
				my_pc_opt.released=0;
				set_powercap_value(DOMAIN_NODE,my_pc_opt.last_t1_allocated);	
			}else{
				/* We must use extra power */
				my_pc_opt.last_t1_allocated=ear_min(my_pc_opt.def_powercap,my_pc_opt.last_t1_allocated+extra);
				my_pc_opt.powercap_status=PC_STATUS_OK;
				my_pc_opt.released=0;
				set_powercap_value(DOMAIN_NODE,my_pc_opt.last_t1_allocated);	
			}	
		}else{ /* we must ask more power */
				uint pending;
				my_pc_opt.last_t1_allocated+=extra;
				my_pc_opt.powercap_status=PC_STATUS_ASK_DEF;
				pending=my_pc_opt.def_powercap-my_pc_opt.last_t1_allocated;
				my_pc_opt.requested=pending;
				set_powercap_value(DOMAIN_NODE,my_pc_opt.last_t1_allocated);	
		}
		break;
    case PC_STATUS_OK:
    case PC_STATUS_GREEDY:
    case PC_STATUS_RELEASE:
    case PC_STATUS_ASK_DEF:
		error("We go to run and we were not in idle ");
		break;
	}
	pmgt_idle_to_run(pcmgr);
	pthread_mutex_unlock(&my_pc_opt.lock);
	return EAR_SUCCESS;
}

int powercap_run_to_idle()
{
    if (!is_powercap_on(&my_pc_opt)) return EAR_SUCCESS;
    debug("powercap_run_to_idle");
    while(pthread_mutex_trylock(&my_pc_opt.lock)); 
    switch(my_pc_opt.powercap_status){
        case PC_STATUS_IDLE:
            error("going from run to idle and we were in idle");
            break;
        case PC_STATUS_OK:
        case PC_STATUS_GREEDY:
        case PC_STATUS_ASK_DEF:
        case PC_STATUS_RELEASE:
            debug("%sGoing from run to idle%s",COL_GRE,COL_CLR);
            my_pc_opt.released=my_pc_opt.last_t1_allocated-my_pc_opt.powercap_idle;
            my_pc_opt.requested=0;
            my_pc_opt.powercap_status=PC_STATUS_IDLE;
            set_powercap_value(DOMAIN_NODE,my_pc_opt.powercap_idle);
            break;
    }
    pmgt_set_status(pcmgr,PC_STATUS_IDLE);
    pmgt_run_to_idle(pcmgr);
    pthread_mutex_unlock(&my_pc_opt.lock);
    if (last_cluster_perc > MAX_PERC_POWER) {
        debug("resetting powercap due to power usage (%u)", last_cluster_perc);
        powercap_reset_default_power();
    }
    return EAR_SUCCESS;
}

/***************************************************************************************/
/********** Executed each time a new periodic metric is ready **************************/
/***************************************************************************************/


/* To consider, should we differentiate the monitoring frequency vs the DB monitoring */
int powercap_set_power_per_domain(dom_power_t *cp,uint use_earl,ulong avg_f)
{
	current_power=(uint)cp->platform;
	if (!is_powercap_on(&my_pc_opt)) return EAR_SUCCESS;
	debug("periodic_metric_info");
	while(pthread_mutex_trylock(&my_pc_opt.lock));
	if (my_pc_opt.powercap_status == PC_STATUS_IDLE) pmgt_set_power_per_domain(pcmgr,cp,PC_STATUS_IDLE);
	else pmgt_set_power_per_domain(pcmgr,cp,PC_STATUS_RUN);
	powercap_set_app_req_freq(pc_app_info_data);
	if (current_power > my_pc_opt.current_pc){
		debug("%s",COL_RED);
	}else{
		debug("%s",COL_GRE);
	}
	debug_pc_app_info(pc_app_info_data);
	debug("PM event, current power %u powercap %u allocated %u status %u released %u requested %u",\
		current_power,my_pc_opt.current_pc,my_pc_opt.last_t1_allocated,my_pc_opt.powercap_status,my_pc_opt.released,\
		my_pc_opt.requested);
	debug("%s",COL_CLR);
	pthread_mutex_unlock(&my_pc_opt.lock);
	return EAR_SUCCESS;
}

void print_power_status(powercap_status_t *my_status)
{
	int i;
	debug("Power_status:Ilde %u released %u requested %u total greedy %u  current power %u total power cap %u total_idle_power %u",
	my_status->idle_nodes,my_status->released,my_status->requested,my_status->num_greedy,my_status->current_power,
	my_status->total_powercap,my_status->total_idle_power);
	for (i=0;i<my_status->num_greedy;i++){
		if (my_status->num_greedy) debug("greedy=(ip=%u,req=%u,extra=%u) ", my_status->greedy_nodes[i], 
                                                my_status->greedy_data[i].requested, my_status->greedy_data[i].extra_power);
	}
}
/***************************************************************************************/
/**********  This function is executed under EARGM request    **************************/
/***************************************************************************************/

void powercap_release_power()
{
    my_pc_opt.powercap_status = PC_STATUS_OK;
    my_pc_opt.released = 0;
    my_pc_opt.last_t1_allocated = my_pc_opt.current_pc;
	pmgt_set_powercap_value(pcmgr, pc_pid, DOMAIN_NODE, (ulong)my_pc_opt.current_pc);
}

void powercap_get_status(powercap_status_t *my_status, pmgt_status_t *status)
{
    debug("powercap_get_status: get_powercap_status_last_t1 %u def_pc %u",my_pc_opt.last_t1_allocated,my_pc_opt.def_powercap);
    while(pthread_mutex_trylock(&my_pc_opt.lock)); 
    my_status->total_nodes++;
    if (my_pc_opt.powercap_status == PC_STATUS_IDLE)
    {
        status->status = PC_STATUS_IDLE;
        my_status->released += my_pc_opt.released;
        my_status->total_idle_power += my_pc_opt.last_t1_allocated;
        my_status->idle_nodes++;
    }
    else {
        pmgt_get_status(status);
        if (my_pc_opt.last_t1_allocated < my_pc_opt.def_powercap) status->status = PC_STATUS_ASK_DEF; 
        my_pc_opt.powercap_status = status->status;
        switch(status->status) 
        {
            case PC_STATUS_GREEDY:
                //status->requested = my_pc_opt.requested;
                if (my_pc_opt.last_t1_allocated>my_pc_opt.def_powercap) status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.def_powercap;
                else status->extra = 0;
                debug("powercap_get_status: PC_STATUS greedy, requesting %u", status->requested);
                break;
            case PC_STATUS_RELEASE:
                if (my_pc_opt.last_t1_allocated>my_pc_opt.def_powercap) status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.def_powercap;
                else status->extra = 0;
                my_pc_opt.released = status->tbr;
                my_pc_opt.current_pc -= status->tbr;
                debug("powercap_get_status: %sReleasing%s %u W allocated %u W",COL_BLU,COL_CLR,my_pc_opt.released,my_pc_opt.current_pc);
                powercap_release_power();
                break;
            case PC_STATUS_ASK_DEF: 
                /* Data management */
                debug("powercap_get_status: %sAsking for default power%s %uW allocated %uW",COL_BLU,COL_CLR,my_pc_opt.requested,my_pc_opt.last_t1_allocated);
                my_status->requested += my_pc_opt.def_powercap - my_pc_opt.last_t1_allocated;
                break;
            case PC_STATUS_OK:
                debug("powercap_get_status: PC_STATUS OK");
                status->requested = 0;
                if (my_pc_opt.last_t1_allocated>my_pc_opt.def_powercap){
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.def_powercap;
                } else {
                    status->extra = 0;
                }
                break;
            case PC_STATUS_ERROR: 
            default:
                break;

        }
    }
    ulong limit = my_pc_opt.current_pc;
    my_status->current_power += powermon_current_power();
    my_status->total_powercap += get_powercap_allocated(&my_pc_opt);
    pthread_mutex_unlock(&my_pc_opt.lock);
    powermon_report_event(POWERCAP_VALUE, limit);

}

void copy_node_powercap_opt(node_powercap_opt_t *dst)
{
	memcpy(dst,&my_pc_opt,sizeof(node_powercap_opt_t));
}

void print_powercap_opt(powercap_opt_t *opt)
{
	int i;
	debug("num_greedy %u",opt->num_greedy);	
	for (i=0;i<opt->num_greedy;i++){
		debug("greedy_node %d extra power %d",opt->greedy_nodes[i],opt->extra_power[i]);
	}
	debug("Current cluster power percentage %u",opt->cluster_perc_power);
}

/***************************************************************************************/
/**********  This function is executed when EARGM sends a message **********************/
/***************************************************************************************/


void powercap_set_opt(powercap_opt_t *opt, int id)
{
    print_powercap_opt(opt);
    if (!is_powercap_on(&my_pc_opt)) return;    
    uint event;
    ulong value;
    debug("powercap_set_opt command received");
    while(pthread_mutex_trylock(&my_pc_opt.lock)); 
    last_cluster_perc = opt->cluster_perc_power;
    my_pc_opt.cluster_perc_power = opt->cluster_perc_power;
    /* Here we must check, based on our status, if actions must be taken */
    if (id >= 0) 
    {
        switch(my_pc_opt.powercap_status){
            case PC_STATUS_IDLE:
            case PC_STATUS_OK:
                if (my_pc_opt.last_t1_allocated>my_pc_opt.def_powercap){
                    debug("%spowercap_set_opt new_pc_opt:status OK but in the greedy node list %d ip=%d with ip=%di extra %d%s",COL_RED,id,opt->greedy_nodes[id],my_ip,opt->extra_power[id],COL_CLR);
                    my_pc_opt.last_t1_allocated=my_pc_opt.last_t1_allocated+opt->extra_power[id];
                    if (opt->extra_power[id] > 0) {
                        event = INC_POWERCAP;
                        value = opt->extra_power[id];
                    } else {
                        event = RED_POWERCAP;
                        value = -opt->extra_power[id];
                    }
                    
                    set_powercap_value(DOMAIN_NODE,my_pc_opt.last_t1_allocated);
                }
                break;
            case PC_STATUS_RELEASE:
                debug("powercap_set_opt: My powercap status is RELEASE, doing nothing ");
                break;
            case PC_STATUS_GREEDY:
                debug("%spowercap_set_opt  new_pc_opt: greedy node %d ip=%d with ip=%d extra %d%s",COL_GRE,id,opt->greedy_nodes[id],my_ip,opt->extra_power[id],COL_CLR);
                if (opt->extra_power[id] > 0) {
                    event = INC_POWERCAP;
                    value = opt->extra_power[id];
                } else {
                    event = RED_POWERCAP;
                    value = -opt->extra_power[id];
                }
                my_pc_opt.last_t1_allocated=my_pc_opt.last_t1_allocated+opt->extra_power[id];
                set_powercap_value(DOMAIN_NODE,my_pc_opt.last_t1_allocated);
                my_pc_opt.powercap_status=PC_STATUS_OK;
                my_pc_opt.requested=0;
                break;
        }
    }
    else {
        if (my_pc_opt.powercap_status == PC_STATUS_ASK_DEF) {
                /* In that case, we assume we cat use the default power cap */
                debug("powercap_set_opt: My status is ASK_DEF and new settings received with new_pc: %u", my_pc_opt.def_powercap);
                my_pc_opt.powercap_status=PC_STATUS_OK;
                /* We assume we will receive the requested power */
                my_pc_opt.last_t1_allocated=my_pc_opt.def_powercap;
                my_pc_opt.requested=0;
                set_powercap_value(DOMAIN_NODE,my_pc_opt.def_powercap);            
                event = SET_ASK_DEF;
                value = 0;
        }
    }
    pthread_mutex_unlock(&my_pc_opt.lock);
    powermon_report_event(event, value);

}


void set_powercapstatus_mode(uint mode)
{
	pc_status_config=mode;
}

uint powercap_get_cpu_strategy()
{
	return pmgt_get_powercap_cpu_strategy(pcmgr);
}

uint powercap_get_gpu_strategy()
{
	return pmgt_get_powercap_gpu_strategy(pcmgr);
}

void powercap_set_app_req_freq(pc_app_info_t *pc_app)
{
	pmgt_set_app_req_freq(pcmgr,pc_app);
}

void powercap_release_idle_power(pc_release_data_t *release)
{
    uint value = 0;
    while(pthread_mutex_trylock(&my_pc_opt.lock));
    switch(my_pc_opt.powercap_status){
        case PC_STATUS_IDLE:
  	        debug("%sReleasing %u allocated IDLE power %s",COL_BLU,my_pc_opt.released,COL_CLR);
          	release->released+=my_pc_opt.released;
            value = my_pc_opt.released;
  	        my_pc_opt.released=0;
      	    my_pc_opt.last_t1_allocated=my_pc_opt.current_pc;
      	    break;
	    default:
            break;
    }
	pthread_mutex_unlock(&my_pc_opt.lock);
    powermon_report_event(RELEASE_POWER, value);
}

void powercap_reset_default_power()
{
    while(pthread_mutex_trylock(&my_pc_opt.lock));
    if (my_pc_opt.last_t1_allocated != my_pc_opt.def_powercap)
    {
        my_pc_opt.last_t1_allocated = my_pc_opt.def_powercap;
        set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
    }
	pthread_mutex_unlock(&my_pc_opt.lock);
}

void powercap_reduce_def_power(uint power)
{
	while(pthread_mutex_trylock(&my_pc_opt.lock));
	if (power > my_pc_opt.def_powercap ){
		pthread_mutex_unlock(&my_pc_opt.lock);
		return;
	}
	my_pc_opt.def_powercap -= power;
	pthread_mutex_unlock(&my_pc_opt.lock);
}

void powercap_increase_def_power(uint power)
{
  while(pthread_mutex_trylock(&my_pc_opt.lock));
  my_pc_opt.def_powercap += power;
  pthread_mutex_unlock(&my_pc_opt.lock);

}

void powercap_set_powercap(uint power)
{
	while(pthread_mutex_trylock(&my_pc_opt.lock));
	my_pc_opt.def_powercap = power;
	my_pc_opt.current_pc = power;	
	my_pc_opt.last_t1_allocated = power;	
	set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
	pthread_mutex_unlock(&my_pc_opt.lock);
}


void powercap_new_job()
{
	pmgt_new_job(pcmgr);
}
void powercap_end_job()
{
	pmgt_end_job(pcmgr);
}
