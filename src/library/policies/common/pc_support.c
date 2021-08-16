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
//#define SHOW_DEBUGS 1
#include <common/includes.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <daemon/powercap/powercap_status.h>
#include <common/types/signature.h>
#include <management/cpufreq/frequency.h>
#include <library/metrics/gpu.h>
#include <library/policies/policy_ctx.h>
#include <common/types/projection.h>
#include <common/types/pc_app_info.h>
#if POWERCAP
extern pc_app_info_t *pc_app_info_data;
static const ulong **gpu_freq_list;
static const uint *gpu_freq_num;
state_t pc_support_init(polctx_t *c)
{
	#if USE_GPUS
	state_t ret;
	if ((ret = gpu_lib_freq_list(&gpu_freq_list,&gpu_freq_num)) != EAR_SUCCESS){
		debug("Error accessing gpu_freq list");
		return ret;
	}
	pc_app_info_data->num_gpus_used=c->num_gpus;
	#endif
	return EAR_SUCCESS;
}
#if USE_GPUS

#if 0
static ulong select_lower_gpu_freq(uint i,ulong f)
{
	int j=0,found=0;
	/* If we are the minimum*/
	debug("Looking for lower freq for %lu",f);
	if (f == gpu_freq_list[i][gpu_num_freqs[i]-1]) return f;
	/* Otherwise, look for f*/
	do{
		if (gpu_freq_list[i][j] == f) found = 1;
		else j++;
	}while((found==0) && (j!=gpu_num_freqs[i]));
	if (!found){
		debug("Frequency %lu not found in GPU %u",f,i);
		return gpu_freq_list[i][gpu_num_freqs[i]-1];
	}else{
		debug("Frequency %lu found in position %d",f,j);
	}
	if (j < (gpu_num_freqs[i]-1)){
		return gpu_freq_list[i][j+1];
	}
	return f;
}

static ulong avg_gpu_freq(signature_t *s)
{
	int i,gused=0;
	ulong avgf=0;
	for (i=0;i<s->gpu_sig.num_gpus;i++){
		if (s->gpu_sig.gpu_data[i].GPU_util){
			gused++;
			avgf+=gpu_sig.gpu_data[i].GPU_freq;
		}
	}
	return avgf/gused;
}
#endif

static void estimate_gpu_freq(ulong *oldf,signature_t *s,double glimit)
{
	#if SHOW_DEBUGS
	int used = sig_gpus_used(s);
	double gpower=sig_total_gpu_power(s);
	double gpower_per_gpu,glimit_per_gpu;
	gpower_per_gpu = gpower/used;
	glimit_per_gpu = glimit = used;
	red = (gpower_per_gpu - glimit_per_gpu)/glimit_per_gpu;
	debug("We should reduce a %f of gpu power",red);
	#endif
	return;
}

void pc_support_adapt_gpu_freq(polctx_t *c,node_powercap_opt_t *pc,ulong *f,signature_t *s)
{
    uint plimit;
    double gpower;
	  signature_t new_s;

		signature_copy(&new_s,s);
		new_s.DC_power = sig_total_gpu_power(s);
    plimit = pc->pper_domain[DOMAIN_GPU];        /* limit */
		gpower = new_s.DC_power;
    debug("checking gpu frequency: gpu_power %lf gpu_powercap %u",gpower,plimit);
		gpu_lib_freq_limit_get_current(&c->gpu_mgt_ctx,f);
    if ((uint)gpower <= plimit){
      pc_app_info_data->pc_gpu_status = PC_STATUS_OK;
			debug("GPU power is enough");
      return ;
    }else{
      pc_app_info_data->pc_status = PC_STATUS_GREEDY;
			debug("GPU power is NOT enough,reducing GPU freq");
			estimate_gpu_freq(f,&new_s,plimit);
		}
    return;

}
#endif

ulong pc_support_adapt_freq(polctx_t *c,node_powercap_opt_t *pc,ulong f,signature_t *s)
{
		ulong req_f,cfreq;
    uint plimit,cpstate,ppstate,power_status;
		uint numpstates,adapted=0;
		double ppower;
	  signature_t new_s;
		signature_copy(&new_s,s);
		new_s.DC_power = sig_node_power(s);
    req_f  = f;
    plimit = pc->pper_domain[DOMAIN_NODE]; 				/* limit */
		cfreq=frequency_closest_high_freq(new_s.avg_f,1); 	/* current freq */
		cpstate=frequency_closest_pstate(cfreq);			 	/* current pstate */
		ppstate=frequency_closest_pstate(req_f);				/* pstate for freq selected */
		if (projection_available(cpstate,ppstate)==EAR_SUCCESS){
			project_power(&new_s,cpstate,ppstate,&ppower);				/* Power at freq selected */
			adapted = 1;
		}else{
			ppower = plimit;
		}
		debug("checking frequency: cfreq %lu cpstate %u ppstate %u cpower %lf ppower %lf limit %u",cfreq,cpstate,ppstate,new_s.DC_power,ppower,plimit);		
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
					project_power(&new_s,cpstate,ppstate,&ppower);
					pc_app_info_data->req_power=(uint)ppower;
				}else ppower = plimit;
			}while(((uint)ppower>plimit) && (ppstate<numpstates));
		}
		if (adapted) req_f=frequency_pstate_to_freq(ppstate);
		return req_f;

}

void pc_support_compute_next_state(polctx_t *c,node_powercap_opt_t *pc,signature_t *s)
{
#if 0
		uint power_status;
		ulong eff_f;
    if (is_powercap_set(pc)){
      power_status=compute_power_status(pc,(uint)(s->DC_power));
      eff_f=frequency_closest_high_freq(s->avg_f,1);
			if (eff_f < pc_app_info_data->req_f){
				verbose_master(2,"Running at reduced freq with powercap, status %u and effective freq %lu vs selected %lu",power_status,eff_f,pc_app_info_data->req_f);
    		//pc_app_info_data->pc_status=compute_next_status(pc,(uint)(s->DC_power),eff_f,pc_app_info_data->req_f);
				pc_app_info_data->pc_status=PC_STATUS_GREEDY;
			}else{
				verbose_master(2,"Running at requested freq with powercap eff_f %lu req_f %lu , going to status %u",eff_f,pc_app_info_data->req_f,power_status);
				if (power_status == PC_STATUS_RELEASE) pc_app_info_data->pc_status=power_status;
			}
    }else{
      power_status=PC_STATUS_ERROR;
    }
#endif
}
#endif
