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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <stdio.h>
#include <stdlib.h>
//#define SHOW_DEBUGS 1
#include <common/states.h>
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
static uint basic_model_init=0;

static int valid_range(ulong from,ulong to)
{
	if ((from<num_pstates) && (to<num_pstates)) return 1;
	else return 0;
}

/* This function loads any information needed by the energy model */
state_t model_init(char *etc,char *tmp,architecture_t *myarch)
{
  char *hack_file = ear_getenv(HACK_EARL_COEFF_FILE);
  int i, ref;

	debug("Using basic_model\n");
	num_pstates = (uint)myarch->pstates;
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
  get_coeffs_default_path(tmp,coeffs_path);
  num_coeffs=0;
  coefficients_sm=attach_coeffs_default_shared_area(coeffs_path,&num_coeffs);
  #endif
  if (hack_file == NULL){
    get_coeffs_default_path(tmp,coeffs_path);
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
        debug("initializing coeffs for ref: %d i: %d\n", ref, i);
      }
    }
  }
	basic_model_init=1;	
#if SHOW_DEBUGS
    for (ref = 0; ref < num_pstates; ref++)
    {
        for (i = 0; i < num_pstates; i++)
                debug("coefficient from ref: %d i: %d available: %d\n", ref, i, coefficients[ref][i].available);
    }
#endif
	return EAR_SUCCESS;
}

double default_project_cpi(signature_t *sign, coefficient_t *coeff)
{
	return (coeff->D * sign->CPI) +
		   (coeff->E * sign->TPI) +
		   (coeff->F);
}

state_t model_project_time(signature_t *sign,ulong from,ulong to,double *ptime)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff;
	if ((basic_model_init) && (valid_range(from,to))){
		coeff=&coefficients[from][to];
		if (coeff->available){
			double proj_cpi = default_project_cpi(sign, coeff);
			double freq_src = (double) coeff->pstate_ref;
			double freq_dst = (double) coeff->pstate;

			*ptime= ((sign->time * proj_cpi) / sign->CPI) *
		   (freq_src / freq_dst);
		}else{
			*ptime=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

state_t model_project_power(signature_t *sign, ulong from,ulong to,double *ppower)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff;
	if ((basic_model_init) && (valid_range(from,to))){
		coeff=&coefficients[from][to];
		if (coeff->available){
			*ppower= (coeff->A * sign->DC_power) +
		   (coeff->B * sign->TPI) +
		   (coeff->C);
       debug("Power %lf = %lf x %lf + %lf x %lf + %lf", *ppower, coeff->A, sign->DC_power, coeff->B, sign->TPI, coeff->C);
		}else{
			*ppower=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

state_t model_projection_available(ulong from,ulong to)
{
	if (valid_range(from, to) && coefficients[from][to].available) return EAR_SUCCESS;
	else return EAR_ERROR;
}

state_t model_projections_available()
{
  uint ready = 0;
  for (uint ps = 0; ps < num_pstates && !ready; ps++){
    for (uint pt = 0; pt < num_pstates && !ready; pt++){
      if ((ps != pt) && (model_projection_available(ps, pt) == EAR_SUCCESS)) ready = 1;
    }
  }
  if (ready) return EAR_SUCCESS;
  return EAR_ERROR;
}

