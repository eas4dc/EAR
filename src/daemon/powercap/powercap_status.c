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
#include <management/cpufreq/frequency.h>
#include <common/states.h>
#include <daemon/powercap/powercap.h>
#include <common/types/pc_app_info.h>

/* This function decides if we have to changed the utilization or not */
#define MAX_ALLOWED_DIFF 0.25
uint util_changed(ulong curr,ulong prev)
{
  float diff;
  int idiff,icurr,iprev;
  icurr=(int)curr;
  iprev=(int)prev;
  if ((prev == 0) && (curr)) return 1;
  if ((curr == 0) && (prev)) return 1;
  if ((!prev) && (!curr)) return 0;
  idiff = (icurr - iprev);
  if (idiff < 0){
    idiff = idiff * -1;
    diff = (float)idiff / (float)iprev;
  }else{
    diff = (float)idiff / (float)icurr;
  }
  // debug("Current util and prev util differs in %.3f prev %lu curr %lu",diff,prev,curr);
  return (diff >  MAX_ALLOWED_DIFF);
}


uint compute_power_to_release(node_powercap_opt_t *pc_opt,uint current)
{
	return pc_opt->th_release;
}

uint limit_max_power(node_powercap_opt_t *pc_opt,uint extra)
{
	uint current=pc_opt->last_t1_allocated;
	if ((current+extra) > pc_opt->max_node_power){
		extra=pc_opt->max_node_power-pc_opt->last_t1_allocated;
	}
	return extra;
}

uint compute_extra_gpu_power(uint current,uint diff,uint target)
{
	/* This function mut be improved */
	return 10;
}

uint more_power(node_powercap_opt_t *pc_opt,uint current)
{
	int dist;
  dist=(int)pc_opt->current_pc-(int)current;
	debug("More Power check:Computed distance %d",dist);
	if (dist<0) return 1;
	if (dist<pc_opt->th_inc) return 1;
	return 0;
}
uint free_power(node_powercap_opt_t *pc_opt,uint current)
{
	int dist;
  dist=(int)pc_opt->current_pc-(int)current;
	debug("Free Power check:Computed distance %d",dist);
	if (dist<0) return 0;
	if (dist>=pc_opt->th_red) return 1;
	return 0;
}
uint ok_power(node_powercap_opt_t *pc_opt,uint current)
{
	int dist;
	dist=(int)pc_opt->current_pc-(int)current;
	debug("OK Power check:Computed distance %d",dist);
	if (dist<0) return 0;
	if ((dist<pc_opt->th_red) && (dist>pc_opt->th_inc)) return 1;
	return 0;
}

int is_powercap_set(node_powercap_opt_t *pc_opt)
{ 
  /* 0 means unlimited */
  /* we are not checking hw configuration in this function */
  return (pc_opt->current_pc!=0);
}

int is_powercap_on(node_powercap_opt_t *pc_opt)
{
  return ((pc_opt->powercap_status!=PC_STATUS_ERROR) && is_powercap_set(pc_opt));
}

uint get_powercapopt_value(node_powercap_opt_t *pc_opt)
{
  /* we are not checking hw configuration in this function */
  return pc_opt->current_pc;
}

uint get_powercap_allocated(node_powercap_opt_t *pc_opt)
{
  /* we are not checking hw configuration in this function */
  return pc_opt->last_t1_allocated;
}

/* Returns the stress in the subsystem. The stress level represents the
 * percentual distance between the current freq and the target freq,
 * with 0 being functioning as expected and 100 being the worst case. 
 * The calculation is:
 *      (target-current)/(max-min) * 100. 
 * 
 * If the max_freq and min_freq are the same (error), 0 is returned 
 * as the distance cannot be computed. */
uint powercap_get_stress(uint max_freq, uint min_freq, uint t_freq, uint c_freq) 
{
    debug("getting stress with %u(max_freq) %u(min_freq) %u(t_freq) and %u(c_freq)", max_freq, min_freq, t_freq, c_freq);
    uint div = max_freq - min_freq;
    if (div > 0)
        return (100*(t_freq - c_freq))/div;

    return 0;
}

uint compute_power_status(node_powercap_opt_t *pc,uint current_power)
{
	if (pc->last_t1_allocated<pc->def_powercap) return PC_STATUS_ASK_DEF;
  if (ok_power(pc,current_power)) return PC_STATUS_OK;
  if (more_power(pc,current_power)) return PC_STATUS_GREEDY;
  if (free_power(pc,current_power)) return PC_STATUS_RELEASE;
	return PC_STATUS_OK;
}


uint compute_next_status(node_powercap_opt_t *pc,uint current_power,ulong eff_f, ulong req_f)
{
	uint pstate=compute_power_status(pc,current_power);
	switch (pstate){
		case PC_STATUS_RELEASE:if (eff_f==req_f) return PC_STATUS_RELEASE; break;
		case PC_STATUS_GREEDY:if (eff_f<req_f) return PC_STATUS_GREEDY; break;
		case PC_STATUS_OK:if (eff_f< req_f) return PC_STATUS_GREEDY;break;
		default: return pstate;
	}
	return PC_STATUS_OK;
}

void powercap_status_to_str(uint s,char *b)
{	
	switch (s){
	case PC_STATUS_OK:sprintf(b,"OK");break;
	case PC_STATUS_GREEDY:sprintf(b,"greedy");break;
	case PC_STATUS_RELEASE:sprintf(b,"release");break;
	case PC_STATUS_ASK_DEF:sprintf(b,"ask_def");break;
	case PC_STATUS_IDLE:sprintf(b,"idle");break;
	case PC_STATUS_STOP:sprintf(b,"stop");break;
	case PC_STATUS_START:sprintf(b,"start");break;
	case PC_STATUS_RUN:sprintf(b,"run");break;
	default:	sprintf(b,"undef");break;
	}
}
