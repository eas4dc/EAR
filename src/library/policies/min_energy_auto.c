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
#define SHOW_DEBUGS 1
#include <common/states.h>
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

static double *ratios_time;
static double *ratios_energy;

state_t policy_init(polctx_t *c)
{
	if (c!=NULL){ 
		ratios_time=malloc(sizeof(double)*c->num_pstates);
		ratios_energy=malloc(sizeof(double)*c->num_pstates);
		return EAR_SUCCESS;
	}else return EAR_ERROR;
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
	int i,min_pstate;
	unsigned int ref;
	double power_proj,time_proj,energy_proj,best_solution,energy_ref;
	double power_ref,time_ref,max_penalty,time_max;
	double energy_nominal,time_nominal;

  ulong best_freq,best_pstate,freq_ref;
	ulong curr_freq,nominal;
	ulong curr_pstate,def_pstate,def_freq;
	state_t st;


	if ((c!=NULL) && (c->app!=NULL)){
		memset(ratios_time,0,sizeof(double)*c->num_pstates);
		memset(ratios_energy,0,sizeof(double)*c->num_pstates);

		fprintf(stderr,"Max_freq set to %lu\n",c->app->max_freq);
    if (c->use_turbo) min_pstate=0;
    else min_pstate=frequency_closest_pstate(c->app->max_freq);

		fprintf(stderr,"min_pstate = %d \n",min_pstate);	

		nominal=frequency_pstate_to_freq(min_pstate);

	ratios_time[min_pstate]=1;
	ratios_energy[min_pstate]=1;

		fprintf(stderr,"nominal %lu\n",nominal);

	// Default values
  max_penalty=c->app->settings[0];
	def_freq=FREQ_DEF(c->app->def_freq);
	def_pstate=frequency_closest_pstate(def_freq);
	


		// This is the frequency at which we were running
		curr_freq=*(c->ear_frequency);
        debug("curr_frequency %lu", curr_freq);
		curr_pstate = frequency_closest_pstate(curr_freq);
        debug("curr_pstate %lu", curr_pstate);


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
			if (curr_freq == nominal){
				/* And we are the nominal freq */
				time_ref=my_app->time;
				power_ref=my_app->DC_power;
				best_freq=curr_freq;
			}else{
				/* Nominal is the reference , if projections are ready, we project time and power */
                debug("current_freq is not the nominal\n");

				if (projection_available(curr_pstate,min_pstate) == EAR_SUCCESS){
					project_power(my_app,curr_pstate,min_pstate,&power_ref);
					project_time(my_app,curr_pstate,min_pstate,&time_ref);
					best_freq=nominal;
          debug("projecting to nominal\t time: %.2lf\t power: %.2lf\n", time_ref, power_ref);
				}else{
        	time_ref=my_app->time;
        	power_ref=my_app->DC_power;
        	best_freq=curr_freq;
				}
			}

		}
		energy_ref=power_ref*time_ref;
		best_solution=energy_ref;
		energy_nominal=energy_ref;
		time_nominal=time_ref;

	// We compute the maximum performance loss
	time_max = time_ref + (time_ref * max_penalty);

    debug("time_max: %.2lf\n", time_max);

	// MIN_ENERGY_TO_SOLUTION ALGORITHM
	for (i = min_pstate; i < c->num_pstates;i++)
	{
		if (projection_available(curr_pstate,i)==EAR_SUCCESS)
		{
				st=project_power(my_app,curr_pstate,i,&power_proj);
				st=project_time(my_app,curr_pstate,i,&time_proj);
				projection_set(i,time_proj,power_proj);
				energy_proj=power_proj*time_proj;
        debug("projected from %lu to %d\t time: %.2lf\t power: %.2lf energy: %.2lf", curr_pstate, i, time_proj, power_proj, energy_proj);
			ratios_energy[i]=((energy_nominal-energy_proj)/energy_nominal)*100.0;
			ratios_time[i]=((time_proj-time_nominal)/time_nominal)*100.0;
			#if 0
			if ((energy_proj < best_solution) && (time_proj < time_max))
			{
                    debug("new best solution found\n");
					best_freq = frequency_pstate_to_freq(i);
					best_solution = energy_proj;
			}
			#endif
		}
	}
	}else{ 
		*ready=0;
		return EAR_ERROR;
	}
	best_pstate=min_pstate;
	double best_ratio=1,curr;
	for (i = min_pstate+1; i < c->num_pstates;i++){
		/* If we save energy */
		curr=ratios_energy[i]/ratios_time[i];
		debug("pstate %d reports %lf energy saving and %lf time penalty ratio=%lf",i,ratios_energy[i],ratios_time[i],curr);
		if (ratios_energy[i]>0){
			if (curr>best_ratio){
				best_pstate=i;
				best_ratio=curr;
			}
		}	
	}
	best_freq = frequency_pstate_to_freq(best_pstate);	
	debug("Best ratio is pstate %lu f=%lu with ratio %lf",best_pstate,best_freq,best_ratio);
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
			*freq_set=c->app->max_freq;
		}else EAR_ERROR;
		return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
  *intents=1;
  return EAR_SUCCESS;
}

