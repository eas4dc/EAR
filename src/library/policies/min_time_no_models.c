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
//#include <ear/states.h>
//#include <ear/policy.h>
#include <common/states.h> //clean
#include <common/config.h>
#include <common/math_operations.h>
#include <common/types/projection.h> //?
#include <library/policies/policy_api.h> //clean
#include <library/policies/policy_ctx.h> //clean
#include <common/hardware/frequency.h>
#include <daemon/eard_api.h> //?

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif


typedef unsigned long ulong;
static signature_t *sig_list;
static unsigned int *sig_ready;

static void go_next_mt(int curr_pstate,int *ready,ulong *best_freq,int min_pstate)
{
  int next_pstate;
fprintf(stderr,"go_next_mt: curr_pstate %d min_pstate %d\n",curr_pstate,min_pstate);
  if (curr_pstate==min_pstate){
fprintf(stderr,"ready\n");
    *ready=1;
    *best_freq=frequency_pstate_to_freq(curr_pstate);
  }else{
    next_pstate=curr_pstate-1;
    *ready=0;
    *best_freq=frequency_pstate_to_freq(next_pstate);
fprintf(stderr,"Not ready: next %d freq %lu\n",next_pstate,*best_freq);
  }
}

static int is_better_min_time(signature_t * curr_sig,signature_t *prev_sig,double min_eff_gain)
{
  double freq_gain,perf_gain;
  int curr_freq,prev_freq;

  curr_freq=curr_sig->def_f;
  prev_freq=prev_sig->def_f;
  freq_gain=min_eff_gain*(double)((curr_freq-prev_freq)/(double)prev_freq);
  perf_gain=(prev_sig->time-curr_sig->time)/prev_sig->time;
  if (perf_gain>=freq_gain) return 1;
  return 0;
}


state_t policy_init(polctx_t *c)
{
if (c!=NULL){
sig_list=(signature_t *)malloc(sizeof(signature_t)*c->num_pstates);
sig_ready=(unsigned int *)malloc(sizeof(unsigned int)*c->num_pstates);
if ((sig_list==NULL) || (sig_ready==NULL)) return EAR_ERROR;
memset(sig_list,0,sizeof(signature_t)*c->num_pstates);
memset(sig_ready,0,sizeof(unsigned int)*c->num_pstates);
return EAR_SUCCESS;
}else return EAR_ERROR;
}


state_t policy_loop_init(polctx_t *c,loop_id_t *loop_id)
{
if (c!=NULL){ 
projection_reset(c->num_pstates);
memset(sig_ready,0,sizeof(unsigned int)*c->num_pstates);
return EAR_SUCCESS;
}else{
return EAR_ERROR;
}

}

state_t policy_loop_end(polctx_t *c,loop_id_t *loop_id)
{
    if ((c!=NULL) && (c->reset_freq_opt))
    {// Use configuration when available
			*(c->ear_frequency) = eards_change_freq(DEF_FREQ(c->app->def_freq));
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

*ready=0;

if (c==NULL) return EAR_ERROR;
if (c->app==NULL) return EAR_ERROR;

    if (c->use_turbo) min_pstate=0;
    else min_pstate=frequency_closest_pstate(c->app->max_freq);

    // This is the frequency at which we were running
curr_freq=*(c->ear_frequency);
    curr_pstate = frequency_closest_pstate(curr_freq);

// New signature ready
sig_ready[curr_pstate]=1;
signature_copy(&sig_list[curr_pstate], sig);


// Default values

min_eff_gain=c->app->settings[0];
def_freq=DEF_FREQ(c->app->def_freq);
def_pstate=frequency_closest_pstate(def_freq);


// ref=1 is nominal 0=turbo, we are not using it


    ulong prev_pstate,next_pstate;
    signature_t *prev_sig;
    /* We must not use models , we will check one by one*/
    /* If we are not running at default freq, we must check if we must follow */
    if (sig_ready[def_pstate]==0){
    *ready=0;
    *new_freq=def_freq;
    } else{
    /* This is the normal case */
      if (curr_freq != def_freq){
      prev_pstate=curr_pstate+1;
       prev_sig=&sig_list[prev_pstate];
       if (is_better_min_time(sig,prev_sig,min_eff_gain)){
       go_next_mt(curr_pstate,ready,new_freq,min_pstate);
        }else{
        *ready=1;
         *new_freq=frequency_pstate_to_freq(prev_pstate);
        }
}else{
      go_next_mt(curr_pstate,ready,new_freq,min_pstate);
      }
    }
return EAR_SUCCESS;
}


state_t policy_ok(polctx_t *c,signature_t *curr_sig,signature_t *last_sig,int *ok)
{

state_t st=EAR_SUCCESS;

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
        *f=DEF_FREQ(c->app->def_freq);
    }
return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
  *intents=2;
  return EAR_SUCCESS;
}

