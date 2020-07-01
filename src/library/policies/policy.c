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

#include <dlfcn.h>
#include <common/includes.h>
#include <common/system/symplug.h>
#include <common/hardware/frequency.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/policy.h>
#include <library/common/externs.h>
#include <common/environment.h>
#include <daemon/eard_api.h>

// external symbols
extern MPI_Comm masters_comm,new_world_comm;

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif


typedef struct policy_symbols {
	state_t (*init)        (polctx_t *c);
	state_t (*apply)       (polctx_t *c,signature_t *my_sig, ulong *new_freq,int *ready);
	state_t (*get_default_freq)   (polctx_t *c, ulong *freq_set);
	state_t (*ok)          (polctx_t *c, signature_t *curr_sig,signature_t *prev_sig,int *ok);
	state_t (*max_tries)   (polctx_t *c,int *intents);
	state_t (*end)         (polctx_t *c);
	state_t (*loop_init)   (polctx_t *c,loop_id_t *loop_id);
	state_t (*loop_end)    (polctx_t *c,loop_id_t *loop_id);
	state_t (*new_iter)    (polctx_t *c,loop_id_t *loop_id);
	state_t (*mpi_init)    (polctx_t *c);
	state_t (*mpi_end)     (polctx_t *c);
	state_t (*configure) (polctx_t *c);
} polsym_t;

// Static data
static polsym_t polsyms_fun;
static void    *polsyms_obj = NULL;
const int       polsyms_n = 12;
const char     *polsyms_nam[] = {
	"policy_init",
	"policy_apply",
	"policy_get_default_freq",
	"policy_ok",
	"policy_max_tries",
	"policy_end",
	"policy_loop_init",
	"policy_loop_end",
	"policy_new_iteration",
	"policy_mpi_init",
	"policy_mpi_end",
	"policy_configure"
};
polctx_t my_pol_ctx;

state_t policy_load(char *obj_path)
{
	return symplug_open(obj_path, (void **)&polsyms_fun, polsyms_nam, polsyms_n);
}

state_t init_power_policy(settings_conf_t *app_settings,resched_t *res)
{
  char basic_path[SZ_PATH_INCOMPLETE];
	conf_install_t *data=&app_settings->installation;

  char *obj_path = getenv("SLURM_EAR_POWER_POLICY");
  if ((obj_path==NULL) || (app_settings->user_type!=AUTHORIZED)){
    	sprintf(basic_path,"%s/policies/%s.so",data->dir_plug,app_settings->policy_name);
    	obj_path=basic_path;
	}
  debug("loading policy %s",obj_path);
	policy_load(obj_path);
	ear_frequency=DEF_FREQ(app_settings->def_freq);
	my_pol_ctx.app=app_settings;
	my_pol_ctx.reconfigure=res;
	my_pol_ctx.user_selected_freq=DEF_FREQ(app_settings->def_freq);
	my_pol_ctx.reset_freq_opt=get_ear_reset_freq();
	my_pol_ctx.ear_frequency=&ear_frequency;
	my_pol_ctx.num_pstates=frequency_get_num_pstates();
	my_pol_ctx.use_turbo=ear_use_turbo;
	my_pol_ctx.mpi.comm=new_world_comm;
	my_pol_ctx.mpi.master_comm=masters_comm;
	return policy_init();
}


/* Policy functions previously included in models */

/*
 *
 * Policy wrappers
 *
 */

state_t policy_init()
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.init, c);
}

state_t policy_apply(signature_t *my_sig,ulong *freq_set, int *ready)
{
	polctx_t *c=&my_pol_ctx;
	state_t st=EAR_ERROR;
	*ready=1;
	if (polsyms_fun.apply!=NULL){
		if (!eards_connected()){
			*ready=0;
			return EAR_SUCCESS;
		}
		st=polsyms_fun.apply(c, my_sig,freq_set,ready);
  	if (*freq_set != *(c->ear_frequency))
  	{
    	*(c->ear_frequency) =  eards_change_freq(*freq_set);
		}
  } else{
		if (c!=NULL) *freq_set=DEF_FREQ(c->app->def_freq);
	}
	return st;
}

state_t policy_get_default_freq(ulong *freq_set)
{
	polctx_t *c=&my_pol_ctx;
	if (polsyms_fun.get_default_freq!=NULL){
		return polsyms_fun.get_default_freq( c, freq_set);
	}else{
		if (c!=NULL) *freq_set=DEF_FREQ(c->app->def_freq);
		return EAR_SUCCESS;
	}
}
state_t policy_set_default_freq()
{
  polctx_t *c=&my_pol_ctx;
	state_t st;
	if (polsyms_fun.get_default_freq!=NULL){
  	st=polsyms_fun.get_default_freq(c,c->ear_frequency);
  	eards_change_freq(*(c->ear_frequency));
		return st;
	}else{ 
		return EAR_ERROR;
	}
}



state_t policy_ok(signature_t *curr,signature_t *prev,int *ok)
{
	polctx_t *c=&my_pol_ctx;
	if (polsyms_fun.ok!=NULL){
		return polsyms_fun.ok(c, curr,prev,ok);
	}else{
		*ok=1;
		return EAR_SUCCESS;
	}
}

state_t policy_max_tries(int *intents)
{ 
	polctx_t *c=&my_pol_ctx;
	if (polsyms_fun.max_tries!=NULL){
		return	polsyms_fun.max_tries( c,intents);
	}else{
		*intents=1;
		return EAR_SUCCESS;
	}
}

state_t policy_end()
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.end,c);
}
/** LOOPS */
state_t policy_loop_init(loop_id_t *loop_id)
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.loop_init,c, loop_id);
}

state_t policy_loop_end(loop_id_t *loop_id)
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.loop_end,c, loop_id);
}
state_t policy_new_iteration(loop_id_t *loop_id)
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.new_iter,c,loop_id);
}

/** MPI CALLS */

state_t policy_mpi_init()
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.mpi_init,c);
}

state_t policy_mpi_end()
{
	polctx_t *c=&my_pol_ctx;
	preturn(polsyms_fun.mpi_end,c);
}

/** GLOBAL EVENTS */

state_t policy_configure()
{
  polctx_t *c=&my_pol_ctx;
  preturn(polsyms_fun.configure, c);
}

state_t policy_force_global_frequency(ulong new_f)
{
  ear_frequency=new_f;
  eards_change_freq(ear_frequency);
	return EAR_SUCCESS;
}

void print_policy_ctx(polctx_t *p)
{
	fprintf(stderr,"user_type %u\n",p->app->user_type);
	fprintf(stderr,"policy_name %s\n",p->app->policy_name);
	fprintf(stderr,"setting 0 %lf\n",p->app->settings[0]);
	fprintf(stderr,"def_freq %lu\n",DEF_FREQ(p->app->def_freq));
	fprintf(stderr,"def_pstate %u\n",p->app->def_p_state);
	fprintf(stderr,"reconfigure %d\n",p->reconfigure->force_rescheduling);
	fprintf(stderr,"user_selected_freq %lu\n",p->user_selected_freq);
	fprintf(stderr,"reset_freq_opt %lu\n",p->reset_freq_opt);
	fprintf(stderr,"ear_frequency %lu\n",*(p->ear_frequency));
	fprintf(stderr,"num_pstates %u\n",p->num_pstates);
	fprintf(stderr,"use_turbo %u\n",p->use_turbo);
}
