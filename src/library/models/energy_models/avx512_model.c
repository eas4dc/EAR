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
static architecture_t arch;
static int avx512_pstate = 1, avx2_pstate = 1;


/** Returns whether the pair <from_ps, to_ps> is between the configured pstate list. */
static int valid_range(ulong from_ps, ulong to_ps);


/** Returns the total number of single and double AVX512 instructions normalized
 * by the total number of instructions contained in the input signature.
 * \pre my_app must be initialized. */
static double avx512_vpi(signature_t *my_app);


/** Computes the projection of the \ref signature loop time by using \ref coeff.
 * \pre All pointers must be initialized. Also freq_dst and signature's CPI must be non-zero. */
static double project_time(coefficient_t *coeff, signature_t *signature, ulong freq_src, ulong freq_dst);


/** Computes the projection of the \ref signature DC node power by using \ref coeffs.
 * \pre Input arguments must be initialized. */
static double project_power(coefficient_t *coeff, signature_t *signature);


/** Returns whether there exists a coefficients entry from \ref from_ps to \ref to_ps. */
static uint projection_available(ulong from_ps, ulong to_ps);


/** This function loads any information needed by the energy model.
 * It is called when the model just has been loaded. */
state_t energy_model_init(char *ear_etc_path, char *ear_tmp_path, architecture_t *arch_desc)
{
  int i, ref;
  char * hack_file = ear_getenv(HACK_EARL_COEFF_FILE);

	debug("Using avx512_model\n");

	num_pstates=arch_desc->pstates;

	copy_arch_desc(&arch, arch_desc);
	print_arch_desc(&arch);

	VERB_SET_EN(0);

	avx512_pstate=frequency_closest_pstate(arch.max_freq_avx512);
	avx2_pstate=frequency_closest_pstate(arch.max_freq_avx2);

	VERB_SET_EN(1);

	debug("Pstate for maximum freq avx512 %lu=%d Pstate for maximum freq avx2 %lu=%d",
				arch.max_freq_avx512,avx512_pstate,arch.max_freq_avx2,avx2_pstate);

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

  if (num_coeffs > 0){
      num_coeffs = num_coeffs / sizeof(coefficient_t);

      int ccoeff;
      for (ccoeff=0;ccoeff<num_coeffs;ccoeff++){
          ref=frequency_closest_pstate(coefficients_sm[ccoeff].pstate_ref);
          i=frequency_closest_pstate(coefficients_sm[ccoeff].pstate);
          if (frequency_is_valid_pstate(ref) && frequency_is_valid_pstate(i)){
              memcpy(&coefficients[ref][i],&coefficients_sm[ccoeff],sizeof(coefficient_t));
							//verbose_master(3,"initializing coeffs for ref: %d i: %d\n", ref, i);
          }
      }
  }
	basic_model_init=1;	
	return EAR_SUCCESS;
}


state_t energy_model_project_time(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_time)
{
	state_t st = EAR_SUCCESS;
	double ctime,time_nosimd,time_avx512 = 0,perc_avx512 = 0;
	coefficient_t *coeff,*avx512_coeffs;
	ulong pdest;

	if (from_ps == to_ps)
	{
		*proj_time = signature->time;
		return st;
	}

	if (basic_model_init && valid_range(from_ps, to_ps))
	{
		coeff = &coefficients[from_ps][to_ps];
		if (coeff->available)
		{
			time_nosimd = project_time(coeff, signature, coeff->pstate_ref, coeff->pstate);
      perc_avx512 = avx512_vpi(signature);

			// debug("Ptime from %lu to %lu : AVX 512: from pstate %lu perc = %.2f",
			//				from_ps,to_ps, (from_ps > to_ps) ? avx512_pstate : 1, perc_avx512);

      if (to_ps < avx512_pstate && perc_avx512 > 0.0)
			{
					/* from_ps > to_ps means from lower cpufreq to high cpufreq */
					if (from_ps > to_ps) pdest = avx512_pstate;
					else pdest = 1;
					unsigned long nominal = frequency_pstate_to_freq(pdest);
					avx512_coeffs = &coefficients[from_ps][pdest];
          time_avx512 = project_time(avx512_coeffs,signature,coeff->pstate_ref,nominal);
      } else {
				perc_avx512 = 0;
			}

			ctime = time_nosimd*(1-perc_avx512)+time_avx512*perc_avx512;
			*proj_time = ctime;

			debug("time projection from %lu to %lu time_nosimd %lf time avx512 %lf perc_avx512 %lf TIME %lf",from_ps,to_ps,
				time_nosimd,time_avx512,perc_avx512,ctime);
		} else {
			*proj_time = 0;
			st = EAR_ERROR;
		}
	} else st = EAR_ERROR;

	return st;
}


state_t energy_model_project_power(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_power)
{
	state_t st=EAR_SUCCESS;
	coefficient_t *coeff,*avx512_coeffs;
	double power_nosimd,power_avx512=0,cpower;
	double perc_avx512=0;
	ulong pdest;

  debug("projct power init %d valid %d",basic_model_init,valid_range(from_ps,to_ps));

	if (from_ps == to_ps) {
		*proj_power = signature->DC_power;
		return st;
	}

	if (basic_model_init && valid_range(from_ps, to_ps))
	{
		coeff = &coefficients[from_ps][to_ps];
		if (coeff->available)
		{
				power_nosimd = project_power(coeff, signature);
				// Is this <= or >= ?? :(
				perc_avx512 = avx512_vpi(signature);	

				// debug("PPower from %lu to %lu : AVX 512: from pstate %lu perc = %.2f",
				//				from_ps,to_ps,(from_ps > to_ps)?avx512_pstate:1,perc_avx512);

				if ((to_ps < avx512_pstate) && (perc_avx512 > 0.0)){
					/* from_ps > to_ps means from lower cpufreq to high cpufreq */
          if (from_ps > to_ps) pdest = avx512_pstate;
          else pdest = avx512_pstate;
					avx512_coeffs = &coefficients[from_ps][pdest];
					power_avx512 = project_power(avx512_coeffs,signature);
				}else{
					perc_avx512 = 0;
				}
				cpower = power_nosimd*(1-perc_avx512) + power_avx512*perc_avx512;
				*proj_power = cpower;
				debug("power projection from %lu to %lu power_nosimd %lf power avx512 %lf perc_avx512 %lf POWER %lf",from_ps,to_ps,
				power_nosimd,power_avx512,perc_avx512,cpower);
		} else
		{
			debug("Coeffs from %lu to %lu not available",from_ps,to_ps);
			*proj_power = 0;
			st = EAR_ERROR;
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
	for (uint ps = 0; ps < num_pstates; ps++)
	{
		for (uint pt = 0; pt < num_pstates; pt++)
		{
			if (ps != pt && projection_available(ps, pt)) return 1;	
		}
	}

	return 0;
}


static int valid_range(ulong from_ps, ulong to_ps)
{
	if ((from_ps < num_pstates) && (to_ps < num_pstates)) return 1;
	else return 0;
}


static double avx512_vpi(signature_t *my_app)
{
	return (double) ((my_app->FLOPS[3]/(ullong) 16)+(my_app->FLOPS[7]/(ullong)8))/(double)my_app->instructions;
}


static double project_time(coefficient_t *coeff, signature_t *signature, ulong freq_src, ulong freq_dst)
{
	debug("From %lu to %lu",freq_src,freq_dst);
	double pcpi = em_common_project_cpi(signature, coeff);
	debug("((time %lf new_cpi %lf) / orig_cpi %lf) * (freq src %lu / freq_dst %lu)",signature->time,pcpi,signature->CPI,freq_src,freq_dst);
	return ((signature->time * pcpi) / signature->CPI) * ((double) freq_src / (double) freq_dst);
}


static double project_power(coefficient_t *coeff,signature_t *signature)
{
	return (coeff->A * signature->DC_power) + (coeff->B * signature->TPI) + (coeff->C);	
}


static uint projection_available(ulong from_ps, ulong to_ps)
{
	//verbose_master(2, "projection_available test: from %lu to %lu aval %lu", from_ps, to_ps, coefficients[from_ps][to_ps].available);
	if (valid_range(from_ps, to_ps))
	{
		return coefficients[from_ps][to_ps].available;
	}

	return 0;
}
