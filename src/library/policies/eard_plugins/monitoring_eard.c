/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


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

