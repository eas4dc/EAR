/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>
#include <common/hardware/frequency.h>
#include <common/types/projection.h>
#include <library/policies/policy_api.h>
#include <daemon/eard_api.h>

typedef unsigned long ulong;
#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define FREQ_DEF(f) (!ext_def_freq?f:ext_def_freq)
#else
#define FREQ_DEF(f) f
#endif


state_t policy_init(polctx_t *c)
{
	if (c!=NULL) return EAR_SUCCESS;
	else return EAR_ERROR;
}

state_t policy_loop_init(polctx_t *c,loop_id_t *l)
{
	if (c!=NULL){
    projection_reset(c->num_pstates);
		return EAR_SUCCESS;
	}else{
		return EAR_ERROR;
	}
}

state_t policy_loop_end(polctx_t *c,loop_id_t *l)
{
    if ((c!=NULL) && (c->reset_freq_opt))
    {
        *(c->ear_frequency) = eards_change_freq(FREQ_DEF(c->app->def_freq));
    }
	return EAR_SUCCESS;
}


state_t policy_apply(polctx_t *c,signature_t *sig,ulong *new_freq,int *ready)
{
	signature_t *my_app = sig;
	int i,min_pstate,max_pstate_512;
	unsigned int ref;
	double power_proj,time_proj,energy_proj,best_solution,energy_ref;
	double power_ref,time_ref,max_penalty,time_max;
	double power_proj_avx512,time_proj_avx512,power_proj_comb,time_proj_comb;
	double perc_avx512;


  ulong best_freq,best_pstate,freq_ref;
	ulong curr_freq;
	ulong curr_pstate,def_pstate,def_freq;
	state_t st;
	


	if ((c!=NULL) && (c->app!=NULL)){

    if (c->use_turbo) min_pstate=0;
    else min_pstate=frequency_freq_to_pstate(c->app->max_freq);

	// Default values

		max_penalty=c->app->settings[0];
		def_freq=FREQ_DEF(c->app->def_freq);
		def_pstate=frequency_freq_to_pstate(def_freq);

		// This is the frequency at which we were running
		curr_freq=*(c->ear_frequency);
		curr_pstate = frequency_freq_to_pstate(curr_freq);
		max_pstate_512=frequency_freq_to_pstate(2200000);

		*ready=1;

		// If is not the default P_STATE selected in the environment, a projection
		// is made for the reference P_STATE in case the coefficents were available.
		if (curr_freq != def_freq) // Use configuration when available
		{
		if (projection_available(curr_pstate,def_pstate)==EAR_SUCCESS)
		{
				st=project_power(my_app,curr_pstate,def_pstate,&power_ref);
				st=project_time(my_app,curr_pstate,def_pstate,&time_ref);
				best_freq=def_freq;
		}
		else
		{
				time_ref=my_app->time;
				power_ref=my_app->DC_power;
				best_freq=curr_freq;
		}
		}
		// If it is the default P_STATE selected in the environment, then a projection
		// is not needed, so the signature will be enough as a reference. 
		else
		{ // we are running at default frequency , signature is our reference
			time_ref=my_app->time;
			power_ref=my_app->DC_power;
			best_freq=curr_freq;
		}
		energy_ref=power_ref*time_ref;
		best_solution=energy_ref;

	// We compute the maximum performance loss
	time_max = time_ref + (time_ref * max_penalty);


	// MIN_ENERGY_TO_SOLUTION ALGORITHM
	for (i = min_pstate; i < c->num_pstates;i++)
	{
		if (projection_available(curr_pstate,i)==EAR_SUCCESS)
		{
				st=project_power(my_app,curr_pstate,i,&power_proj);
				st=project_time(my_app,curr_pstate,i,&time_proj);
				if (i>=max_pstate_512){
					st=project_power(my_app,curr_pstate,min_pstate,&power_proj_avx512);
					st=project_time(my_app,curr_pstate,min_pstate,&time_proj_avx512);
				}else{
					power_proj_avx512=power_proj;
					time_proj_avx512=time_proj;
				}
				perc_avx512=(double)((my_app->FLOPS[3]/(unsigned long long)16)+(my_app->FLOPS[7]/(unsigned long long)8))/(double)my_app->instructions;
				time_proj_comb=(time_proj*(1-perc_avx512)+time_proj_avx512*perc_avx512);
				power_proj_comb=(power_proj*(1-perc_avx512)+power_proj_avx512*perc_avx512);

				projection_set(i,time_proj,power_proj);
				energy_proj=power_proj_comb*time_proj_comb;
				debug("target %d: time (%lf,%lf) power (%lf,%lf) percavx512 %lf, best %lf current %lf",i,time_proj,time_proj_avx512,power_proj,power_proj_avx512,perc_avx512,best_solution,energy_proj);
			if ((energy_proj < best_solution) && (time_proj_comb < time_max))
			{
					best_freq = frequency_pstate_to_freq(i);
					best_solution = energy_proj;
			}
		}
	}
	}else{ 
		*ready=0;
		return EAR_ERROR;
	}
	*new_freq=best_freq;
	return EAR_SUCCESS;
}


state_t policy_ok(polctx_t *c,signature_t *curr_sig,signature_t *last_sig,int *ok)
{
	double energy_last, energy_curr;

	if ((c==NULL) || (curr_sig==NULL) || (last_sig==NULL)) return EAR_ERROR;
	if (curr_sig->def_f==last_sig->def_f) *ok=1;

	energy_last = last_sig->time*last_sig->DC_power;
	energy_curr = curr_sig->time * curr_sig->DC_power;

	if ((energy_curr<energy_last)&&(curr_sig->time<(last_sig->time*c->app->settings[0]))) *ok=1;
	else *ok=0;
	return EAR_SUCCESS;
}

state_t  policy_get_default_freq(polctx_t *c, ulong *freq_set)
{
		if (c!=NULL){
			if (*freq_set>c->app->max_freq)  *freq_set=c->app->max_freq;
		}else EAR_ERROR;
		return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
  *intents=1;
  return EAR_SUCCESS;
}

