/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
// #define SHOW_DEBUGS 1
#include <common/states.h>
#include <library/common/externs.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/types/signature.h>
#include <common/types/coefficient.h>
#include <common/hardware/architecture.h>
#include <management/cpufreq/frequency.h>
#include <daemon/shared_configuration.h>
#include <library/models/models_api.h>


static coefficient_t **coefficients;
static coefficient_t *coefficients_sm;
static int num_coeffs;
static uint num_pstates;
static uint spr_imc_perc_model_init=0;

static int valid_range(ulong from,ulong to)
{
	if ((from<num_pstates) && (to<num_pstates)) return 1;
	else return 0;
}

/* This function loads any information needed by the energy model */
state_t energy_model_init(char *ear_etc_path, char *ear_tmp_path, architecture_t *arch_desc)
{
  char *hack_file = ear_getenv(HACK_EARL_COEFF_FILE);
  int i, ref;

	debug("Using spr_imc_perc_model\n");
	num_pstates = (uint)arch_desc->pstates;
	debug("Using %u pstates",num_pstates);

  coefficients = (coefficient_t **) malloc(sizeof(coefficient_t *) * num_pstates);
  if (coefficients == NULL) {
		return EAR_ERROR;
  }
  for (i = 0; i < num_pstates; i++)
  {
    coefficients[i] = (coefficient_t *) malloc(sizeof(coefficient_t) * num_pstates);
    if (coefficients[i] == NULL) {
			return EAR_ERROR;
    }

    for (ref = 0; ref < num_pstates; ref++)
    {

      coefficients[i][ref].pstate_ref = frequency_pstate_to_freq(i);
      coefficients[i][ref].pstate = frequency_pstate_to_freq(ref);
      coefficients[i][ref].available = 0;
    }
  }


  char coeffs_path[GENERIC_NAME];
  int cfile_size;

  if (hack_file == NULL){
 		xsnprintf(coeffs_path, sizeof(coeffs_path), "%s/ear/coeffs/island%d/coeffs.imc_perc.%s",\
 	  ear_etc_path, system_conf->island, system_conf->tag);
 		
		debug("spr_imc_perc_model coeffs path %s", coeffs_path);

  }else{
    strcpy(coeffs_path,hack_file);
	}
  debug("Using hck coefficients path %s", coeffs_path);
  cfile_size = coeff_file_size(coeffs_path);
  if (cfile_size == 0) num_coeffs = 0;
  else{
      int num_coeffs_in_file = cfile_size / sizeof(coefficient_t);
      num_coeffs = cfile_size ;
      debug("%d coefficients found in file", num_coeffs_in_file);
      coefficients_sm = (coefficient_t *) calloc(num_coeffs_in_file, sizeof(coefficient_t));
      coeff_file_read_no_alloc(coeffs_path, coefficients_sm, cfile_size);
      debug("Coefficients read from file");
  }

  if (num_coeffs>0){
    num_coeffs=num_coeffs/sizeof(coefficient_t);
    int ccoeff;
    for (ccoeff = 0 ; ccoeff < num_coeffs;ccoeff++){
      ref = frequency_closest_pstate(coefficients_sm[ccoeff].pstate_ref);
      i = frequency_closest_pstate(coefficients_sm[ccoeff].pstate);
      if (frequency_is_valid_pstate(ref) && frequency_is_valid_pstate(i)){
				memcpy(&coefficients[ref][i],&coefficients_sm[ccoeff],sizeof(coefficient_t));
        debug("initializing coeffs for ref: %d i: %d\n", ref, i);
      }
    }
  }
	spr_imc_perc_model_init=1;	
#if SHOW_DEBUGS
    for (ref = 0; ref < num_pstates; ref++)
    {
        for (i = 0; i < num_pstates; i++)
                debug("coefficient from ref: %d i: %d available: %d\n", ref, i, coefficients[ref][i].available);
    }
#endif
	return EAR_SUCCESS;
}

/*
*	Time(f) = (E*AvgIMC_freq + F)* Time(fref)
*/
state_t energy_model_project_time(signature_t *sign,ulong from,ulong to,double *ptime)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff;
	if ((spr_imc_perc_model_init) && (valid_range(from,to))){
		coeff=&coefficients[from][to];
		if (coeff->available){
			*ptime= sign->time * (coeff->E * sign->avg_imc_f + coeff->F);
       debug("Time %lf = Time(%lf) * (E x MemF(%lf x %lu) + F(%lf))", *ptime, sign->time, coeff->E, sign->avg_imc_f, coeff->F);
		}else{
			*ptime=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

/*
Power(f) = A⋅P(f_ref) + B⋅Avg_imcf + C
*/

state_t energy_model_project_power(signature_t *sign, ulong from,ulong to,double *ppower)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff;
	if ((spr_imc_perc_model_init) && (valid_range(from,to))){
		coeff=&coefficients[from][to];
		if (coeff->available){
			double power = compute_dc_nogpu_power(sign); 	
			*ppower= (coeff->A * power) +
		   (coeff->B * sign->avg_imc_f) +
		   (coeff->C);
       debug("Power %lf = AxPower(%lf x %lf) + BxMemF(%lf x %lu) + C(%lf)", *ppower, coeff->A, power, coeff->B, sign->avg_imc_f, coeff->C);
		}else{
			*ppower=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

state_t energy_model_projection_available(ulong from,ulong to)
{
	if (valid_range(from, to) && coefficients[from][to].available) return EAR_SUCCESS;
	else return EAR_ERROR;
}

state_t energy_model_projections_available()
{
  uint ready = 0;
  for (uint ps = 0; ps < num_pstates && !ready; ps++){
    for (uint pt = 0; pt < num_pstates && !ready; pt++){
      if ((ps != pt) && (energy_model_projection_available(ps, pt) == EAR_SUCCESS)) ready = 1;
    }
  }
  if (ready) return EAR_SUCCESS;
  return EAR_ERROR;
}

