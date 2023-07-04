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

#define _GNU_SOURCE
#include <sched.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// #define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <common/hardware/topology.h>
#include <common/types/projection.h>

#include <management/cpufreq/frequency.h>

#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/api/clasify.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/imc_policy_support.h>

#define CPU_SAVINGS_LVL 0

extern uint dyn_unc;
extern polctx_t my_pol_ctx;
extern ear_classify_t phases_limits;

state_t compute_reference(polctx_t *c,signature_t *my_app,ulong *curr_freq,ulong *def_freq,ulong *freq_ref,double *time_ref,
        double *power_ref)
{
    ulong def_pstate, curr_pstate;

    def_pstate = frequency_closest_pstate(def_freq[0]);
    curr_pstate = frequency_closest_pstate(curr_freq[0]);
    if (curr_freq[0] != def_freq[0]) // Use configuration when available
    {
        debug("curr_freq[0] != def_freq[0] %lu != %lu", curr_freq[0], def_freq[0]);
        if (projection_available(curr_pstate,def_pstate)==EAR_SUCCESS)
        {
						verbose_master(3, "Using projections for references");
            project_power(my_app,curr_pstate,def_pstate,power_ref);
            project_time(my_app,curr_pstate,def_pstate,time_ref);
            freq_ref[0] = def_freq[0];
        } else {
            *time_ref = my_app->time;
            *power_ref = my_app->DC_power;
            freq_ref[0] = curr_freq[0];
        }
    }else{ 
        /* And we are the nominal freq */
        *time_ref = my_app->time;
        *power_ref = my_app->DC_power;
        freq_ref[0] = curr_freq[0];
    }
    return EAR_SUCCESS;
}

double get_time_nominal(signature_t *my_app)
{
  double t = my_app->time;
  uint curr, target;
  if (my_app->def_f != frequency_get_nominal_freq()){
    target = frequency_get_nominal_pstate();
    curr   = frequency_closest_pstate(my_app->def_f);
    project_time(my_app, curr, target, &t);
  }
  verbose_master(CPU_SAVINGS_LVL,"Policy savins[%s]: Projecting reference time from %u to %u %lf", node_name, curr, target, t);
  return t;
}

double get_power_nominal(signature_t *my_app)
{
  double p = my_app->DC_power;
  uint curr, target;
  if (my_app->def_f != frequency_get_nominal_freq()){
    target = frequency_get_nominal_pstate();
    curr   = frequency_closest_pstate(my_app->def_f);
    project_power(my_app, curr, target, &p);
  }
  verbose_master(CPU_SAVINGS_LVL,"Policy savins[%s]: Projecting reference power from %u to %u %lf", node_name, curr, target, p);
  return p;
}

double get_time_projected(signature_t *my_app, ulong f)
{
  double t = my_app->time;
  uint curr, target;
  if (f != my_app->def_f){
    curr   = frequency_closest_pstate(my_app->def_f);
    target = frequency_closest_pstate(f);
    project_time(my_app, curr, target, &t);
  }
  //verbose(0,"Policy savins[%s] Projecting from %lu-%u to %lu-%u time %lf (ref %.3lf)", node_name, my_app->def_f, curr, f,target, t, my_app->time);
  return (float)t;
}
double get_power_projected(signature_t *my_app, ulong f)
{
  double p = my_app->DC_power;
  uint curr, target;
  if (f != my_app->def_f) {
    curr   = frequency_closest_pstate(my_app->def_f);
    target = frequency_closest_pstate(f);
    project_power(my_app, curr, target, &p);
  }
  //verbose(0," Policy savins[%s] Projecting from %lu-%u to %lu-%u power %lf (ref %.2lf)", node_name, my_app->def_f, curr, f, target, p, my_app->DC_power);
  return p;
}

state_t compute_cpu_freq_min_energy(polctx_t *c, signature_t *my_app, ulong freq_ref, double time_ref, double power_ref,
        double penalty, ulong curr_pstate, ulong minp, ulong maxp, ulong *newf)
{
    double vpi;
    double energy_ref,best_solution,time_max;
    double power_proj,time_proj,energy_proj;
    ulong i,from;

    ulong best_freq = freq_ref;

    energy_ref		= power_ref*time_ref;
    best_solution	= energy_ref;
    time_max 			= time_ref + (time_ref * penalty);	
    from 					= curr_pstate;	
    compute_sig_vpi(&vpi,my_app);
    verbose_master(3, "CPUfreq algorithm for min_energy timeref %lf powerref %lf energy %lf F=%lu VPI %lf. Pstates %lu...%lu",time_ref,power_ref,
            energy_ref,best_freq,vpi,minp,maxp);
    for (i = minp; i < maxp;i++)
    {
        if (projection_available(from,i) == EAR_SUCCESS)
        {
            project_power(my_app,from,i,&power_proj);
            project_time(my_app,from,i,&time_proj);
            projection_set(i,time_proj,power_proj);
            energy_proj = power_proj*time_proj;

            verbose_master(3,"projected from %lu to %lu\t time: %.2lf\t power: %.2lf energy: %.2lf",
                    from, i, time_proj, power_proj, energy_proj);

            if ((energy_proj < best_solution) && (time_proj < time_max)) {
                best_freq = frequency_pstate_to_freq(i);
                verbose(3,"new best solution found %lu",best_freq);
                best_solution = energy_proj;
            }
        }
    }
    *newf = best_freq;
    return EAR_SUCCESS;
}

state_t compute_cpu_freq_min_time(signature_t *my_app, int min_pstate, double time_ref,
        double min_eff_gain, ulong curr_pstate, ulong best_pstate, ulong best_freq, ulong def_freq, ulong *newf){
    int i;
    uint try_next;
    double time_current, power_proj, time_proj, freq_gain, perf_gain, vpi;
    ulong freq_ref;

    compute_sig_vpi(&vpi,my_app);
    // error al compilar con verbose_master :( error: identifier "masters_info" is undefined
    verbose_master(3, "CPUfreq algorithm for min_time. timeref %lf; pstate %lu; F %lu; deffreq %lu; VPI %lf",
            time_ref, curr_pstate, best_freq, def_freq, vpi);

    try_next = 1;
    i = best_pstate - 1;
    time_current = time_ref;
    while(try_next && (i >= min_pstate))
    {
        if (projection_available(curr_pstate,i)==EAR_SUCCESS)
        {
            verbose_master(3, "Looking for pstate %d",i);
            project_power(my_app, curr_pstate, i, &power_proj);
            project_time(my_app, curr_pstate, i, &time_proj);
            projection_set(i, time_proj, power_proj);
            freq_ref = frequency_pstate_to_freq(i);
            freq_gain = min_eff_gain * (double)(freq_ref - best_freq) / (double)best_freq;
            perf_gain = (time_current - time_proj) / time_current;
            if (min_eff_gain < 0.5){
            verbose_master(3, "%lu to %d time %lf proj time %lf freq gain %lf perf_gain %lf", curr_pstate, i, my_app->time, time_proj,
                    freq_gain,perf_gain);
            }else{
            verbose_master(3, "%lu to %d time %lf proj time %lf freq gain %lf perf_gain %lf", curr_pstate, i, my_app->time, time_proj,
                    freq_gain,perf_gain);
            }
            // OK
            if (perf_gain>=freq_gain)
            {
                verbose(3, "New best solution found: Best freq: %lu; Best pstate: %d; Current time ref: %lf",
                        freq_ref, i, time_proj);
                best_freq=freq_ref;
                best_pstate=i;
                time_current = time_proj;
                //i--;
            }
            #if 0
            else
            {
                try_next = 0;
            }
            #endif
        } // Projections available
        else{
            try_next=0;
        }
        i--;
    }
    /* Controlar la freq por power cap, si capado poner GREEDY, gestionar req-f */
    if (best_freq<def_freq) best_freq=def_freq;
    *newf = best_freq;
    verbose_master(3, "*new_freq=%lu", *newf);

    return EAR_SUCCESS;
}

uint cpu_supp_try_boost_cpu_freq(int nproc, uint *critical_path, ulong *freqs, int min_pstate){
    uint belongs_to_cp = critical_path[nproc];
    int pstate = frequency_closest_pstate(freqs[nproc]);
    verbosen_master(2, "Checking for boost process %d: Critical path ? %u | pstate ? %d | min_pstate ? %d ",
            nproc, belongs_to_cp, pstate, min_pstate);
    if (belongs_to_cp && pstate == min_pstate) {
        freqs[nproc] = frequency_pstate_to_freq(0);
        verbose_master(2, "%sOK%s", COL_GRE, COL_CLR);
        return 1;
    }
    verbose_master(2, "%sFALSE%s", COL_RED, COL_CLR);
    return 0;
}

int signatures_different(signature_t *s1, signature_t *s2,float p)
{

        if (s1->CPI == 0 || s2->CPI == 0 || s1->GBS == 0 || s2->GBS == 0) {
        verbose_master(2, "%sWARNING%s some signature have a 0 value CPI (%.2lfvs%.2lf) GBS (%.2lfvs%.2lf)",
                COL_RED, COL_CLR, s1->CPI, s2->CPI, s1->GBS, s2->GBS);
        return 0;
    }

#if FAKE_SIGNATURES_DIFFERENT
        verbose_master(2, "Signatures different fakely forced.");
        return 1;
#endif

	/* If one of them is doing some computation, we check. */
    if ((s1->GBS > phases_limits.gbs_busy_waiting) || (s2->GBS > phases_limits.gbs_busy_waiting)){
				/* If the CPI is different we check if the classification is also different */
        if (!equal_with_th(s1->CPI, s2->CPI, p)) {
            uint s1_cbound, s2_cbound;
            is_cpu_bound(s1, lib_shared_region->num_cpus, &s1_cbound);
            is_cpu_bound(s2, lib_shared_region->num_cpus, &s2_cbound);
            if (s1_cbound != s2_cbound) {
                verbose_master(2, "CPI (%.2lfvs%.2lf) and cpu bound phase changed from %u to %u",
                        s1->CPI, s2->CPI, s1_cbound, s2_cbound);
                return 1;
            }
            verbose_master(2, "CPIs different (%.2lfvs%.2lf) but same cpu behaviour (%uvs%u)",
                    s1->CPI, s2->CPI, s1_cbound, s2_cbound);
        }
				/* If Mem. Band is different, we double check */
				/* Compare if one is GBS_BUSY_WAITING and not the other */
        uint s1_mbound, s2_mbound;
				s1_mbound  = s1->GBS < phases_limits.gbs_busy_waiting;
				s2_mbound  = s2->GBS < phases_limits.gbs_busy_waiting;

				if (s1_mbound != s2_mbound) {
					verbose_master(2, "One signature has memory bandwidth below busy waiting threshold (%f GB/s) %f/%f",
              phases_limits.gbs_busy_waiting, s1->GBS, s2->GBS);

					return 1;
				}

				/* If one is MEM bound and not the other they are not the same */
        is_mem_bound(s1, lib_shared_region->num_cpus, &s1_mbound); /* Node */
        is_mem_bound(s2, lib_shared_region->num_cpus, &s2_mbound);
        if (s1_mbound != s2_mbound) {
						verbose_master(2, "One signature is memory bound (GB/s) %f/%f (CPI) %f/%f",
                s1->GBS, s2->GBS, s1->CPI, s2->CPI);
            return 1;
        }

				/* Compare the absolute value */
        if (abs((int) s1->GBS - (int) s2->GBS) > MAX_GBS_DIFF) {
						verbose_master(2, "Memory bandwidth differ more far than %d GB/s: %f/%f",
                MAX_GBS_DIFF, s1->GBS, s2->GBS);
            return 1;
        }
    }


#if USE_GPUS
    for (int i = 0; i< s1->gpu_sig.num_gpus; i ++){
        //if (!equal_with_th_ul(s1->gpu_sig.gpu_data[i].GPU_util, s2->gpu_sig.gpu_data[i].GPU_util,p)){ 
        if (abs((int)s1->gpu_sig.gpu_data[i].GPU_util - (int) s2->gpu_sig.gpu_data[i].GPU_util) > 10){
          verbose_master(2, "GPU util[%d] %lu/%lu", i, s1->gpu_sig.gpu_data[i].GPU_util, s2->gpu_sig.gpu_data[i].GPU_util);
          return 1;
        }
        //if (!equal_with_th_ul(s1->gpu_sig.gpu_data[i].GPU_mem_util, s2->gpu_sig.gpu_data[i].GPU_mem_util,p)){ 
        if (abs((int)s1->gpu_sig.gpu_data[i].GPU_mem_util - (int)s2->gpu_sig.gpu_data[i].GPU_mem_util) > 10){
           verbose_master(2, "GPU mem util[%d] %lu/%lu", i, s1->gpu_sig.gpu_data[i].GPU_mem_util, s2->gpu_sig.gpu_data[i].GPU_mem_util);
           return 1;
        }
    }
#endif
    return 0;
}

int are_default_settings(node_freqs_t *freqs,node_freqs_t *def)
{
    int i, sid = 0;
    i = 0;
    int num_processes = lib_shared_region->num_processes;
    while ((i<num_processes) && (freqs->cpu_freq[i] == def->cpu_freq[i])) i++;
    if (i< num_processes) return 0;
    if (dyn_unc){
		for (sid = 0;sid < arch_desc.top.socket_count; sid++){
        if (freqs->imc_freq[sid*IMC_VAL+IMC_MAX] != def->imc_freq[sid*IMC_VAL+IMC_MAX]) return 0;
        if (freqs->imc_freq[sid*IMC_VAL+IMC_MIN] != def->imc_freq[sid*IMC_VAL+IMC_MIN]) return 0;
		}
    }
/* GPU freqs are ignored for now since they are set explicitly at each policy execution based on utilization */
#if 0
#if USE_GPUS
    if (my_pol_ctx.num_gpus){
        for (i=0; i< my_pol_ctx.num_gpus;i++){
            if (freqs->gpu_freq[i] != def->gpu_freq[i]) return 0;
            /* GPU mem pending */
        }
    }
#endif
#endif
    return 1;  
}

void set_default_settings(node_freqs_t *freqs, node_freqs_t * def) 
{
    int i, sid = 0;
    int num_processes = lib_shared_region->num_processes;
    freqs->cpu_freq[0] = def->cpu_freq[0];
    for (i=1;i<num_processes;i++) freqs->cpu_freq[i] = freqs->cpu_freq[0];
    if (dyn_unc){
		for (sid = 0;sid < arch_desc.top.socket_count; sid++){
        freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = def->imc_freq[sid*IMC_VAL+IMC_MAX];
        freqs->imc_freq[sid*IMC_VAL+IMC_MIN] = def->imc_freq[sid*IMC_VAL+IMC_MIN];
		}
    }
/* GPU freqs are ignored for now since they are set explicitly at each policy execution based on utilization */

}

void verbose_node_freqs(int vl, node_freqs_t *freqs)
{
    int i;
    int sid = 0;
    if (freqs == NULL) return;
    int num_processes = lib_shared_region->num_processes;
    for (i=0;i<num_processes;i++){
        verbosen_master(vl,"CPU[%d] = %.2f ",i,(float)freqs->cpu_freq[i]/1000000.0);
    }
		for (sid = 0;sid < arch_desc.top.socket_count; sid++){
    	verbose_master(vl,"\n IMC %d (%lu-%lu)",sid,freqs->imc_freq[sid*IMC_VAL+IMC_MAX],freqs->imc_freq[sid*IMC_VAL+IMC_MIN]);
		}
#if USE_GPUS
    for (i=0; i< my_pol_ctx.num_gpus;i++){
        verbosen_master(vl,"GPU[%d] = %.2f ",i,(float)freqs->gpu_freq[i]/1000000.0);
    }
    verbose_master(vl," ");
#endif

}

void node_freqs_alloc(node_freqs_t *node_freq)
{
    node_freq->cpu_freq = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    node_freq->imc_freq = calloc(MAX_SOCKETS_SUPPORTED*IMC_VAL,sizeof(ulong));
#if USE_GPUS
    node_freq->gpu_freq = calloc(MAX_GPUS_SUPPORTED,sizeof(ulong));
    node_freq->gpu_mem_freq = calloc(MAX_GPUS_SUPPORTED,sizeof(ulong));
#endif
}

void node_freqs_free(node_freqs_t *node_freq)
{
    free(node_freq->cpu_freq);
    free(node_freq->imc_freq);
#if USE_GPUS
    free(node_freq->gpu_freq);
    free(node_freq->gpu_mem_freq);
#endif
}

void node_freqs_copy(node_freqs_t * dst, node_freqs_t *src)
{
    if ((dst == NULL) || (src == NULL)) return;
    if ((dst->cpu_freq == NULL) || (src->cpu_freq == NULL)) return;
    memcpy(dst->cpu_freq,src->cpu_freq,sizeof(ulong)*MAX_CPUS_SUPPORTED);
    if ((dst->imc_freq == NULL) || (src->imc_freq == NULL)) return;
    memcpy(dst->imc_freq,src->imc_freq,MAX_SOCKETS_SUPPORTED*IMC_VAL*sizeof(ulong));
#if USE_GPUS
    memcpy(dst->gpu_freq,src->gpu_freq,MAX_GPUS_SUPPORTED*sizeof(ulong));
    memcpy(dst->gpu_mem_freq,src->gpu_mem_freq,MAX_GPUS_SUPPORTED*sizeof(ulong));
#endif
}

state_t set_all_cores(ulong *freqs, int len, ulong freq_val) {
    verbose_master(3, "Setting %lu freq. value to %d cores...", freq_val, len);
    for (int i = 0; i < len; i++) {
        freqs[i] = freq_val;
    }
    return EAR_SUCCESS;
}

state_t copy_cpufreq_sel(ulong *to, ulong *from, size_t size) {
    memcpy(to, from, size);
    return EAR_SUCCESS;
}

ulong node_freqs_avgcpufreq(ulong *f)
{
	ulong ftotal = 0;
	ulong ctotal = 0;
	ulong flocal;
	ulong ccount = ccount = arch_desc.top.cpu_count;
	for (uint p = 0 ;p<lib_shared_region->num_processes; p++){
		flocal = f[p];
		cpu_set_t m = sig_shared_region[p].cpu_mask;
		for (uint i=0;i<ccount; i++){
			if (CPU_ISSET(i,&m)){
				//verbosen_master(2,"cpu[%d]=%lu ",i,flocal);
				ctotal++;
				ftotal += flocal;
			}
		}
	}
	//verbose_master(2,"Computing a total of %lu cpus and f = %lu, avg %lu",ctotal,ftotal, ftotal/ctotal);
	return ftotal / ctotal;
}
/*
 *   ulong *cpu_freq;
 *     ulong *imc_freq;
 *       ulong *gpu_freq;
 *         ulong *gpu_mem_freq;
 */

static uint node_dom_freqs_are_diff(ulong *v1, ulong *v2, uint l)
{
	uint i = 0;
	while ((i < l) && (v1[i] == v2[i])) i++;
	if (i < l) return 1;
	return 0;
}

uint node_freqs_are_diff(uint flag, node_freqs_t * nf1, node_freqs_t *nf2)
{
	switch (flag){
	case DOM_CPU:
		return node_dom_freqs_are_diff(nf1->cpu_freq, nf2->cpu_freq, MAX_CPUS_SUPPORTED);
		break;
	case DOM_MEM:
		return node_dom_freqs_are_diff(nf1->imc_freq, nf2->imc_freq, MAX_SOCKETS_SUPPORTED);
		break;
  #if USE_GPUS
	case DOM_GPU:
		return 1;
		break;
	case DOM_GPU_MEM:
		return 0;
		break;
  #endif
	default: return 0;
	}
	return 0;
}

ulong avg_to_khz(ulong freq_khz)
{
    ulong newf;
    float ff;

    ff = roundf((float) freq_khz / 100000.0);
    newf = (ulong) ((ff / 10.0) * 1000000);
    return newf;
}
