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

#include <common/states.h>
#include <common/types/event_type.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/policies/common/generic.h>
#include <management/cpufreq/frequency.h>
#include <report/report.h>

#define POLICY_SAVINGS_LVL 2
#define CPU_SAVINGS_LVL    2

#define REPORT_SAVING(type, val)                                                                                       \
    if (report_earl_events) {                                                                                          \
        policy_savings_event.event     = type;                                                                         \
        policy_savings_event.value     = val;                                                                          \
        policy_savings_event.timestamp = time(NULL);                                                                   \
        report_events(&rep_id, &policy_savings_event, 1);                                                              \
    }

extern volatile uint ear_periodic_mode;
extern uint report_earl_events;
extern report_id_t rep_id;

// CPU
static ulong *policy_savings_comp_freqs = NULL;
static float energy_policy_saving       = 0.0;
static float time_policy_penalty        = 0.0;
static float power_policy_saving        = 0.0;
static timestamp policy_saving_time_start;
static uint policy_saving_updated = 0;
static ear_event_t policy_savings_event;

// GPU
static ulong *policy_savings_comp_gpu_freqs = NULL;
static float gpu_energy_policy_saving       = 0.0;
static float gpu_power_policy_saving        = 0.0;
static float gpu_time_policy_penalty        = 0.0;
static timestamp gpu_policy_saving_time_start;
static uint gpu_policy_saving_updated = 0;
static const ulong **gpuf_list;
static uint num_gpu_pstates;

// CPU
static double get_time_nominal(energy_model_t energy_model, signature_t *signature);
static double get_power_projected(energy_model_t energy_model, signature_t *signature, ulong f);
static double get_time_projected(energy_model_t energy_model, signature_t *signature, ulong f);
static double get_power_nominal(energy_model_t energy_model, signature_t *signature);

static uint must_compute_policy_savings(signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom, uint *savings);

// GPU
static double get_gpu_time_projected(energy_model_t energy_model, signature_t *signature, ulong *gpu_f,
                                     int get_nominal);
static double get_gpu_power_projected(energy_model_t energy_model, signature_t *signature, ulong *gpu_f,
                                      int get_nominal);
static uint must_compute_gpu_policy_savings(signature_t *ns, node_freqs_t *freqs, uint *savings);
static ulong gpu_frequency_closest_pstate(float gpu_freq);

double metric_ratio(double metric_ref, double metric_curr, uint type)
{
    if (type) {
        return (metric_curr - metric_ref) / metric_ref;
    } else {
        return (metric_ref - metric_curr) / metric_ref;
    }
}

uint above_max_penalty(double time_ref, double time_curr, double cpi_ref, double cpi_curr, double gbs_ref,
                       double gbs_curr, double penalty_th)
{
    double metric_max, metric_min;
    if (ear_periodic_mode == PERIODIC_MODE_OFF) {
        /* We use TIME as our reference metric */
        metric_max = time_ref * (1 + penalty_th);
        return (time_curr > metric_max);
    } else {
        /* We use CPI and GBS our reference metrics */
        metric_max = cpi_ref * (1 + penalty_th);
        metric_min = gbs_ref * (1 - penalty_th);
        return ((cpi_curr > metric_max) || (gbs_curr < metric_min));
    }
}

uint below_perf_min_benefit(double time_ref, double time_curr, double cpi_ref, double cpi_curr, double gbs_ref,
                            double gbs_curr, double freq_ref, double freq_curr, double penalty_th)
{
    double freq_gain        = metric_ratio(freq_ref, freq_curr, RATIO_INCR);
    double perf_min_benefit = freq_gain * penalty_th;

    double metric_gain, metric_decr;
    if (ear_periodic_mode == PERIODIC_MODE_OFF) {
        metric_decr = metric_ratio(time_ref, time_curr, RATIO_DECR);
        return (metric_decr < perf_min_benefit);
    } else {
        metric_decr = metric_ratio(cpi_ref, cpi_curr, RATIO_DECR);
        metric_gain = metric_ratio(gbs_ref, gbs_curr, RATIO_INCR);
        return (metric_gain < perf_min_benefit || metric_decr < perf_min_benefit);
    }
}

void compute_energy_savings(signature_t *curr, signature_t *prev)
{
    // float performance_prev, performance_curr;
    float psaving  = 0;
    float tpenalty = 0;
    float esaving  = 0;
    ullong saving_elapsed;
#if 0
	double tflops;
#endif

    sig_ext_t *sig_ext = curr->sig_ext;

    if (energy_policy_saving == 0) {
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

    tpenalty = time_policy_penalty * 100;
    esaving  = energy_policy_saving * 100;
    psaving  = power_policy_saving * 100;

    verbose_master(POLICY_SAVINGS_LVL, "MR[%d]: Esaving %.2f, Psaving %.2f, Tpenalty %.2f, Elapsed %.2f",
                   masters_info.my_master_rank, esaving, psaving, tpenalty, sig_ext->elapsed);

    if (policy_saving_updated) {
        saving_elapsed        = timestamp_diffnow(&policy_saving_time_start, TIME_SECS);
        policy_saving_updated = 0;
    } else {
        saving_elapsed = (ullong) sig_ext->elapsed;
    }

    verbose_master(POLICY_SAVINGS_LVL, "Policy savings[%s]: Using elapsed time %llu", node_name, saving_elapsed);

    sig_ext->saving   = esaving * saving_elapsed;
    sig_ext->psaving  = psaving * saving_elapsed;
    sig_ext->tpenalty = tpenalty * saving_elapsed;

    REPORT_SAVING(ENERGY_SAVING, (llong) esaving);
    REPORT_SAVING(POWER_SAVING, (llong) psaving);
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

    if (policy_savings_comp_freqs == NULL) {
        policy_savings_comp_freqs = (ulong *) calloc(MAX_CPUS_SUPPORTED, sizeof(ulong));
    }

    if (!must_compute_policy_savings(ns, freqs, dom, &sav)) {
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

    memcpy(policy_savings_comp_freqs, cpu_f, sizeof(ulong) * MAX_CPUS_SUPPORTED);
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
    for (uint p = 0; p < limit; p++) {
        if (cpu_f[p] <= nominal) {
            tnext += get_time_projected(energy_model, ns, cpu_f[p]);
            pnext += get_power_projected(energy_model, ns, cpu_f[p]);
            cpu_f_avg += cpu_f[p];
            procs++;
        }
    }

    if (!procs)
        return;

    tnext     = tnext / procs;
    pnext     = pnext / procs;
    cpu_f_avg = cpu_f_avg / procs;

    /* renergy = energy at the nominal , nenergy = energy with policy selected CPu freq */
    renergy = tref * pref;
    nenergy = tnext * pnext;

    if ((renergy == 0) || (nenergy == renergy)) {
        return;
    }

    /* These estimated savings are used in policy_ok, they are constant until we apply
     * again the node_policy
     */

    if ((pref == 0) || (tref == 0))
        return;

    energy_policy_saving = (float) ((renergy - nenergy) / renergy);
    if (pref != pnext)
        power_policy_saving = (float) ((pref - pnext) / pref);
    if (tref != tnext)
        time_policy_penalty = (float) ((tref - tnext) / tref);

    timestamp_getfast(&policy_saving_time_start);
    policy_saving_updated = 1;

    verbose_master(POLICY_SAVINGS_LVL,
                   "Policy savins[%s]: energy/power/time %f/%f/%f CPU freq ref %lu, CPU freq next %lu, Ref data (time "
                   "%.3lf power %.1lf), New data (time %.3lf power %.1lf)",
                   node_name, energy_policy_saving, power_policy_saving, time_policy_penalty, ns->def_f, cpu_f_avg,
                   tref, pref, tnext, pnext);
}

static uint must_compute_policy_savings(signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom, uint *savings)
{
    *savings = 1;

    ulong nominal_freq   = frequency_get_nominal_freq();
    uint at_nominal_freq = 1;
    ulong *cpu_f         = freqs->cpu_freq;

    uint limit = (dom->cpu == POL_GRAIN_CORE) ? lib_shared_region->num_processes : 1;

    // if we run at nominal, there are no savings
    for (uint i = 0; (i < limit) && at_nominal_freq; i++) {
        at_nominal_freq = at_nominal_freq && (cpu_f[i] == nominal_freq);
    }

    verbose_master(POLICY_SAVINGS_LVL, "Policy savins[%s]: running at nominal ? at_ref %u f[0] != %lu", node_name,
                   at_nominal_freq, nominal_freq);

    if (at_nominal_freq) {
        *savings = 0;
        return 0;
    }

    /* If the list of CPU freqs is different than last, we must compute savings */
    for (uint i = 0; i < limit; i++) {
        if (cpu_f[i] != policy_savings_comp_freqs[i]) {
            verbose_master(POLICY_SAVINGS_LVL, "Policy savins[%s]: f[%u] %lu != f[%u] %lu", node_name, i, cpu_f[i], i,
                           policy_savings_comp_freqs[i]);
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
    // verbose(0," Policy savins[%s] Projecting from %lu-%u to %lu-%u power %lf (ref %.2lf)", node_name,
    // signature->def_f, curr, f, target, p, signature->DC_power);
    return p;
}

static double get_time_projected(energy_model_t energy_model, signature_t *signature, ulong f)
{
    double t = signature->time;
    uint curr, target;
    if (f != signature->def_f) {
        curr   = frequency_closest_pstate(signature->def_f);
        target = frequency_closest_pstate(f);
        energy_model_project_time(energy_model, signature, curr, target, &t);
    }
    // verbose(0,"Policy savins[%s] Projecting from %lu-%u to %lu-%u time %lf (ref %.3lf)", node_name, signature->def_f,
    // curr, f,target, t, signature->time);
    return (float) t;
}

static double get_power_nominal(energy_model_t energy_model, signature_t *signature)
{
    double p = signature->DC_power;
    uint curr, target;
    if (signature->def_f != frequency_get_nominal_freq()) {
        target = frequency_get_nominal_pstate();
        curr   = frequency_closest_pstate(signature->def_f);
        energy_model_project_power(energy_model, signature, curr, target, &p);

        verbose_master(CPU_SAVINGS_LVL, "Policy savins[%s]: Projecting reference power from %u to %u %lf", node_name,
                       curr, target, p);
    }
    return p;
}

static double get_time_nominal(energy_model_t energy_model, signature_t *signature)
{
    double t = signature->time;
    uint curr, target;
    if (signature->def_f != frequency_get_nominal_freq()) {
        target = frequency_get_nominal_pstate();
        curr   = frequency_closest_pstate(signature->def_f);
        energy_model_project_time(energy_model, signature, curr, target, &t);
        verbose_master(CPU_SAVINGS_LVL, "Policy savins[%s]: Projecting reference time from %u to %u %lf", node_name,
                       curr, target, t);
    }
    return t;
}

// GPU Savings functions

void compute_gpu_energy_savings(signature_t *curr)
{
    float gpu_tpenalty = 0;
    float gpu_psaving  = 0;
    float gpu_esaving  = 0;
    float node_psaving = 0;
    float node_esaving = 0;
    ullong saving_elapsed;

    sig_ext_t *sig_ext = curr->sig_ext;
    if (gpu_energy_policy_saving == 0) {
        verbose_master(POLICY_SAVINGS_LVL, "%sWARNING%s GPU Energy policy saving equal to 0", COL_YLW, COL_CLR);
        return;
    }

    /* These policy savings are estimations based on the energy models */
    gpu_tpenalty = gpu_time_policy_penalty * 100;
    gpu_esaving  = gpu_energy_policy_saving * 100;
    gpu_psaving  = gpu_power_policy_saving * 100;
    node_esaving = energy_policy_saving * 100;
    node_psaving = power_policy_saving * 100;

    verbose_master(POLICY_SAVINGS_LVL,
                   "MR[%d]: GPU Esav %.2f, Pred %.2f, Node Esav %.2f, Pred %.2f, Tpen %.2f, Elapsed %.2f",
                   masters_info.my_master_rank, gpu_esaving, gpu_psaving, node_esaving, node_psaving, gpu_tpenalty,
                   sig_ext->elapsed);

    if (gpu_policy_saving_updated) {
        saving_elapsed            = timestamp_diffnow(&gpu_policy_saving_time_start, TIME_SECS);
        gpu_policy_saving_updated = 0;
    } else {
        saving_elapsed = (ullong) sig_ext->elapsed;
    }

    // Init energy savings fields
    policy_savings_event.jid     = application.job.id;
    policy_savings_event.step_id = application.job.step_id;
    strcpy(policy_savings_event.node_id, application.node_id);

    // verbose_master(POLICY_SAVINGS_LVL,"EVENT: jobid %lu stepid %lu nodeid %s elpased %lld (upd
    // %d)",policy_savings_event.jid, policy_savings_event.step_id, policy_savings_event.node_id, saving_elapsed,
    // gpu_policy_saving_updated);
    //  GPU Savings
    sig_ext->gpu_saving   = gpu_esaving * saving_elapsed;
    sig_ext->gpu_psaving  = gpu_psaving * saving_elapsed;
    sig_ext->gpu_tpenalty = gpu_tpenalty * saving_elapsed;
    REPORT_SAVING(GPU_ENERGY_SAVING, (llong) gpu_esaving);
    REPORT_SAVING(GPU_POWER_SAVING, (llong) gpu_psaving);
    REPORT_SAVING(GPU_PERF_PENALTY, (ulong) gpu_tpenalty);

    // Node Savings
    sig_ext->saving   = node_esaving * saving_elapsed;
    sig_ext->psaving  = node_psaving * saving_elapsed;
    sig_ext->tpenalty = gpu_tpenalty * saving_elapsed;
    REPORT_SAVING(ENERGY_SAVING, (llong) node_esaving);
    REPORT_SAVING(POWER_SAVING, (llong) node_psaving);
    REPORT_SAVING(PERF_PENALTY, (ulong) gpu_tpenalty);

    // verbose_master(POLICY_SAVINGS_LVL,"MR[%d]: sig_ext GPU Esaving %.2f Psaving %.2f, Node Esaving %.2f Psaving, %.2f
    // Tpenalty %.2f",
    //		masters_info.my_master_rank, sig_ext->gpu_saving , sig_ext->gpu_psaving, sig_ext->saving, sig_ext->psaving,
    // sig_ext->gpu_tpenalty);
}

void compute_gpu_policy_savings(energy_model_t energy_model, signature_t *ns, node_freqs_t *freqs)
{
    ulong *gpu_f = freqs->gpu_freq;
    double tref, pref;
    double eref, enext;
    double node_pref, node_pnext;
    double node_eref, node_enext;
    double tnext = 0, pnext = 0, pcurr = 0;
    uint sav;
    const uint *gpuf_list_items;

    /* At this point, we assume GPU mgt has been already initialized */
    gpuf_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
    gpuf_list_items = (const uint *) metrics_gpus_get(MGT_GPU)->avail_count;
    num_gpu_pstates = gpuf_list_items[0];

    if (policy_savings_comp_gpu_freqs == NULL) {
        policy_savings_comp_gpu_freqs = (ulong *) calloc(MAX_GPUS_SUPPORTED, sizeof(ulong));
    }

    if (!must_compute_gpu_policy_savings(ns, freqs, &sav)) {
        if (!sav) {
            /* We are running at nominal */
            // the following variables are set in this function and used in compute_energy_savings
            gpu_energy_policy_saving = 0;
            gpu_power_policy_saving  = 0;
            gpu_time_policy_penalty  = 0;
            energy_policy_saving     = 0;
            power_policy_saving      = 0;
            timestamp_getfast(&policy_saving_time_start);
            gpu_policy_saving_updated = 1;
            verbose_master(POLICY_SAVINGS_LVL, "GPU Policy savings[%s]: Reset of gpu policy savings", node_name);
        }
        verbose_master(POLICY_SAVINGS_LVL, "GPU Policy savings[%s]: Not applied", node_name);
        return;
    }

    memcpy(policy_savings_comp_gpu_freqs, gpu_f, sizeof(ulong) * MAX_GPUS_SUPPORTED);
    signature_print_simple_fd(verb_channel, ns);

    // Time and power reference are always computed based on the nominal GPU freq
    pref = get_gpu_power_projected(energy_model, ns, gpu_f, 1);
    tref = get_gpu_time_projected(energy_model, ns, gpu_f, 1);

    // Time and power with GPU optimization
    pnext = get_gpu_power_projected(energy_model, ns, gpu_f, 0);
    tnext = get_gpu_time_projected(energy_model, ns, gpu_f, 0);

    // eref = energy at the nominal , enext = energy with policy selected CPu freq
    eref  = tref * pref;
    enext = tnext * pnext;

    if ((eref == 0) || (enext == eref)) {
        return;
    }

    // These estimated savings are used in policy_ok, they are constant until we apply again the node_policy
    if ((pref == 0) || (tref == 0))
        return;

    gpu_energy_policy_saving = (float) ((eref - enext) / eref);
    if (pref != pnext)
        gpu_power_policy_saving = (float) ((pref - pnext) / pref);
    if (tref != tnext)
        gpu_time_policy_penalty = (float) ((tnext - tref) / tref);
    // We estimate the node savings assuming that GPU optimisation affect only the GPU power
    // Node Power = GPU Power + Other
    // Current GPU Power
    for (int i = 0; i < ns->gpu_sig.num_gpus; i++) {
        if (ns->gpu_sig.gpu_data[i].GPU_util) {
            pcurr += ns->gpu_sig.gpu_data[i].GPU_power;
        }
    }
    node_pref            = ns->DC_power + pref - pcurr;
    node_pnext           = ns->DC_power + pnext - pcurr;
    node_eref            = tref * node_pref;
    node_enext           = tnext * node_pnext;
    power_policy_saving  = (float) ((pref - pnext) / node_pref);
    energy_policy_saving = (float) ((node_eref - node_enext) / node_eref);

    timestamp_getfast(&policy_saving_time_start);
    policy_saving_updated = 1;

    verbose_master(POLICY_SAVINGS_LVL,
                   "GPU Policy savings[%s]: energy/power/time %f/%f/%f, Ref data (time %.3lf power %.1lf), New data "
                   "(time %.3lf power %.1lf)",
                   node_name, gpu_energy_policy_saving, gpu_power_policy_saving, gpu_time_policy_penalty, tref, pref,
                   tnext, pnext);
    verbose_master(POLICY_SAVINGS_LVL,
                   "GPU Policy (Node) savings[%s]: energy/power/time %f/%f/%f, Ref data (time %.3lf power %.1lf), New "
                   "data (time %.3lf power %.1lf)",
                   node_name, energy_policy_saving, power_policy_saving, gpu_time_policy_penalty, tref, node_pref,
                   tnext, node_pnext);
}

static double get_gpu_time_projected(energy_model_t energy_model, signature_t *signature, ulong *gpu_f, int get_nominal)
{
    double gpu_time_proj = signature->time;
    float curr_gpu_avgf  = 0;
    float next_gpu_avgf  = 0;
    ulong curr, target;
    int gused = 0;

    for (int i = 0; i < signature->gpu_sig.num_gpus; i++) {
        if (signature->gpu_sig.gpu_data[i].GPU_util) {
            curr_gpu_avgf += signature->gpu_sig.gpu_data[i].GPU_freq;
            if (!get_nominal)
                next_gpu_avgf += gpu_f[i];
            gused++;
        }
    }

    curr_gpu_avgf = curr_gpu_avgf / gused;
    curr          = gpu_frequency_closest_pstate(curr_gpu_avgf);
    if (get_nominal) {
        next_gpu_avgf = gpuf_list[0][0];
        target        = 0;
    } else {
        next_gpu_avgf = next_gpu_avgf / gused;
        target        = gpu_frequency_closest_pstate(next_gpu_avgf);
    }

    if (target != curr) {
        energy_model_project_time(energy_model, signature, curr, target, &gpu_time_proj);
    }

    debug("GPU Policy savings[%s] Projecting from %.2f-%ld to %.2f-%ld time %.3f (origin %.3f)", node_name,
          curr_gpu_avgf, curr, next_gpu_avgf, target, gpu_time_proj, signature->time);
    return gpu_time_proj;
}

static double get_gpu_power_projected(energy_model_t energy_model, signature_t *signature, ulong *gpu_f,
                                      int get_nominal)
{
    double acc_gpu_power_from = 0;
    double acc_gpu_power_proj = 0;
    double *gpu_power_proj;
    float curr_gpu_avgf = 0;
    float next_gpu_avgf = 0;
    int gused           = 0;
    ulong curr, target;
    int i;

    for (i = 0; i < signature->gpu_sig.num_gpus; i++) {
        if (signature->gpu_sig.gpu_data[i].GPU_util) {
            acc_gpu_power_from += signature->gpu_sig.gpu_data[i].GPU_power;
            curr_gpu_avgf += signature->gpu_sig.gpu_data[i].GPU_freq;
            if (!get_nominal)
                next_gpu_avgf += gpu_f[i];
            gused++;
        }
    }

    curr_gpu_avgf = curr_gpu_avgf / gused;
    curr          = gpu_frequency_closest_pstate(curr_gpu_avgf);
    if (get_nominal) {
        next_gpu_avgf = gpuf_list[0][0];
        target        = 0;
    } else {
        next_gpu_avgf = next_gpu_avgf / gused;
        target        = gpu_frequency_closest_pstate(next_gpu_avgf);
    }

    if (target != curr) {
        gpu_power_proj = (double *) calloc(signature->gpu_sig.num_gpus, sizeof(double));
        energy_model_project_power(energy_model, signature, curr, target, gpu_power_proj);

        for (i = 0; i < signature->gpu_sig.num_gpus; i++) {
            if (signature->gpu_sig.gpu_data[i].GPU_util)
                acc_gpu_power_proj += gpu_power_proj[i];
        }
    } else {
        acc_gpu_power_proj = acc_gpu_power_from;
    }

    debug("GPU Policy savings[%s] Projecting from %.2f-%ld to %.2f-%ld - power %.3f (origin %.3f)", node_name,
          curr_gpu_avgf, curr, next_gpu_avgf, target, acc_gpu_power_proj, acc_gpu_power_from);
    return acc_gpu_power_proj;
}

static uint must_compute_gpu_policy_savings(signature_t *ns, node_freqs_t *freqs, uint *savings)
{
    *savings = 1;

    ulong gpu_nominal_freq = gpuf_list[0][0];
    int i;
    uint at_gpu_nominal_freq = 1;
    ulong *gpu_f             = freqs->gpu_freq;

    // if we run at nominal, there are no savings
    for (i = 0; (i < ns->gpu_sig.num_gpus) && at_gpu_nominal_freq; i++) {
        at_gpu_nominal_freq = at_gpu_nominal_freq && (gpu_f[i] == gpu_nominal_freq);
    }

    verbose_master(POLICY_SAVINGS_LVL, "GPU Policy savins[%s]: running at nominal ? at_ref %u f[0] != %lu", node_name,
                   at_gpu_nominal_freq, gpu_nominal_freq);

    if (at_gpu_nominal_freq) {
        *savings = 0;
        return 0;
    }

    /* If the list of GPU freqs is different than last, we must compute savings */
    for (i = 0; i < ns->gpu_sig.num_gpus; i++) {
        if (gpu_f[i] != policy_savings_comp_gpu_freqs[i]) {
            verbose_master(POLICY_SAVINGS_LVL, "GPU Policy savings[%s]: f[%u] %lu != f[%u] %lu", node_name, i, gpu_f[i],
                           i, policy_savings_comp_gpu_freqs[i]);
            return 1;
        }
    }

    return 0;
}

// get the closed gpu pstate to a given gpu freq
ulong gpu_frequency_closest_pstate(float gpu_freq)
{
    float current_diff, smallest_diff;

    smallest_diff    = fabs(gpu_freq - (float) gpuf_list[0][0]);
    ulong closest_ps = 0;

    for (ulong i = 1; i < num_gpu_pstates; i++) {
        current_diff = fabs(gpu_freq - (float) gpuf_list[0][i]);
        if (current_diff < smallest_diff) {
            smallest_diff = current_diff;
            closest_ps    = i;
        } else {
            break;
        }
    }
    return closest_ps;
}
