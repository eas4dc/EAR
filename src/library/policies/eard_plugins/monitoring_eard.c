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
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/risk.h>
#include <management/cpufreq/frequency.h>


#define WARNING1_DEF_PSTATES_LESS 0
#define WARNING2_DEF_PSTATES_LESS 1
#define PANIC_DEF_PSTATES_LESS 1



state_t policy_set_risk(policy_conf_t *ref,policy_conf_t *current,risk_t risk_level,ulong opt_target,ulong mfreq,ulong *nfreq,ulong *f_list,uint nump)
{
  debug("monitoring , risk set to %lu",(unsigned long)risk_level);
  debug("current conf def_pstate %u max_freq %lu",ref->p_state,mfreq);
  int risk1,risk2,panic;
  unsigned long f;

  risk1=is_risk_set(risk_level,WARNING1);
  risk2=is_risk_set(risk_level,WARNING2);
  panic=is_risk_set(risk_level,PANIC);
  if (risk1){
    debug("We are in risk1");
    current->p_state=ref->p_state+WARNING1_DEF_PSTATES_LESS;
  }
  if (risk2){
    debug("We are in risk2");
    current->p_state=ref->p_state+WARNING1_DEF_PSTATES_LESS+WARNING2_DEF_PSTATES_LESS;
  }
  if (panic){
    current->p_state=ref->p_state+WARNING1_DEF_PSTATES_LESS+WARNING2_DEF_PSTATES_LESS+PANIC_DEF_PSTATES_LESS;
    debug("We are in panic");
  }
  f=frequency_pstate_to_freq_list(current->p_state,f_list,nump);
  current->def_freq=(float)f/1000000.0;
  *nfreq=f;
  debug("new conf def_pstate %u def_freq %f max_freq %lu",current->p_state,current->def_freq,*nfreq);
  return EAR_SUCCESS;

}

