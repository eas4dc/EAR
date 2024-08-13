/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/



#include <stdlib.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <common/types/signature.h>
#include <common/types/coefficient.h>
#include <common/hardware/architecture.h>
#include <management/cpufreq/frequency.h>
#include <daemon/shared_configuration.h>
#include <library/models/energy_models/common.h>
#include <library/common/verbose_lib.h>


static coefficient_t **coefficients;
static coefficient_t *coefficients_sm;
static int num_coeffs;
static uint num_pstates;
static uint basic_model_init;


/** Returns whether there exists a coefficients entry from \ref from_ps to \ref to_ps. */
static uint projection_available(ulong from_ps, ulong to_ps);


/** Returns whether the pair <from_ps, to_ps> is between the configured pstate list. */
static int valid_range(ulong from_ps, ulong to_ps);


/* This function loads any information needed by the energy model */
state_t energy_model_init(char *ear_etc_path, char *ear_tmp_path, architecture_t *arch_desc)
{
  char *hack_file = ear_getenv(HACK_EARL_COEFF_FILE);
  int i, ref;

	debug("Using basic_model\n");
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
#if 0
  get_coeffs_default_path(ear_tmp_path,coeffs_path);
  num_coeffs=0;
  coefficients_sm=attach_coeffs_default_shared_area(coeffs_path,&num_coeffs);
#endif
  if (hack_file == NULL){
    get_coeffs_default_path(ear_tmp_path,coeffs_path);
    num_coeffs=0;
    debug("Using shared memory coefficients");
    coefficients_sm=attach_coeffs_default_shared_area(coeffs_path,&num_coeffs);
  }else{
    int cfile_size;
    strcpy(coeffs_path,hack_file);
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
  }

  if (num_coeffs>0){
    num_coeffs=num_coeffs/sizeof(coefficient_t);
    int ccoeff;
    for (ccoeff = 0 ; ccoeff < num_coeffs;ccoeff++){
      ref = frequency_closest_pstate(coefficients_sm[ccoeff].pstate_ref);
      i = frequency_closest_pstate(coefficients_sm[ccoeff].pstate);
      if (frequency_is_valid_pstate(ref) && frequency_is_valid_pstate(i)){
				memcpy(&coefficients[ref][i],&coefficients_sm[ccoeff],sizeof(coefficient_t));
        //verbose_master(3,"initializing coeffs for ref: %d i: %d\n", ref, i);
      }
    }
  }
	basic_model_init=1;	
#if SHOW_DEBUGS
    for (ref = 0; ref < num_pstates; ref++)
    {
        for (i = 0; i < num_pstates; i++)
                debug("coefficient from ref: %d i: %d available: %d\n",
											ref, i, coefficients[ref][i].available);
    }
#endif
	return EAR_SUCCESS;
}


state_t energy_model_project_time(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_time)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff;
	if ((basic_model_init) && (valid_range(from_ps,to_ps))){
		coeff=&coefficients[from_ps][to_ps];
		if (coeff->available){
			double proj_cpi = em_common_project_cpi(signature, coeff);
			double freq_src = (double) coeff->pstate_ref;
			double freq_dst = (double) coeff->pstate;

			*proj_time = ((signature->time * proj_cpi) / signature->CPI) *
		   (freq_src / freq_dst);
		}else{
			*proj_time = 0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}


state_t energy_model_project_power(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_power)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff;
	if ((basic_model_init) && (valid_range(from_ps,to_ps))){
		coeff=&coefficients[from_ps][to_ps];
		if (coeff->available){
			*proj_power= (coeff->A * signature->DC_power) +
		   (coeff->B * signature->TPI) +
		   (coeff->C);
       debug("Power %lf = %lf x %lf + %lf x %lf + %lf", *proj_power, coeff->A, signature->DC_power, coeff->B, signature->TPI, coeff->C);
		}else{
			*proj_power=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}


uint energy_model_projection_available(ulong from_ps, ulong to_ps)
{
	return projection_available(from_ps, to_ps);
}


uint energy_model_any_projection_available()
{
  for (uint ps = 0; ps < num_pstates; ps++){
    for (uint pt = 0; pt < num_pstates; pt++){
      if ((ps != pt) && (projection_available(ps, pt)))
			{
				return 1;
			}
    }
  }
	return 0;
}


static int valid_range(ulong from_ps, ulong to_ps)
{
	if ((from_ps < num_pstates) && (to_ps < num_pstates)) return 1;
	else return 0;
}


static uint projection_available(ulong from_ps, ulong to_ps)
{
	if (valid_range(from_ps, to_ps))
	{
		if (coefficients[from_ps][to_ps].available)
		{
			return 1;
		}
	}

	return 0;
}
