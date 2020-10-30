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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/hardware/frequency.h>
#include <common/types/projection.h>
#include <daemon/eard_api.h>
#include <library/policies/policy_api.h>
#include <common/math_operations.h>
#include <library/common/externs.h>
#include <common/system/time.h>
#if POWERCAP
#include <daemon/powercap_status.h>
#endif


static timestamp pol_time_init;
static ulong req_f;

typedef unsigned long ulong;

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define FREQ_DEF(f) (!ext_def_freq?f:ext_def_freq)
#else
#define FREQ_DEF(f) f
#endif

state_t policy_init(polctx_t *c)
{
	if (c!=NULL){ 
	  sig_shared_region[my_node_id].mpi_info.mpi_time=0;
  	sig_shared_region[my_node_id].mpi_info.total_mpi_calls=0;
		sig_shared_region[my_node_id].mpi_info.exec_time=0;
		sig_shared_region[my_node_id].mpi_info.perc_mpi=0;

		return EAR_SUCCESS;
	}else return EAR_ERROR;
}


state_t policy_loop_init(polctx_t *c,loop_id_t *loop_id)
{
		if (c!=NULL){ 
			projection_reset(c->num_pstates);
			return EAR_SUCCESS;
		}else{
			return EAR_ERROR;
		}
}

state_t policy_loop_end(polctx_t *c,loop_id_t *loop_id)
{
    if ((c!=NULL) && (c->reset_freq_opt))
    {
        // Use configuration when available
        *(c->ear_frequency) = eards_change_freq(FREQ_DEF(c->app->def_freq));
    }
	return EAR_SUCCESS;
}



// This is the main function in this file, it implements power policy
state_t policy_apply(polctx_t *c,signature_t *sig,ulong *new_freq,int *ready)
{
    signature_t *my_app;
    int i,min_pstate;
    unsigned int ref,try_next;
    double freq_gain,perf_gain,min_eff_gain;
    double power_proj,time_proj;
    double power_ref,time_ref,time_current;
    ulong best_freq,best_pstate,freq_ref;
		ulong curr_freq;
		ulong curr_pstate,def_pstate,def_freq;
		state_t st;
    my_app=sig;

		*ready=1;


		if (c==NULL) return EAR_ERROR;
		if (c->app==NULL) return EAR_ERROR;

#if POWERCAP
  	if (is_powercap_set(&c->app->pc_opt)){ 
			verbose(1,"Powercap is set to %uWatts",get_powercapopt_value(&c->app->pc_opt));
		}else{ 
			verbose(1,"Powercap is not set");
		}
#endif


    if (c->use_turbo) min_pstate=0;
    else min_pstate=frequency_closest_pstate(c->app->max_freq);

		// Default values
		
		min_eff_gain=c->app->settings[0];
		def_freq=FREQ_DEF(c->app->def_freq);
		def_pstate=frequency_closest_pstate(def_freq);

    // This is the frequency at which we were running
    #ifdef POWERCAP
		curr_freq=frequency_closest_high_freq(my_app->avg_f,1);
		#else
    curr_freq=*(c->ear_frequency);
		#endif
    curr_pstate = frequency_closest_pstate(curr_freq);
		


    // If is not the default P_STATE selected in the environment, a projection
    // is made for the reference P_STATE in case the projections were available.
    if (curr_freq != def_freq) // Use configuration when available
    {
				if (projection_available(curr_pstate,def_pstate)==EAR_SUCCESS)
        {
					st=project_power(my_app,curr_pstate,def_pstate,&power_ref);
					st=project_time(my_app,curr_pstate,def_pstate,&time_ref);
          best_freq=def_freq;
					best_pstate=def_pstate;
        }
        else
        {
          time_ref=my_app->time;
          power_ref=my_app->DC_power;
          best_freq=curr_freq;
					best_pstate=curr_pstate;
        }
    }
    else
    { 
        time_ref=my_app->time;
        power_ref=my_app->DC_power;
        best_freq=curr_freq;
				best_pstate=curr_pstate;
    }


	// ref=1 is nominal 0=turbo, we are not using it

		try_next=1;
		i=best_pstate-1;
		time_current=time_ref;
		debug("Policy Signature (CPI=%lf GBS=%lf Power=%lf Time=%lf TPI=%lf)",my_app->CPI,my_app->GBS,my_app->DC_power,my_app->time,my_app->TPI);
		debug("Starting at pstate %d, curr_freq %lu def_freq %lu min_pstate %d",i,curr_freq,def_freq,min_pstate);

		while(try_next && (i >= min_pstate))
		{
			if (projection_available(curr_pstate,i)==EAR_SUCCESS)
			{
				//debug("Looking for pstate %d",i);
				st=project_power(my_app,curr_pstate,i,&power_proj);
				st=project_time(my_app,curr_pstate,i,&time_proj);
				projection_set(i,time_proj,power_proj);
				freq_ref=frequency_pstate_to_freq(i);
				freq_gain=min_eff_gain*(double)(freq_ref-best_freq)/(double)best_freq;
				perf_gain=(time_current-time_proj)/time_current;

				// OK
				if (perf_gain>=freq_gain)
				{
					best_freq=freq_ref;
					best_pstate=i;
					time_current = time_proj;
					i--;
				}
				else
				{
					try_next = 0;
				}
			} // Projections available
			else{
				try_next=0;
			}
		}	
		/* Controlar la freq por power cap , si capado poner GREEDY, gestionar req-f */	
		if (best_freq<def_freq) best_freq=def_freq;
		*new_freq=best_freq;
		return EAR_SUCCESS;
}


state_t policy_ok(polctx_t *c,signature_t *curr_sig,signature_t *last_sig,int *ok)
{
	state_t st=EAR_SUCCESS;
	ulong eff_f;
	uint power_status;
	uint next_status;

	if ((c==NULL) || (curr_sig==NULL) || (last_sig==NULL)) return EAR_ERROR;



	
	if (curr_sig->def_f==last_sig->def_f) *ok=1;

	// Check that efficiency is enough
	if (curr_sig->time < last_sig->time) *ok=1;

	// If time is similar and it was same freq it is ok	
	if (equal_with_th(curr_sig->time , last_sig->time,0.1) && (curr_sig->def_f == last_sig->def_f)) *ok=1;
	
	// If we run at a higher freq but we are slower, we are not ok	
	if ((curr_sig->time > last_sig->time) && (curr_sig->def_f > last_sig->def_f)) *ok=0;

	else *ok=0;

	return st;

}
state_t  policy_get_default_freq(polctx_t *c,ulong *f)
{
		if ((c!=NULL) && (c->app!=NULL)){
    // Just in case the bestPstate was the frequency at which the application was running
        *f=FREQ_DEF(c->app->def_freq);
    }
		return EAR_SUCCESS;
}
	
state_t policy_max_tries(polctx_t *c,int *intents)
{
  *intents=2;
  return EAR_SUCCESS;
}


