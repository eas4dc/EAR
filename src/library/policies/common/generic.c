/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


// #define SHOW_DEBUGS 1

#include <library/policies/common/generic.h>
#include <common/states.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <management/cpufreq/frequency.h>
#include <common/types/event_type.h>
#include <report/report.h>


#define POLICY_SAVINGS_LVL 2
#define CPU_SAVINGS_LVL 2

#define REPORT_SAVING(type, val) \
  if (report_earl_events){ \
    policy_savings_event.event = type; \
		policy_savings_event.value = val; \
		policy_savings_event.timestamp = time(NULL); \
    report_events(&rep_id, &policy_savings_event, 1); \
  }

extern volatile uint ear_periodic_mode;
extern uint report_earl_events;
extern report_id_t rep_id;

static ulong *policy_savings_comp_freqs = NULL;
static float energy_policy_saving = 0.0;
static float time_policy_penalty = 0.0; 
static float power_policy_saving = 0.0;
static timestamp policy_saving_time_start;
static uint policy_saving_updated = 0;
static ear_event_t policy_savings_event;

static double get_time_nominal(energy_model_t energy_model, signature_t *signature);
static double get_power_projected(energy_model_t energy_model, signature_t *signature, ulong f);
static double get_time_projected(energy_model_t energy_model, signature_t *signature, ulong f);
static double get_power_nominal(energy_model_t energy_model, signature_t *signature);

static uint must_compute_policy_savings(signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom, uint *savings);


double metric_ratio(double metric_ref, double metric_curr, uint type)
{
    if (type) {
        return (metric_curr - metric_ref) / metric_ref;
    }
    else {
        return (metric_ref - metric_curr) / metric_ref;
    }
}


uint above_max_penalty(double time_ref, double time_curr, double cpi_ref, double cpi_curr,
                       double gbs_ref, double gbs_curr, double penalty_th)
{
    double metric_max, metric_min;
    if (ear_periodic_mode == PERIODIC_MODE_OFF){
        /* We use TIME as our reference metric */
        metric_max = time_ref * (1 + penalty_th);
        return (time_curr > metric_max);
    }else{
        /* We use CPI and GBS our reference metrics */
        metric_max = cpi_ref * (1 + penalty_th);
        metric_min = gbs_ref * (1 - penalty_th);
        return ((cpi_curr > metric_max) || (gbs_curr < metric_min));
    }
}


uint below_perf_min_benefit(double time_ref, double time_curr, double cpi_ref, double cpi_curr,
        double gbs_ref, double gbs_curr, double freq_ref, double freq_curr, double penalty_th){
    double freq_gain = metric_ratio(freq_ref, freq_curr, RATIO_INCR);
    double perf_min_benefit = freq_gain * penalty_th;

    double metric_gain, metric_decr;
    if (ear_periodic_mode == PERIODIC_MODE_OFF){
         metric_decr =  metric_ratio(time_ref, time_curr, RATIO_DECR);
         return (metric_decr < perf_min_benefit);
    }else{
        metric_decr = metric_ratio(cpi_ref, cpi_curr, RATIO_DECR);
        metric_gain = metric_ratio(gbs_ref, gbs_curr, RATIO_INCR);
        return (metric_gain < perf_min_benefit || metric_decr < perf_min_benefit);
    }
}


void compute_energy_savings(signature_t *curr, signature_t *prev)
{
	// float performance_prev, performance_curr;
	float psaving = 0;
	float tpenalty = 0;
	float esaving = 0;
	ullong saving_elapsed;
#if 0
	double tflops;
#endif

	sig_ext_t *sig_ext = curr->sig_ext;

	if (energy_policy_saving == 0)
	{
		verbose_master(POLICY_SAVINGS_LVL, "%sWARNING%s Energy policy saving equal to 0", COL_YLW, COL_CLR); 
		return;
	}

#if 0
	if ((prev->Gflops == 0) || (curr->Gflops == 0)) {
		esaving = 0;
		return;
	}

	/* The Energy saving is the ration bettween GFlops/W */
	performance_prev = prev->Gflops/prev->DC_power;
	performance_curr = curr->Gflops/curr->DC_power;


#if 0
	if (state_fail(compute_job_node_gflops(sig_shared_region,
					lib_shared_region->num_processes, &tflops))) {
		verbose(3, "%sWARNING%s Error on computing job's node GFLOP/s rate: %s",
				COL_RED, COL_CLR, state_msg);
	}

	verbose_master(2, "MR[%d] Curr performance %.4f (%.2f/%.2f) Last performance %.4f (%.2f/%.2f)", masters_info.my_master_rank, \
			performance_curr, curr->Gflops, curr->DC_power, \
			performance_prev, prev->Gflops, prev->DC_power); 
#endif

	esaving  = ((performance_curr - performance_prev) / performance_prev) * 100;
	psaving  = ((prev->DC_power - curr->DC_power) / prev->DC_power ) * 100;
	tpenalty = ((prev->Gflops - curr->Gflops) / prev->Gflops) * 100;
#endif

	/* These policy savings are estimations based on the energy models */

	tpenalty = time_policy_penalty  * 100;
	esaving  = energy_policy_saving * 100;
	psaving  = power_policy_saving  * 100;

	verbose_master(POLICY_SAVINGS_LVL,"MR[%d]: Esaving %.2f, Psaving %.2f, Tpenalty %.2f, Elapsed %.2f",
								 masters_info.my_master_rank, esaving, psaving, tpenalty, sig_ext->elapsed);

	if (policy_saving_updated){
		saving_elapsed = timestamp_diffnow(&policy_saving_time_start, TIME_SECS);
		policy_saving_updated = 0;
	}else{
		saving_elapsed = (ullong) sig_ext->elapsed;
	}

	verbose_master(POLICY_SAVINGS_LVL, "Policy savings[%s]: Using elapsed time %llu", node_name, saving_elapsed);

	sig_ext->saving   = esaving * saving_elapsed;
	sig_ext->psaving  = psaving * saving_elapsed;
	sig_ext->tpenalty = tpenalty * saving_elapsed;

	REPORT_SAVING(ENERGY_SAVING, (llong) esaving);
	REPORT_SAVING(POWER_SAVING,  (llong) psaving);
	// REPORT_SAVING(PERF_PENALTY , (ulong)tpenalty);
}


void compute_policy_savings(energy_model_t energy_model, signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom)
{
  ulong *cpu_f = freqs->cpu_freq;
  double tref, pref;
  double tnext = 0, pnext = 0;
  double renergy, nenergy;
  ulong cpu_f_avg = 0;
  uint sav;
  ulong nominal = frequency_get_nominal_freq();
  
  if (policy_savings_comp_freqs == NULL){
    policy_savings_comp_freqs = (ulong *)calloc(MAX_CPUS_SUPPORTED, sizeof(ulong));
  }

  if (!must_compute_policy_savings(ns, freqs, dom, &sav)){
    if (!sav) {
      /* We are running at nominal */
      energy_policy_saving = 0;
      power_policy_saving  = 0;
      time_policy_penalty  = 0;
      timestamp_getfast(&policy_saving_time_start);
      policy_saving_updated = 1;
      verbose_master(POLICY_SAVINGS_LVL, "Policy savins[%s]: Reset of policy savings", node_name);
    }
    verbose_master(POLICY_SAVINGS_LVL, "Policy savins[%s]: Not applied", node_name);
    return;
  }

  memcpy(policy_savings_comp_freqs, cpu_f, sizeof(ulong)* MAX_CPUS_SUPPORTED);
  signature_print_simple_fd(verb_channel, ns);

  uint limit = (dom->cpu == POL_GRAIN_CORE) ? lib_shared_region->num_processes : 1;
  uint procs = 0;

  /* Time reference and power reference is always computed based on the nominal CPU freq 
   * IF the default CPU freq is not the nominal (min_time for example), we project data
   * for the nominal CPU frequency
   */
  tref = get_time_nominal(energy_model, ns);
  pref = get_power_nominal(energy_model, ns);
 
  /* IF we are selecting CPU freqs per process, we aggregate and compute the average */
  for (uint p = 0; p < limit; p++){
    if (cpu_f[p] <= nominal){
        tnext += get_time_projected(energy_model, ns, cpu_f[p]);
        pnext += get_power_projected(energy_model, ns, cpu_f[p]);
        cpu_f_avg += cpu_f[p];
        procs++;
      }
  }

  if (!procs) return;

  tnext = tnext / procs;
  pnext = pnext / procs;
  cpu_f_avg = cpu_f_avg / procs;

  /* renergy = energy at the nominal , nenergy = energy with policy selected CPu freq */
  renergy = tref  * pref;
  nenergy = tnext * pnext;

  if ((renergy == 0) || (nenergy == renergy)) {
    return;
  }

  /* These estimated savings are used in policy_ok, they are constant until we apply 
   * again the node_policy 
   */

  if ((pref == 0) || (tref == 0)) return;

  energy_policy_saving = (float)((renergy - nenergy)/renergy);
  if (pref != pnext) power_policy_saving = (float)((pref - pnext)/pref);
  if (tref != tnext) time_policy_penalty = (float)((tref - tnext)/tref);

  timestamp_getfast(&policy_saving_time_start);
  policy_saving_updated = 1;

  verbose_master(POLICY_SAVINGS_LVL, "Policy savins[%s]: energy/power/time %f/%f/%f CPU freq ref %lu, CPU freq next %lu, Ref data (time %.3lf power %.1lf), New data (time %.3lf power %.1lf)", 
      node_name, energy_policy_saving, power_policy_saving, time_policy_penalty, ns->def_f, cpu_f_avg, tref, pref, tnext, pnext);
}


static uint must_compute_policy_savings(signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom, uint *savings)
{
  *savings = 1;

  ulong nominal_freq = frequency_get_nominal_freq();
  uint at_nominal_freq = 1;
  ulong *cpu_f = freqs->cpu_freq;

  uint limit = (dom->cpu == POL_GRAIN_CORE) ? lib_shared_region->num_processes : 1;

  // if we run at nominal, there are no savings
  for (uint i = 0; (i < limit) && at_nominal_freq; i++)
	{
    at_nominal_freq = at_nominal_freq && (cpu_f[i] == nominal_freq);
  }

  verbose_master(POLICY_SAVINGS_LVL,"Policy savins[%s]: running at nominal ? at_ref %u f[0] != %lu", node_name, at_nominal_freq, nominal_freq);

  if (at_nominal_freq)
	{
    *savings = 0;
    return 0;
  }

  /* If the list of CPU freqs is different than last, we must compute savings */
  for (uint i = 0; i < limit; i++){
    if (cpu_f[i] != policy_savings_comp_freqs[i]){
      verbose_master(POLICY_SAVINGS_LVL,"Policy savins[%s]: f[%u] %lu != f[%u] %lu", node_name, i, cpu_f[i], i, policy_savings_comp_freqs[i]);
      return 1;
    }
  }
  
  return 0;
}


static double get_power_projected(energy_model_t energy_model, signature_t *signature, ulong f)
{
  double p = signature->DC_power;
  uint curr, target;
  if (f != signature->def_f) {
    curr   = frequency_closest_pstate(signature->def_f);
    target = frequency_closest_pstate(f);
    energy_model_project_power(energy_model, signature, curr, target, &p);
  }
  //verbose(0," Policy savins[%s] Projecting from %lu-%u to %lu-%u power %lf (ref %.2lf)", node_name, signature->def_f, curr, f, target, p, signature->DC_power);
  return p;
}


static double get_time_projected(energy_model_t energy_model, signature_t *signature, ulong f)
{
  double t = signature->time;
  uint curr, target;
  if (f != signature->def_f){
    curr   = frequency_closest_pstate(signature->def_f);
    target = frequency_closest_pstate(f);
    energy_model_project_time(energy_model, signature, curr, target, &t);
  }
  //verbose(0,"Policy savins[%s] Projecting from %lu-%u to %lu-%u time %lf (ref %.3lf)", node_name, signature->def_f, curr, f,target, t, signature->time);
  return (float)t;
}


static double get_power_nominal(energy_model_t energy_model, signature_t *signature)
{
  double p = signature->DC_power;
  uint curr, target;
  if (signature->def_f != frequency_get_nominal_freq()){
    target = frequency_get_nominal_pstate();
    curr   = frequency_closest_pstate(signature->def_f);
    energy_model_project_power(energy_model, signature, curr, target, &p);

		verbose_master(CPU_SAVINGS_LVL,"Policy savins[%s]: Projecting reference power from %u to %u %lf", node_name, curr, target, p);
  }
  return p;
}


static double get_time_nominal(energy_model_t energy_model, signature_t *signature)
{
  double t = signature->time;
  uint curr, target;
  if (signature->def_f != frequency_get_nominal_freq()){
    target = frequency_get_nominal_pstate();
    curr   = frequency_closest_pstate(signature->def_f);
    energy_model_project_time(energy_model, signature, curr, target, &t);
		verbose_master(CPU_SAVINGS_LVL,"Policy savins[%s]: Projecting reference time from %u to %u %lf", node_name, curr, target, t);
  }
  return t;
}
