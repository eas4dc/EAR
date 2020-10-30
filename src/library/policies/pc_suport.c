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

#include <common/config.h>
#define SHOW_DEBUGS 1
#include <common/includes.h>
#include <daemon/powercap_status.h>
#include <common/types/signature.h>
#include <common/hardware/frequency.h>
#include <common/types/projection.h>
#include <common/types/pc_app_info.h>
#if POWERCAP
extern pc_app_info_t *pc_app_info_data;

ulong pc_support_adapt_freq(node_powercap_opt_t *pc,ulong f,signature_t *s)
{
		ulong req_f,cfreq;
    uint plimit,cpstate,ppstate,power_status;
		uint numpstates,adapted=0;
		double ppower;
    req_f=f;
    plimit=pc->last_t1_allocated; 				/* limit */
		cfreq=frequency_closest_high_freq(s->avg_f,1); 	/* current freq */
		cpstate=frequency_closest_pstate(cfreq);			 	/* current pstate */
		ppstate=frequency_closest_pstate(req_f);				/* pstate for freq selected */
		if (projection_available(cpstate,ppstate)==EAR_SUCCESS){
			project_power(s,cpstate,ppstate,&ppower);				/* Power at freq selected */
			adapted=1;
		}else{
			ppower=plimit;
		}
		debug("checking frequency: cfreq %lu cpstate %u ppstate %u power %lf limit %u",cfreq,cpstate,ppstate,ppower,plimit);		
		if (((uint)ppower<=plimit) && (adapted)){
			power_status=compute_power_status(pc,(uint)ppower);
			pc_app_info_data->req_power=(uint)ppower;
			if (power_status == PC_STATUS_RELEASE) pc_app_info_data->pc_status=power_status;
			return req_f;
		}else{
			pc_app_info_data->pc_status=PC_STATUS_GREEDY;
			/* If the power was projected we reprot it, otherwise we set to 0 */
			if (adapted) pc_app_info_data->req_power=(uint)ppower;
			else pc_app_info_data->req_power=0;
			numpstates=frequency_get_num_pstates();
			do{
				ppstate++;
				if (projection_available(cpstate,ppstate)==EAR_SUCCESS){			
					adapted=1;
					project_power(s,cpstate,ppstate,&ppower);
					pc_app_info_data->req_power=(uint)ppower;
				}else ppower = plimit;
			}while(((uint)ppower>plimit) && (ppstate<numpstates));
		}
		if (adapted) req_f=frequency_pstate_to_freq(ppstate);
		return req_f;

}

void pc_support_compute_next_state(node_powercap_opt_t *pc,signature_t *s)
{
		uint power_status;
		ulong eff_f;
    if (is_powercap_set(pc)){
      power_status=compute_power_status(pc,(uint)(s->DC_power));
      eff_f=frequency_closest_high_freq(s->avg_f,1);
			if (eff_f < pc_app_info_data->req_f){
				verbose(1,"Running at reduced freq with powercap, status %u and effective freq %lu vs selected %lu",power_status,eff_f,pc_app_info_data->req_f);
    		//pc_app_info_data->pc_status=compute_next_status(pc,(uint)(s->DC_power),eff_f,pc_app_info_data->req_f);
				pc_app_info_data->pc_status=PC_STATUS_GREEDY;
			}else{
				verbose(1,"Running at requested freq with powercap eff_f %lu req_f %lu , going to status %u",eff_f,pc_app_info_data->req_f,power_status);
				if (power_status == PC_STATUS_RELEASE) pc_app_info_data->pc_status=power_status;
			}
    }else{
      verbose(1,"Powercap is not set");
      power_status=PC_STATUS_ERROR;
    }
}
#endif
