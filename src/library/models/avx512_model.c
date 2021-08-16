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

#include <stdio.h>
#include <stdlib.h>
//#define SHOW_DEBUGS 1
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/signature.h>
#include <common/hardware/architecture.h>
#include <management/cpufreq/frequency.h>
#include <daemon/shared_configuration.h>
#include <library/models/models_api.h>

static coefficient_t **coefficients;
static coefficient_t *coefficients_sm;
static int num_coeffs;
static uint num_pstates;
static uint basic_model_init=0;
static architecture_t arch;
static int avx512_pstate=1,avx2_pstate=1;

static int valid_range(ulong from,ulong to)
{
	if ((from<num_pstates) && (to<num_pstates)) return 1;
	else return 0;
}

/* This function loads any information needed by the energy model */
state_t model_init(char *etc,char *tmp,architecture_t *myarch)
{
  int i, ref;

	debug("Using avx512_model\n");
	num_pstates=myarch->pstates;
	copy_arch_desc(&arch,myarch);
	print_arch_desc(&arch);
	VERB_SET_EN(0);
	avx512_pstate=frequency_closest_pstate(arch.max_freq_avx512);
	avx2_pstate=frequency_closest_pstate(arch.max_freq_avx2);
	VERB_SET_EN(1);
	debug("Pstate for maximum freq avx512 %lu=%d Pstate for maximum freq avx2 %lu=%d",arch.max_freq_avx512,avx512_pstate,arch.max_freq_avx2,avx2_pstate);

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
  get_coeffs_default_path(tmp,coeffs_path);
  num_coeffs=0;
  coefficients_sm=attach_coeffs_default_shared_area(coeffs_path,&num_coeffs);
  if (num_coeffs>0){
    num_coeffs=num_coeffs/sizeof(coefficient_t);
    int ccoeff;
    for (ccoeff=0;ccoeff<num_coeffs;ccoeff++){
      ref=frequency_closest_pstate(coefficients_sm[ccoeff].pstate_ref);
      i=frequency_closest_pstate(coefficients_sm[ccoeff].pstate);
      if (frequency_is_valid_pstate(ref) && frequency_is_valid_pstate(i)){
				memcpy(&coefficients[ref][i],&coefficients_sm[ccoeff],sizeof(coefficient_t));
      }
    }
  }
	basic_model_init=1;	
	return EAR_SUCCESS;
}
double avx512_vpi(signature_t *my_app);
double proj_cpi(coefficient_t *coeff,signature_t *sign)
{
	return (coeff->D * sign->CPI) +
		   (coeff->E * sign->TPI) +
		   (coeff->F);
}

double proj_time(coefficient_t *coeff,signature_t *sign,unsigned long freq_src,unsigned long freq_dst)
{
	debug("From %lu to %lu",freq_src,freq_dst);
	double pcpi = proj_cpi(coeff,sign);
	debug("((time %lf new_cpi %lf) / orig_cpi %lf) * (freq src %lu / freq_dst %lu)",sign->time,pcpi,sign->CPI,freq_src,freq_dst);
	return ((sign->time * pcpi) / sign->CPI) * ((double)freq_src / (double)freq_dst);
	
}

state_t model_project_time(signature_t *sign,ulong from,ulong to,double *ptime)
{
	state_t st = EAR_SUCCESS;
	double ctime,time_nosimd,time_avx512 = 0,perc_avx512 = 0;
	coefficient_t *coeff,*avx512_coeffs;
	ulong pdest;
	if (from == to ){
		*ptime = sign->time;
		return EAR_SUCCESS;
	}
	if ((basic_model_init) && (valid_range(from,to))){
		coeff = &coefficients[from][to];
		if (coeff->available){
			time_nosimd = proj_time(coeff,sign,coeff->pstate_ref,coeff->pstate);
      perc_avx512 = avx512_vpi(sign);
			//debug("Ptime from %lu to %lu : AVX 512: from pstate %lu perc = %.2f",from,to,(from > to)?avx512_pstate:1,perc_avx512);
      if ((to < avx512_pstate) && (perc_avx512 > 0.0)){
					/* from > to means from lower cpufreq to high cpufreq */
					if (from > to) pdest = avx512_pstate;
					else pdest = 1;
					unsigned long nominal = frequency_pstate_to_freq(pdest);
					avx512_coeffs = &coefficients[from][pdest];
          time_avx512 = proj_time(avx512_coeffs,sign,coeff->pstate_ref,nominal);
      }else{
				perc_avx512 = 0;
			}
			ctime = time_nosimd*(1-perc_avx512)+time_avx512*perc_avx512;
			*ptime = ctime;
			debug("time projection from %lu to %lu time_nosimd %lf time avx512 %lf perc_avx512 %lf TIME %lf",from,to,
				time_nosimd,time_avx512,perc_avx512,ctime);
		}else{
			*ptime = 0;
			st = EAR_ERROR;
		}
	}else st = EAR_ERROR;
	return st;
}


double avx512_vpi(signature_t *my_app)
{
	return (double)((my_app->FLOPS[3]/(unsigned long long)16)+(my_app->FLOPS[7]/(unsigned long long)8))/(double)my_app->instructions;
}
double proj_power(coefficient_t *coeff,signature_t *sign)
{
	return (coeff->A * sign->DC_power) + (coeff->B * sign->TPI) + (coeff->C);	
}
state_t model_project_power(signature_t *sign, ulong from,ulong to,double *ppower)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff,*avx512_coeffs;
	double power_nosimd,power_avx512=0,cpower;
	double perc_avx512=0;
	ulong pdest;
  debug("projct power init %d valid %d",basic_model_init,valid_range(from,to));
	if (from == to){
		*ppower = sign->DC_power;
		return EAR_SUCCESS;
	}
	if ((basic_model_init) && (valid_range(from,to))){
		coeff = &coefficients[from][to];
		if (coeff->available){
				power_nosimd = proj_power(coeff,sign);
				// Is this <= or >= ?? :(
				perc_avx512 = avx512_vpi(sign);	
				//debug("PPower from %lu to %lu : AVX 512: from pstate %lu perc = %.2f",from,to,(from > to)?avx512_pstate:1,perc_avx512);
				if ((to < avx512_pstate) && (perc_avx512 > 0.0)){
					/* from > to means from lower cpufreq to high cpufreq */
          if (from > to) pdest = avx512_pstate;
          else pdest = avx512_pstate;
					avx512_coeffs = &coefficients[from][pdest];
					power_avx512 = proj_power(avx512_coeffs,sign);
				}else{
					perc_avx512 = 0;
				}
				cpower = power_nosimd*(1-perc_avx512) + power_avx512*perc_avx512;
				*ppower = cpower;
				debug("power projection from %lu to %lu power_nosimd %lf power avx512 %lf perc_avx512 %lf POWER %lf",from,to,
				power_nosimd,power_avx512,perc_avx512,cpower);
		}else{
			debug("Coeffs from %lu to %lu not available",from,to);
			*ppower = 0;
			st = EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

state_t model_projection_available(ulong from,ulong to)
{
	if (coefficients[from][to].available) return EAR_SUCCESS;
	else return EAR_ERROR;
}

