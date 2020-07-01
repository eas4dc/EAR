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

#include <stdio.h>
#include <stdlib.h>
#include <common/states.h>
#include <common/types/signature.h>
#include <daemon/shared_configuration.h>
#include <common/hardware/frequency.h>
#include <common/hardware/architecture.h>

#include <library/models/models_api.h>


static coefficient_t **coefficients;
static coefficient_t *coefficients_sm;
static int num_coeffs;
static uint num_pstates;
static uint basic_model_init=0;
static architecture_t arch;
static int avx512_pstate=1,avx2_pstate=1;

#ifdef SHOW_DEBUGS
#define debug(...) fprintf(stderr, __VA_ARGS__); 
#else
#define debug(...) 
#endif

static int valid_range(ulong from,ulong to)
{
	if ((from<num_pstates) && (to<num_pstates)) return 1;
	else return 0;
}

/* This function loads any information needed by the energy model */
state_t model_init(char *etc,char *tmp,architecture_t *myarch)
{
  char coeff_file[128];
  char coeff_default_file[128];
  char coeff_file_fn[128];
  int begin_pstate, end_pstate;
  int i, ref;

	debug("Using basic_model\n");
	num_pstates=myarch->pstates;
	copy_arch_desc(&arch,myarch);
	print_arch_desc(&arch);
	avx512_pstate=frequency_closest_pstate(arch.max_freq_avx512);
	avx2_pstate=frequency_closest_pstate(arch.max_freq_avx2);
	debug("Pstate for maximum freq avx512 %d Pstate for maximum freq avx2 %d",avx512_pstate,avx2_pstate);

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
	double pcpi=proj_cpi(coeff,sign);
	return ((sign->time * pcpi) / sign->CPI) * ((double)freq_src / (double)freq_dst);
	
}

state_t model_project_time(signature_t *sign,ulong from,ulong to,double *ptime)
{
	state_t st=EAR_SUCCESS;
	double ctime,time_nosimd,time_avx2=0,time_avx512=0,perc_avx512=0,perc_avx2=0;
	coefficient_t *coeff,*avx512_coeffs;
	unsigned long nominal;
	if ((basic_model_init) && (valid_range(from,to))){
		coeff=&coefficients[from][to];
		if (coeff->available){
			time_nosimd=proj_time(coeff,sign,coeff->pstate_ref,coeff->pstate);
      if (to<=avx512_pstate){
					unsigned long nominal=frequency_get_nominal_freq();
					avx512_coeffs=&coefficients[from][1];
          perc_avx512=avx512_vpi(sign);
          time_avx512=proj_time(avx512_coeffs,sign,coeff->pstate_ref,nominal);
      }
			ctime=time_nosimd*(1-perc_avx512)+time_avx512*perc_avx512;
			*ptime=ctime;
		}else{
			*ptime=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

/*
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

*/

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
	double power_nosimd,power_avx2=0,power_avx512=0,cpower;
	double perc_avx512=0,perc_avx2=0;
	if ((basic_model_init) && (valid_range(from,to))){
		coeff=&coefficients[from][to];
		if (coeff->available){
				power_nosimd=proj_power(coeff,sign);
				// Is this <= or >= ?? :(
				if (to<=avx512_pstate){
					avx512_coeffs=&coefficients[from][1];
					perc_avx512=avx512_vpi(sign);	
					power_avx512=proj_power(avx512_coeffs,sign);
				}
				cpower=power_nosimd*(1-perc_avx512)+power_avx512*perc_avx512;
				*ppower=cpower;
		}else{
			*ppower=0;
			st=EAR_ERROR;
		}
	}else st=EAR_ERROR;
	return st;
}

state_t model_projection_available(ulong from,ulong to)
{
	if (coefficients[from][to].available) return EAR_SUCCESS;
	else return EAR_ERROR;
}

