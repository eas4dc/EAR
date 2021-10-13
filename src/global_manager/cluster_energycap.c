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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <daemon/remote_api/eard_rapi.h>
#include <global_manager/cluster_energycap.h>

extern uint last_risk_sent;
extern uint default_state;
node_info_t *einfo;

int compare_node_info_lower_first(const void *a, const void *b)
{
	const node_info_t *na = (const node_info_t *)a;
	const node_info_t *nb = (const node_info_t *)b;
	/* Idle is a special case */
	if ((na->idle) && (nb->idle)) return 0;
	if (na->idle) return 1;
	if (nb->idle) return -1;
	/* Distance is based on the pstate distance with the max_freq*/	
	if (na->dist_pstate== nb->dist_pstate) return 0;
	// if (na->dist_pstate<nb->dist_pstate) 1;
	if (na->dist_pstate<nb->dist_pstate) return 1;
	return -1;
}

void print_ordered_node_info(uint numn,node_info_t * einfo)
{
	int i;
	for (i=0;i<numn;i++){
		verbose(1,"node[%d] ip=%d dist %u power_red %f victim %u idle %u",i,einfo[i].ip,einfo[i].dist_pstate,einfo[i].power_red,einfo[i].victim,einfo[i].idle);
	}
}
void select_victim_nodes(uint numn,node_info_t * einfo,float target)
{
	int i=0;
	float total=0;
	while((i<numn) && (total<target)){
		total+=einfo[i].power_red;
		einfo[i].victim=1;	
		i++;
	}
}

state_t get_nodes_status(cluster_conf_t my_cluster_conf,uint *nnodes,node_info_t **einfo)
{
	int i;
	float ff;
	ulong newf,diff_f;
	node_info_t *cinfo;
	int num_n;
	status_t *my_status;
	num_n=status_all_nodes(&my_cluster_conf,&my_status);
	if (num_n<=0){
		*nnodes=0;
		return EAR_ERROR;
	}
	debug("%d nodes have reported their status",num_n);
	cinfo=(node_info_t *)calloc(num_n,sizeof(node_info_t));
	if (cinfo==NULL) return EAR_ERROR;
	for (i=0;i<num_n;i++){
		// dist_pstate and ip
		ff=roundf((float)my_status[i].node.avg_freq/100000.0);
		newf=(ulong)((ff/10.0)*1000000);
		diff_f=my_status[i].node.max_freq-newf;
		cinfo[i].dist_pstate=diff_f/100000;
		cinfo[i].ip=my_status[i].ip;
		if (cinfo[i].dist_pstate==1) cinfo[i].power_red=0.1/(float)num_n;
		else cinfo[i].power_red=0.05/(float)num_n;
		cinfo[i].victim=0;
		if (my_status[i].app.job_id==0) cinfo[i].idle=1;
	}	
	qsort((void *)cinfo,num_n,sizeof(node_info_t),compare_node_info_lower_first);

	/* ES un vector consecutivo ? */
	free(my_status);
	*nnodes=num_n;
	*einfo=cinfo;
	return EAR_SUCCESS;	
}

void create_risk(risk_t *my_risk,int wl)
{
  *my_risk=0;
  switch(wl){
    case EARGM_WARNING1:set_risk(my_risk,WARNING1);last_risk_sent=EARGM_WARNING1;break;
    case EARGM_WARNING2:set_risk(my_risk,WARNING1);add_risk(my_risk,WARNING2);last_risk_sent=EARGM_WARNING2;break;
    case EARGM_PANIC:set_risk(my_risk,WARNING1);add_risk(my_risk,WARNING2);add_risk(my_risk,PANIC);last_risk_sent=EARGM_PANIC;break;
  }
  verbose(1,"EARGM risk level %lu",(ulong)*my_risk);
  default_state=0;
}



void manage_warning(risk_t * risk,uint level,cluster_conf_t my_cluster_conf,float target,uint mode)
{
	uint numn;
#if 0
	verbose(1,"Our target is to reduce %.2f %% of  Watts",target);
	if (get_nodes_status(my_cluster_conf,&numn,&einfo)!=EAR_SUCCESS){
		error("Getting node status");
	}else{
		select_victim_nodes(numn,einfo,target);
		print_ordered_node_info(numn,einfo);
	}
#endif
	if (mode){
		create_risk(risk,level);
		set_risk_all_nodes(*risk,0,&my_cluster_conf);
	}
}

