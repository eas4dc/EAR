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
#include <math.h>
#if MPI
#include <mpi.h>
#endif
#include <dlfcn.h>
#include <semaphore.h>
// #define SHOW_DEBUGS 1
#include <common/includes.h>
#include <common/system/symplug.h>
#include <common/environment.h>
#include <common/types/generic.h>
#include <common/types/pc_app_info.h>
#include <common/hardware/hardware_info.h>
#include <management/cpufreq/frequency.h>
#include <management/imcfreq/imcfreq.h>
#include <management/cpufreq/cpufreq.h>
#include <report/report.h>
#include <library/metrics/metrics.h>
#include <library/api/mpi_support.h>
#include <library/api/clasify.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/policy.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/global_comm.h>
#include <library/loader/module_cuda.h>
#include <library/tracer/tracer.h>
#include <library/tracer/tracer_paraver_comp.h>
#include <library/policies/common/pc_support.h>
#include <library/policies/policy_state.h>
#include <daemon/local_api/eard_api.h>
#include <daemon/local_api/node_mgr.h>
#include <daemon/powercap/powercap_status_conf.h>

/* Verbose levels */
#define POLICY_INFO   2
#define POLICY_PHASES 2
#define POLICY_GPU    2
#define POLICY_SAVINGS_LVL 2

#define DEF_USE_CPUPRIO 0

/* Must be migrated to config_env */
#define GPU_ENABLE_OPT "EAR_GPU_ENABLE_OPT"

#define is_master masters_info.my_master_rank >= 0


static ear_event_t curr_policy_event;
extern uint report_earl_events;
extern report_id_t rep_id;

extern sem_t *lib_shared_lock_sem;

static float time_policy_penalty = 0.0; 
static float power_policy_saving = 0.0;
static float energy_policy_saving = 0.0;
static timestamp policy_saving_time_start;
static uint policy_saving_updated = 0;

#define REPORT_PHASE(phase) \
  if (report_earl_events){ \
    fill_event(&curr_policy_event, EARL_PHASE, phase); \
    report_events(&rep_id, &curr_policy_event, 1); \
  }


#define REPORT_OPT_ACCURACY(phase) \
  if (report_earl_events){ \
    fill_event(&curr_policy_event, EARL_OPT_ACCURACY, phase); \
    report_events(&rep_id, &curr_policy_event, 1); \
  }

#define REPORT_SAVING(type, value) \
  if (report_earl_events){ \
    fill_event(&curr_policy_event, type, value); \
    report_events(&rep_id, &curr_policy_event, 1); \
  }

// TODO: Can below macro be defined in a more coherent file?
#define GHZ_TO_KHZ 1000000.0

#define POLICY_MPI_OPT 1 // Enables doing stuff at MPI call level.
                         // Useful for fast overhead testing.

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif

extern masters_info_t masters_info;

extern cpu_set_t ear_process_mask;
extern int       ear_affinity_is_set;

extern       uint *imc_max_pstate; // Hardware, not config
extern       uint *imc_min_pstate; // Hardware, not config
extern       uint  imc_devices;
extern const pstate_t *imc_pstates;

extern pc_app_info_t *pc_app_info_data;

/* Node mgr data */
extern ear_njob_t *node_mgr_data;
extern uint node_mgr_index;
extern node_mgr_sh_data_t *node_mgr_job_info;

extern node_freq_domain_t earl_default_domains;

/** Options */

/* Uncore Frequency Scaling */
uint conf_dyn_unc   = EAR_eUFS;
uint dyn_unc        = EAR_eUFS;
double imc_extra_th = EAR_IMC_TH;

/* Load Balance */
uint enable_load_balance = EAR_USE_LB;
uint use_turbo_for_cp    = EAR_USE_TURBO_CP;

/* Try Turbo */
uint try_turbo_enabled = 0;

/* Energy models */
uint use_energy_models = 1;

/* Exclusive mode */
static uint exclusive_mode = 0;

/* CPU and IMC lower bounds */
static ullong min_cpufreq  = 0; // CPu freq. selection works with khz
static ullong min_imcfreq  = 0;
uint   max_policy_imcfreq_ps = ps_nothing; // IMC freq. selection works with pstates

#if MPI_OPTIMIZED
/* MPI optimization */
extern uint ear_mpi_opt;
#endif
uint gpu_optimize = 0;

static pstate_t *cpu_pstate_lst; /*!< The list of available CPU pstates in the current system.*/
static uint      cpu_pstate_cnt;

static pstate_t *imc_pstate_lst; /*!< The list of available IMC pstates in the current system.*/
static uint      imc_pstate_cnt;

static uint use_earl_phases = EAR_USE_PHASES;
static ulong init_def_cpufreq_khz;
static uint is_monitoring = 0;
//  NEW CLASSIFY
extern ear_classify_t phases_limits;
uint policy_cpu_bound = 0;
uint policy_mem_bound = 0;

int my_globalrank;

static signature_t policy_last_local_signature;
static signature_t policy_last_global_signature;

polctx_t my_pol_ctx;

unsigned long long int total_mpi = 0; // This variable only counts the number of MPI calls and it's only used
                                      // at state_comm::verbose_signature. It only is useful when only the master verboses,
                                      // so it's value only shows master info while that signature verbose message tries to
                                      // show per-node info.

node_freqs_t nf;     /*!< Nominal frequency */
node_freqs_t avg_nf;
node_freqs_t last_nf;

#if POLICY_MPI_OPT
static node_freqs_t per_process_node_freq;
#endif

static ulong max_freq_per_socket[MAX_SOCKETS_SUPPORTED];
static uint  cpu_must_be_normalized[MAX_CPUS_SUPPORTED];

uint last_earl_phase_classification = APP_COMP_BOUND;
static uint ignore_affinity_mask = 0;

int cpu_ready = EAR_POLICY_NO_STATE;
int gpu_ready = EAR_POLICY_NO_STATE;

gpu_state_t curr_gpu_state;
gpu_state_t last_gpu_state = _GPU_NoGPU;             // Initial GPU state as Computation

static uint fake_io_phase = 0;
static uint fake_bw_phase = 0;

const int   polsyms_n = 18;
const char *polsyms_nam[] = {
    "policy_init",
    "policy_apply",
    "policy_app_apply",
    "policy_get_default_freq",
    "policy_ok",
    "policy_max_tries",
    "policy_end",
    "policy_loop_init",
    "policy_loop_end",
    "policy_new_iteration",
    "policy_mpi_init",
    "policy_mpi_end",
    "policy_configure",
    "policy_domain",
    "policy_io_settings",
    "policy_cpu_gpu_settings",
    "policy_busy_wait_settings",
    "policy_restore_settings",
};

static polsym_t polsyms_fun;

static ulong *freq_per_core;

static node_freq_domain_t freqs_domain;

static const ulong **gpuf_pol_list;
static const uint   *gpuf_pol_list_items;

static uint first_policy_try = 1;

static uint  use_cpuprio = DEF_USE_CPUPRIO; // Enables the usage of proiority system

static uint total_mpi_optimized =  0;

/* Governor management for exclusive mode */
static uint *governor_list_before_init = NULL;

void fill_common_event(ear_event_t *ev);
void fill_event(ear_event_t *ev, uint event, llong value);


/** Reads information about some management API and stores the pstate list
 * and devices count information to the output arguments. Pure function.
 * \param api_id[in]      The API id of the mgt API you want to get the information, e.g., MGT_CPUFREQ.
 * \param pstate_lst[out] The list of pstates read from \p api_id API.
 * \param pstate_cnt[out] The number of pstate available in \p pstate_lst.
 */
static state_t fill_mgt_api_pstate_data(uint api_id, pstate_t **pstate_lst, uint *pstate_cnt);


/** Maps the list of CPU frequencies mapped per process, to a list of CPU frequencies mapped per core.
 * \param[in]  process_cpu_freqs_khz A list of frequencies indexed per local (node-level) rank.
 * \param[in]  process_cnt           The number of elements in \p process_cpu_freqs_khz.
 * \param[out] core_cpu_freqs_khz    A list of frequencies indexed per core.
 * \param[in]  cpu_cnt               The number of cores which is expected to be in core_cpu_freqs_khz.
 * \param[out] cpu_freq_max          If a non-NULL address is passed, the argument is filled with the maximum
 * CPU freq. found in \process_cpu_freqs_khz.
 * \param[out] cpu_freq_min          If a non-NULL address is passed, the argument is filled with the minimum
 * CPU freq. found in \process_cpu_freqs_khz.
 * \return EAR_ERROR if some argument is invalid.
 * \return EAR_SUCCESS otherwise.
 */
static state_t from_proc_to_core(ulong *process_cpu_freqs_khz, int process_cnt, ulong **core_cpu_freqs_khz,
                                 int cpu_cnt, ulong *cpu_freq_max, ulong *cpu_freq_min, int process_id);


/** This function returns whether the CPU frequency selected by the policy itself must be selected.
 * The result depends on current Powercap status.
 */
static uint must_set_policy_cpufreq(pc_app_info_t *pcdata, settings_conf_t *settings_conf);


/** Calls the different methods for applying the frequency selection for the domains compatible with the
 * current loaded policy. It only works if master process (node-level) called the function. */
static state_t policy_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id);


static state_t policy_master_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id);


/** Returns the mean of the frequency list \p freq_list with size \p n. */
static ulong compute_avg_freq(ulong *freq_list, int n);


/** Sets the CPU frequency according to what was selected by the policy plugin.
 * \param[in] c The current policy context.
 * \param[in] my_sig The signature used by the policy plugin. Not used.
 * \param[in] freqs The set of frequencies for the different system domains (e.g., CPU, IMC, GPU).
 * \param[in] dom The domain of the current policy plugin.
 * \param[out] avg_freq After the function, this adress will contain the average CPU freq. selected. */
static state_t policy_cpu_freq_selection(polctx_t *c, signature_t *my_sig,
                                         node_freqs_t *freqs, node_freq_domain_t *dom,
                                         node_freqs_t *avg_freq, int process_id);


/** Verboses the policy context \p policy_ctx if the verbosity level
 * of the job is equal or higher than \p verb_lvl. */
static void verbose_policy_ctx(int verb_lvl, polctx_t *policy_ctx);


/** Verboses the list of frequencies \p core_cpu_freqs_khz, assuming that is given as the per CPU frequency
 * list, for a total number of \p cpu_cnt CPU/processors. Use \p verb_lvl to specify the verbosity level
 * required by the job to print the information. */
static void verbose_freq_per_core(int verb_lvl, ulong **core_cpu_freqs_khz, int cpu_cnt);


/** Verboses the elapsed time on each phase encountered
 * in the extended signature of the passed signature \p sig. */
static void verbose_time_in_phases(signature_t *sig);


/** Encapsulates priority setup. */
static void policy_setup_priority();


/** Encapsulates all user-priovided PRIO affinity configuration. */
static state_t policy_setup_priority_per_cpu(uint *setup_prio_list, uint setup_prio_list_count);


/** Encapsulates all user-provided PRIO task configuration. */
static state_t policy_setup_priority_per_task(uint *setup_prio_list, uint setup_prio_list_count);


/** This function sets the priority list \p src_prio_list to \p dst_prio_list. The \p mask is used to map
 * which index of \p dst_prio_list corresponds to the i-th value of \p src_prio_list, i.e., the index of the
 * i-th cpu set in \p mask sets the index of \p dst_prio_list that must be filled with the i-th value of
 * \p src_prio_list. Positions of \p dst_prio_list not corresponding to any cpu set in \p mask, are filled
 * with the PRIO_SAME value. */
static state_t set_priority_list(uint *dst_prio_list, uint dst_prio_list_count, const uint *src_prio_list,
                                 uint src_prio_list_count, const cpu_set_t *mask);


/** Sets \p priority on those positions of \p dst_prio_list enabled by \p mask. */
static state_t policy_set_prio_with_mask(uint *dst_prio_list, uint dst_prio_list_count, uint priority, const cpu_set_t *mask);


/** Sets \p priority on those positions of \p not enabled by \p mask. */
static state_t policy_set_prio_with_mask_inverted(uint *dst_prio_list, uint dst_prio_list_count, uint priority, const cpu_set_t *mask);


/** This function resets priority configuration for CPUs used by the job
 * in the case priority system is enabled by the user. */
static state_t policy_reset_priority();


/** This function sets the priority 0 to \p prio_list to those positions indicated by \p mask. */
static state_t reset_priority_list_with_mask(uint *prio_list, uint prio_list_count, const cpu_set_t *mask);


/** Verboses a list of priorities. */
static void verbose_cpuprio_list(int verb_lvl, uint *cpuprio_list, uint cpuprio_list_count);


/** Sets governor `userspace` to  CPUs that are
 * in my node mask or all if exclusive_mode is set */
static state_t policy_set_cpus_governor(uint exclusive);


/** Sets the governor list stored at governor_list_before_init. */
static state_t policy_restore_cpus_governor();


/**** Auxiliary functions ***/
#if 0
state_t policy_load(char *obj_path, polsym_t *psyms)
{
    state_t st;
    if ((st = symplug_open(obj_path, (void **)psyms, polsyms_nam, polsyms_n)) != EAR_SUCCESS){
        verbose_master(POLICY_INFO, "%sError%s loading policy file: %s", COL_RED, COL_CLR, state_msg);
    }
    return st;	
}
#endif

static ulong *policy_savings_comp_freqs = NULL;
static uint must_compute_policy_savings(signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom, uint *savings)
{
  ulong *cpu_f = freqs->cpu_freq;
  ulong fref = frequency_get_nominal_freq();
  ulong at_fref = 1;
  uint limit = ((dom->cpu == POL_GRAIN_CORE)?lib_shared_region->num_processes:1);

  *savings = 1;


  /* if we run at nominal, there is no savings */

  for (uint i = 0; (i < limit) && at_fref; i++){
    at_fref = at_fref && (cpu_f[i] == fref);
  }

  verbose_master(POLICY_SAVINGS_LVL,"Policy savins[%s]: running at nominal? at_ref %lu f[0] != %lu", node_name, at_fref, fref);

  if (at_fref){
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

static void compute_policy_savings(signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom)
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



  uint limit = ((dom->cpu == POL_GRAIN_CORE)?lib_shared_region->num_processes:1);
  uint procs = 0;


  /* Time reference and power reference is always computed based on the nominal CPU freq 
   * IF the default CPU freq is not the nominal (min_time for example), we project data
   * for the nominal CPU frequency
   */
  tref = get_time_nominal(ns);
  pref = get_power_nominal(ns);
 
  /* IF we are selecting CPU freqs per process, we aggregate and compute the average */
  for (uint p = 0; p < limit; p++){
    if (cpu_f[p] <= nominal){
        tnext += get_time_projected(ns, cpu_f[p]);
        pnext += get_power_projected(ns, cpu_f[p]);
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


void fill_cpus_out_of_cgroup(uint exc)
{
    int cpu_cnt, cpu_idx;
    if (exc == 0) return;
    cpu_cnt = arch_desc.top.cpu_count;

    for (cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++){
        if (freq_per_core[cpu_idx] == 0){
            debug("Reducing frequency of CPU %d", cpu_idx);
            freq_per_core[cpu_idx] = frequency_pstate_to_freq(frequency_get_num_pstates()-1);
        }
    }
}

#define DEBUG_CPUFREQ_COST 0

#if DEBUG_CPUFREQ_COST
static ulong total_cpufreq_cost_us = 0;
static ulong calls_cpufreq = 0;
#endif

timestamp init_last_phase, init_last_comp;



static void policy_new_phase(uint phase, signature_t *sig)
{
  verbose_master(3, "policy_new_phase summary computation %s (%u/%u)",
                 phase_to_str(phase), policy_cpu_bound, policy_mem_bound);

  sig_ext_t *se = (sig_ext_t *) sig->sig_ext;
  if (se != NULL)
  {
    for (uint ph = 0; ph < EARL_MAX_PHASES; ph++) se->earl_phase[ph].elapsed = 0;

    timestamp currt;
    timestamp_getfast(&currt);
    se->earl_phase[phase].elapsed = timestamp_diff(&currt, &init_last_phase, TIME_USECS);

    /* For COMP_BOUND we compute if CPU or MEM (or mix) */
    if (phase == APP_COMP_BOUND)
    {
      ullong elapsed_comp = timestamp_diff(&currt, &init_last_comp, TIME_USECS);

      /* policy_cpu_bound and policy_mem_bound are updated only in the APP_COMP_PHASE */
      verbose_master(3, "COMP_PHASE: cpu %u mem %u mix %u", policy_cpu_bound, policy_mem_bound , !policy_cpu_bound && !policy_mem_bound);

      if (policy_cpu_bound)      se->earl_phase[APP_COMP_CPU].elapsed = elapsed_comp;
      else if (policy_mem_bound) se->earl_phase[APP_COMP_MEM].elapsed = elapsed_comp;
      else                            se->earl_phase[APP_COMP_MIX].elapsed = elapsed_comp;  
    }
    memcpy(&init_last_phase, &currt, sizeof(timestamp));
    verbose_time_in_phases(sig);
  }
}


/* This function confgures freqs for apps consuming lot GPU. */
state_t policy_cpu_gpu_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_CPU_GPU);
    if (polsyms_fun.cpu_gpu_settings != NULL){
        polsyms_fun.cpu_gpu_settings(c,my_sig,freqs);
    }
    return EAR_SUCCESS;
}


/* This function confgures freqs based on IO criteria. */
state_t policy_io_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_IO_BOUND);
    if (polsyms_fun.io_settings != NULL)
    {
        polsyms_fun.io_settings(c, my_sig, freqs);
    }
    return EAR_SUCCESS;
}


/* This function sets again default frequencies after a setting modification */
state_t policy_restore_comp_configuration(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_COMP_BOUND);
    if (polsyms_fun.restore_settings != NULL) {
        polsyms_fun.restore_settings(c,my_sig,freqs);
        return policy_master_freq_selection(c,my_sig,freqs,dom,avg_freq, ALL_PROCESSES);
    } else {
        return EAR_SUCCESS;
    }
}


state_t policy_busy_waiting_configuration(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_BUSY_WAITING);    
    if (polsyms_fun.busy_wait_settings != NULL){
        polsyms_fun.busy_wait_settings(c,my_sig,freqs);
    }
    return EAR_SUCCESS;
}


state_t policy_domain_detail(int status, char* buff){
    switch (status){
        case POL_NOT_SUPPORTED:
            sprintf(buff, "%sNOT SUPPORTED%s", COL_RED, COL_CLR);
            return EAR_SUCCESS;
        case POL_GRAIN_CORE:
            sprintf(buff, "%sGRAIN CORE%s", COL_GRE, COL_CLR);
            return EAR_SUCCESS;
        case POL_GRAIN_NODE:
            sprintf(buff, "%sGRAIN NODE%s", COL_GRE, COL_CLR);
            return EAR_SUCCESS;
        default:
            break;
    }
    return EAR_ERROR;
}

state_t policy_show_domain(node_freq_domain_t *dom){
    char detail[64];

    verbose_master(2, "--- EAR policy domain ---");

    if (policy_domain_detail(dom->cpu, detail) == EAR_SUCCESS) {
        verbose_master(2, "      CPU: %s", detail);
    }
    else return EAR_ERROR;

    if (policy_domain_detail(dom->mem, detail) == EAR_SUCCESS) {
        verbose_master(2, "      MEM: %s", detail);
    }
    else return EAR_ERROR;

    if (policy_domain_detail(dom->gpu, detail) == EAR_SUCCESS) {
        verbose_master(2, "      GPU: %s", detail);
    }
    else return EAR_ERROR;

    if (policy_domain_detail(dom->gpumem, detail) == EAR_SUCCESS) {
        verbose_master(2, "  GPU_MEM: %s", detail);
    }
    else return EAR_ERROR;

    verbose_master(2, "-------------------------");

    return EAR_SUCCESS;
}

/**** End auxiliary functions ***/

/** This function is called automatically by the init_power_policy that loads policy symbols. */
state_t policy_init()
{
    polctx_t *c = &my_pol_ctx;

    char *cdyn_unc              = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_SET_IMCFREQ)       : NULL; // eUFS
    char *cimc_extra_th         = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_IMC_TH)            : NULL; // eUFS threshold
    char *cexclusive_mode       = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_EXCLUSIVE_MODE)    : NULL; // Job has the node exclusive
    char *cuse_earl_phases      = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_EARL_PHASES)       : NULL; // Detect application phases
    char *cenable_load_balance  = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_LOAD_BALANCE)      : NULL; // Load balance efficiency
    char *cuse_turbo_for_cp     = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_TURBO_CP)          : NULL; // Boost critical path processes
    char *ctry_turbo_enabled    = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_TRY_TURBO)         : NULL; // Boost turbo if policy sets max cpufreq
    char *cuse_energy_models    = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_USE_ENERGY_MODELS) : NULL; // Use energy models
    char *cmin_cpufreq          = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_MIN_CPUFREQ)       : NULL; // Minimum CPU freq.
    char *cfake_io_phase        = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_IO_FAKE_PHASE)     : NULL;
    char *cfake_bw_phase        = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_BW_FAKE_PHASE)     : NULL;
#if MPI_OPTIMIZED
    char *cear_mpi_opt          = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_MPI_OPT)           : NULL; // MPI optimizattion
#endif
    char *cignore_affinity_mask = ear_getenv(FLAG_NO_AFFINITY_MASK);

    char *cgpu_optimize = ear_getenv(GPU_ENABLE_OPT);
    if (cgpu_optimize != NULL) gpu_optimize = atoi(cgpu_optimize);
    verbose_master(POLICY_INFO, "%sPOLICY%s GPU optimization during GPU comp %s", COL_BLU, COL_CLR, (gpu_optimize? "Enabled":"Disabled"));

    state_t retg = EAR_SUCCESS, ret = EAR_SUCCESS;

    if (state_ok(policy_gpu_init_ctx(c))) {
        verbose_master(POLICY_INFO, "%sPOLICY%s GPU library init success.", COL_BLU, COL_CLR);
    } else {
        verbose_master(POLICY_INFO, "%sERROR%s GPU library init.", COL_RED, COL_CLR);
    }

    /* To compute time consumed in each phase */
    timestamp_getfast(&init_last_phase);
    memcpy(&init_last_comp, &init_last_phase, sizeof(timestamp));

    // Here it is a good place to print policy context:
    verbose_policy_ctx(POLICY_INFO, c);

    /* We read FLAG_USE_ENERGY_MODELS before calling to polsyms_fun.init() because this function
     * may needs `use_energy_models` to configure itself. */
    if (cuse_energy_models != NULL) {
        use_energy_models = atoi(cuse_energy_models);
    }
    verbose_master(POLICY_INFO, "%sPOLICY%s Using energy models: %u", COL_BLU, COL_CLR, use_energy_models);

    if (cfake_io_phase != NULL) fake_io_phase = atoi(cfake_io_phase);
    if (cfake_bw_phase != NULL) fake_bw_phase = atoi(cfake_bw_phase);

    if (fake_io_phase || fake_bw_phase) {
        verbose_master(0, "%sWARNING%s Using FAKE I/O phase: %u FAKE BW phase: %u",
                       COL_RED, COL_CLR, fake_io_phase, fake_bw_phase);
    }

    if (cdyn_unc != NULL && metrics_get(MGT_IMCFREQ)->ok) {
        conf_dyn_unc = atoi(cdyn_unc);
    }
    dyn_unc = conf_dyn_unc;

#if MPI_OPTIMIZED
    if (cear_mpi_opt != NULL) ear_mpi_opt = atoi(cear_mpi_opt);

    if (ear_mpi_opt) {
      verbose_master(2, "MPI optimization enabled.");
    }
#endif

    if (cimc_extra_th != NULL) {
        imc_extra_th = atof(cimc_extra_th);
    }

    if (cmin_cpufreq != NULL) {
        min_cpufreq = (ulong) atoi(cmin_cpufreq);
    }

    verbose_master(POLICY_INFO, "%sPOLICY%s Min. CPU freq. limited: %llu kHz"
            " / Min. IMC freq. limited: %llu kHz", COL_BLU, COL_CLR, min_cpufreq, min_imcfreq);

    // Calling policy plug-in init
    if (polsyms_fun.init != NULL) {

        ret = polsyms_fun.init(c);

        if (state_fail(ret)) {
            verbose_master(POLICY_INFO, "%sERROR%s Policy init: %s", COL_RED, COL_CLR, state_msg);
        }
    }

    // Read policy configuration
    if (cexclusive_mode != NULL) exclusive_mode = atoi(cexclusive_mode);
    verbose_master(POLICY_INFO, "%sPOLICY%s Running in exclusive mode: %u",
                   COL_BLU, COL_CLR, exclusive_mode);
    if (is_master)
    {
        if (state_fail(policy_set_cpus_governor(exclusive_mode)))
        {
            verbose(POLICY_INFO, "%sWARNING%s Governor configuration error: %s",
                    COL_YLW, COL_CLR, state_msg);
        }
    }

    if (cuse_earl_phases != NULL) use_earl_phases = atoi(cuse_earl_phases);
    verbose_master(POLICY_INFO, "%sPOLICY%s Phases detection: %u", COL_BLU, COL_CLR, use_earl_phases);


    /* Even though the policy could be at the Node granularity, we will use a list */
#if USE_GPUS
    gpuf_pol_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
    gpuf_pol_list_items = (const uint *)   metrics_gpus_get(MGT_GPU)->avail_count;
#endif // USE_GPUS
    node_freqs_alloc(&nf);
    node_freqs_alloc(&avg_nf);
    node_freqs_alloc(&last_nf);
#if POLICY_MPI_OPT
    /* This is to be used in mpi_calls */
    node_freqs_alloc(&per_process_node_freq);
#endif
    freq_per_core = calloc(MAX_CPUS_SUPPORTED, sizeof(ulong));
    signature_init(&policy_last_local_signature);
    signature_init(&policy_last_global_signature);
#if USE_GPUS
    policy_last_local_signature.gpu_sig.num_gpus  = metrics_gpus_get(MET_GPU)->devs_count;
    policy_last_global_signature.gpu_sig.num_gpus = policy_last_local_signature.gpu_sig.num_gpus;
#endif
    #if 0
    if (arch_desc.top.vendor == VENDOR_AMD) {
        enable_load_balance = 0;
    }
    #endif

    // TODO: This check is for the transition to the new environment variables.
    // It will be removed when SCHED_ENABLE_LOAD_BALANCE will be removed, in the next release.
    if (cenable_load_balance == NULL) {
        cenable_load_balance = (system_conf->user_type == AUTHORIZED) ? ear_getenv(SCHED_ENABLE_LOAD_BALANCE) : NULL;
        if (cenable_load_balance != NULL) {
            verbose(1, "%sWARNING%s %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_ENABLE_LOAD_BALANCE, FLAG_LOAD_BALANCE);
        }
    }
    if (cenable_load_balance != NULL) enable_load_balance = atoi(cenable_load_balance);
    /* EAR relies on the affinity mask, some special use cases (Seissol) modifies at runtime
     * the mask. Since EAR reads the mask during the initializacion, we offer this option
     * to switch off the utilization of the mask
     */
    if (cignore_affinity_mask != NULL){
      enable_load_balance = 0;
      ignore_affinity_mask = 1;
      verbose_master(POLICY_INFO,"%sPOLICY%s Affinity mask explicitly ignored. load balance off", COL_BLU, COL_CLR);
    }


    verbose_master(POLICY_INFO, "%sPOLICY%s Load balance: %u", COL_BLU, COL_CLR, enable_load_balance);

    if (cuse_turbo_for_cp != NULL) use_turbo_for_cp = atoi(cuse_turbo_for_cp);
    verbose_master(POLICY_INFO, "%sPOLICY%s Turbo for CP when app in the node is unbalanced: %u",
            COL_BLU, COL_CLR, use_turbo_for_cp);

    try_turbo_enabled = c->use_turbo;

    // TODO: This check is for the transition to the new environment variables.
    // It will be removed when SCHED_TRY_TURBO will be removed, in the next release.

    if (ctry_turbo_enabled == NULL) {

        ctry_turbo_enabled = (system_conf->user_type == AUTHORIZED) ? ear_getenv(SCHED_TRY_TURBO) : NULL;

        if (ctry_turbo_enabled != NULL) {
            verbose(1, "%sWARNING%s %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_TRY_TURBO, FLAG_TRY_TURBO);
        }
    }

    if (ctry_turbo_enabled != NULL) try_turbo_enabled = atoi(ctry_turbo_enabled);
    verbose_master(POLICY_INFO, "%sPOLICY%s Turbo for cbound app in the node: %u",
            COL_BLU, COL_CLR, try_turbo_enabled);

    // Read the domain of the loaded policy plugin
    if (polsyms_fun.domain != NULL) {
        ret = polsyms_fun.domain(c, &freqs_domain);
    } else {
        freqs_domain.cpu = POL_GRAIN_NODE;
        freqs_domain.mem = POL_NOT_SUPPORTED;
        freqs_domain.gpu = POL_NOT_SUPPORTED;
        freqs_domain.gpumem = POL_NOT_SUPPORTED;
    }

#if 0
    // TODO
    if (earl_default_domains.mem == POL_NOT_SUPPORTED){
    }
#endif

    if (policy_show_domain(&freqs_domain) == EAR_ERROR) {
        verbose_master(POLICY_INFO, "%sERROR%s Getting policy domains.", COL_RED, COL_CLR);
    }

    if (is_master) {
        // Powercap (?)
        pc_support_init(c);
    }

    fill_common_event(&curr_policy_event);
    REPORT_OPT_ACCURACY(OPT_NOT_READY);

    /* We have migrated frequency set from eard to earl */
    policy_get_default_freq(&nf);

    if (is_master) {
      for (uint lp = 0 ; lp < lib_shared_region->num_processes; lp++) {
        sig_shared_region[lp].unbalanced = 0;
        sig_shared_region[lp].new_freq = nf.cpu_freq[0];
      }
    }

    /* What do we need at this point in the signature ? */
    policy_master_freq_selection(c, &policy_last_local_signature, &nf,
                                 &freqs_domain, &avg_nf, ALL_PROCESSES);

    if (is_master && !ignore_affinity_mask)
    {
        policy_setup_priority();
    }

    if ((ret == EAR_SUCCESS) && (retg == EAR_SUCCESS)) return EAR_SUCCESS;
    else return EAR_ERROR;
}

/* This function loads specific CPU and GPU policy functions and calls the initialization functions */
state_t init_power_policy(settings_conf_t *app_settings, resched_t *res)
{
    char basic_path[SZ_PATH_INCOMPLETE];
    char user_path[SZ_PATH_INCOMPLETE];

    conf_install_t *install_data = &app_settings->installation;

    // This part of code is to decide which policy to load based on default and hacks
    char *obj_path       = (system_conf->user_type == AUTHORIZED) ? ear_getenv(HACK_POWER_POLICY) : NULL;
    char *ins_path       = (system_conf->user_type == AUTHORIZED) ? ear_getenv(HACK_EARL_INSTALL_PATH) : NULL; // TODO: this path may be recicled without calling again getenv

#if SHOW_DEBUGS
    if (is_master)
    {
        if (obj_path != NULL)
        { 
            debug("%s = %s", HACK_POWER_POLICY, obj_path);
        } else
        {
            debug("%s undefined", HACK_POWER_POLICY);
        }
        if (ins_path != NULL)
        {
            debug("%s = %s", HACK_EARL_INSTALL_PATH,ins_path);
        } else
        {
            debug("%s undefined",HACK_EARL_INSTALL_PATH);
        }
    }
#endif

    char *app_mgr_policy = ear_getenv(FLAG_APP_MGR_POLICIES);

    int app_mgr = 0;
    if (app_mgr_policy != NULL){
        app_mgr = atoi(app_mgr_policy);
    }

    char *monitoring = "monitoring";

    char *my_policy_name = app_settings->policy_name;

    if (earl_default_domains.cpu == POL_NOT_SUPPORTED) {
        my_policy_name = monitoring;
        verbose_master(POLICY_INFO, "%sINFO%s Forcing policy: %s", COL_BLU, COL_CLR, my_policy_name);
    }

    // Policy file selection
#if !USER_TEST
    if ((obj_path == NULL && ins_path == NULL) || app_settings->user_type != AUTHORIZED) {
#else
    if (obj_path == NULL && ins_path == NULL) {
#endif
        if (!app_mgr) {
            xsnprintf(basic_path,sizeof(basic_path),"%s/policies/%s.so",install_data->dir_plug,my_policy_name);
        } else {
            xsnprintf(basic_path,sizeof(basic_path),"%s/policies/app_%s.so",install_data->dir_plug,my_policy_name);
        }
        obj_path=basic_path;
    } else {
        if (obj_path == NULL){
            if (ins_path != NULL){
                xsnprintf(user_path,sizeof(user_path),"%s/plugins/policies", ins_path);
            }else{
                xsnprintf(user_path,sizeof(user_path),"%s/policies", install_data->dir_plug);
            }
            if (!app_mgr){
                xsnprintf(basic_path,sizeof(basic_path),"%s/%s.so",user_path,my_policy_name);		
            }else{
                xsnprintf(basic_path,sizeof(basic_path),"%s/app_%s.so",user_path,my_policy_name);		
            }
            obj_path=basic_path;
        }
    }

    // Load policy plugin
    verbose_master(POLICY_INFO, "%sINFO%s Loading policy: %s", COL_BLU, COL_CLR, obj_path);
    if (state_fail(symplug_open(obj_path, (void **)&polsyms_fun, polsyms_nam, polsyms_n))) {
        verbose_master(POLICY_INFO, "%sERROR%s Loading %s: %s", COL_RED, COL_CLR, obj_path, state_msg);
    }
#if 0
    if (state_fail(policy_load(obj_path, &polsyms_fun))) {
        verbose_master(POLICY_INFO, "%sERROR%s Loading %s: %s", COL_RED, COL_CLR, obj_path, state_msg);
    }
#endif

    ear_frequency                 = DEF_FREQ(app_settings->def_freq);
	init_def_cpufreq_khz          = ear_frequency;
    my_pol_ctx.app	              = app_settings;
    my_pol_ctx.reconfigure        = res;
    my_pol_ctx.user_selected_freq = DEF_FREQ(app_settings->def_freq);
    my_pol_ctx.reset_freq_opt     = get_ear_reset_freq();
    my_pol_ctx.ear_frequency      = &ear_frequency;
    my_pol_ctx.num_pstates	      = frequency_get_num_pstates();
    my_pol_ctx.use_turbo 	      = ear_use_turbo;
    my_pol_ctx.affinity  	      = ear_affinity_is_set;
    my_globalrank                 = masters_info.my_master_rank;
    my_pol_ctx.pc_limit	          = app_settings->pc_opt.current_pc;

#if MPI
    if (PMPI_Comm_dup(masters_info.new_world_comm, &my_pol_ctx.mpi.comm) != MPI_SUCCESS) {
        verbose_master(POLICY_INFO, "%sERROR%s Duplicating COMM_WORLD in policy.", COL_RED, COL_CLR);
    }
    if (masters_info.my_master_rank >=0) {
        if (PMPI_Comm_dup(masters_info.masters_comm, &my_pol_ctx.mpi.master_comm) != MPI_SUCCESS) {
            verbose_master(POLICY_INFO, "%sERROR%s Duplicating master_comm in policy", COL_RED, COL_CLR);
        }
    }
#endif

    // We fill available pstates info for both CPU and IMC APIs. this is a must in order to use
    // functions for converting a pstate value to a khz frequency, and viceversa.
    // The called function is defined at the end of this file.
    if (state_fail(fill_mgt_api_pstate_data(MGT_CPUFREQ, &cpu_pstate_lst, &cpu_pstate_cnt))) {
        verbose_master(POLICY_INFO, "%sERROR%s Policy init could not read mgt_cpufreq: %s",
                       COL_RED, COL_CLR, state_msg);
    }
    if (state_fail(fill_mgt_api_pstate_data(MGT_IMCFREQ, &imc_pstate_lst, &imc_pstate_cnt))) {
        verbose_master(POLICY_INFO, "%sERROR%s Policy init could not read mgt_cpufreq: %s",
                       COL_RED, COL_CLR, state_msg);
    }

    // Set the minimum CPU and IMC freqs. to the max P-States set by the Admin in ear.conf

    debug("max cpu pstate: %u / cpu pstate cnt %u",
          (uint) app_settings->cpu_max_pstate, cpu_pstate_cnt);

    uint max_cpu_ps = cpu_pstate_cnt - 1;
    if (strcmp(my_policy_name, "monitoring") == 0)
    {
        is_monitoring = 1;
    } else
    {
        max_cpu_ps = ear_min(max_cpu_ps, (uint) app_settings->cpu_max_pstate);
    }

    if (state_fail(pstate_pstofreq(cpu_pstate_lst, cpu_pstate_cnt, max_cpu_ps, &min_cpufreq)))
    {
        verbose_master(POLICY_INFO, "%sERROR%s Policy init could not read cpu_max_pstate: %s",
                       COL_RED, COL_CLR, state_msg);
    }

    verbose_master(POLICY_INFO, "%sPOLICY%s System Min. CPU freq. limited: %llu kHz ", COL_BLU, COL_CLR, min_cpufreq);


    debug("max imc pstate: %u / imc pstate cnt %u",
          (uint) app_settings->imc_max_pstate, imc_pstate_cnt);

    max_policy_imcfreq_ps = ear_min(imc_pstate_cnt - 1, (uint) app_settings->imc_max_pstate);
    if (state_fail(pstate_pstofreq(imc_pstate_lst, imc_pstate_cnt, max_policy_imcfreq_ps, &min_imcfreq)))
    {
        verbose_master(POLICY_INFO, "%sERROR%s Policy init could not read imc_max_pstate: %s",
                       COL_RED, COL_CLR, state_msg);
    }
    verbose_master(POLICY_INFO, "%sPOLICY%s System Min. IMC freq. limited: %llu kHz ", COL_BLU, COL_CLR, min_imcfreq);

    return policy_init();
}


/* Policy functions previously included in models */

/*
 *
 * Policy wrappers
 *
 */



/* This is the main function for frequency selection at application level */
/* The signatures are used first the last signatur computed is a global variable, second signatures are in the shared memory */
/* freq_set and ready are output values */
/* There is not a GPU part here, still pending */
/* The CPU freq and GPu freq are set in this function, so freq_set is just a reference for the CPU freq selected */


state_t policy_app_apply(ulong *freq_set, int *ready)
{
    polctx_t *c=&my_pol_ctx;
    state_t st = EAR_SUCCESS;
    node_freqs_t *freqs = &nf;
    *ready = EAR_POLICY_READY;
    if (polsyms_fun.app_policy_apply == NULL){
        *ready = EAR_POLICY_LOCAL_EV;
        return st;
    }
    if (!eards_connected() || (masters_info.my_master_rank<0)){
        *ready = EAR_POLICY_CONTINUE;
        return st;
    }
    if (freqs_domain.cpu == POL_GRAIN_CORE) memset(freqs->cpu_freq,0,sizeof(ulong)*MAX_CPUS_SUPPORTED);
    st=polsyms_fun.app_policy_apply(c, &policy_last_global_signature, freqs,ready);
    if (*ready == EAR_POLICY_READY){
        debug("Average frequency after app_policy is %lu",*freq_set );
        //policy_cpu_freq_selection(&policy_last_global_signature,freqs);
        st = policy_master_freq_selection(c,&policy_last_global_signature,freqs,&freqs_domain,&avg_nf, ALL_PROCESSES);
        *freq_set = avg_nf.cpu_freq[0];
    }
    return st;
}

static int is_all_ready()
{
    if (my_pol_ctx.num_gpus == 0) gpu_ready = EAR_POLICY_READY;
    if ((cpu_ready == EAR_POLICY_READY) && (gpu_ready == EAR_POLICY_READY)) return 1;
    return 0;
}

static int is_try_again()
{
    if ((cpu_ready == EAR_POLICY_TRY_AGAIN) || (gpu_ready == EAR_POLICY_TRY_AGAIN)) return 1;
    return 0;
}

/* This is the main function for frequency selection at node level */
/* my_sig is an input and freq_set and ready are output values */
/* The CPU and GPU freqs are set in this function, so freq_set is just a reference for the CPU freq selected */
state_t policy_node_apply(signature_t *my_sig, ulong *freq_set, int *ready)
{
    uint busy, iobound, mpibound, earl_phase_classification;
    uint jobs_in_node;

    polctx_t *c = &my_pol_ctx;

    node_freqs_t *freqs = &nf;

    state_t st = EAR_ERROR;

    verbose_master(POLICY_PHASES, "EAR policy....");

    cpu_ready = EAR_POLICY_NO_STATE;
    gpu_ready = EAR_POLICY_NO_STATE;

    if (!eards_connected() || (masters_info.my_master_rank < 0)) {
#if EARL_LIGHT
        if (is_master) {
            clean_signatures(lib_shared_region, sig_shared_region);
        }
#endif
        *ready = EAR_POLICY_CONTINUE;
        return EAR_SUCCESS;
    }

    if (!is_monitor_mpi_calls_enabled()) enable_load_balance = 0;

    verbose_master(POLICY_INFO + 1, "%sPOLICY%s Policy node apply", COL_BLU, COL_CLR);

    //nodemgr_get_num_jobs_attached(&jobs_in_node);
    jobs_in_node = 1;
    if (jobs_in_node > 1) {
      verbose_jobs_in_node(2, node_mgr_data, node_mgr_job_info);
    }

    if (jobs_in_node > 1) dyn_unc = 0;
    else                  dyn_unc = conf_dyn_unc;

    /* End update */

    //  NEW CLASSIFY
    is_network_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &mpibound);
    is_io_bound(     &lib_shared_region->job_signature, lib_shared_region->num_cpus, &iobound);
    classify(iobound, mpibound, &earl_phase_classification);
    is_cpu_busy_waiting(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &busy);

    gpu_activity_state(&lib_shared_region->job_signature, &curr_gpu_state); // GPU phase classification
    if (curr_gpu_state & _GPU_Idle)
    {
        verbose_master(POLICY_GPU, "%sGPU idle phase%s", COL_BLU, COL_CLR);
    }

    /* Applications gpu bound cannot be applied as CPU-GPU */
    if ((curr_gpu_state & _GPU_Bound) && (busy != BUSY_WAITING))
    {
      earl_phase_classification = APP_CPU_GPU;
    }

    //  NEW CLASSIFY
    verbose_master(POLICY_PHASES, "EARL phase %s: Busy Waiting %u / I/O bound %u / MPI bound %u / Using phases %u",
        phase_to_str(earl_phase_classification), busy, iobound, mpibound, use_earl_phases);

    if (use_earl_phases)
    {
      REPORT_OPT_ACCURACY(OPT_OK);
      /* CPU-GPU Code */
      if (earl_phase_classification == APP_CPU_GPU)
      {
            // verbose_master(POLICY_PHASES,"%sCPU-GPU application%s", COL_BLU,COL_CLR);
            verbose_master(2, "%sCPU-GPU application%s (%s)", COL_BLU, COL_CLR, node_name);

            // TODO: The below function does not modify avg_nf, but it's used just after the function call.
            st = policy_cpu_gpu_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf);
            *freq_set = avg_nf.cpu_freq[0];

            policy_new_phase(last_earl_phase_classification, my_sig);

            last_earl_phase_classification = earl_phase_classification;
            last_gpu_state = curr_gpu_state;

            policy_master_freq_selection(c,my_sig,freqs,&freqs_domain,&avg_nf, ALL_PROCESSES);

            clean_signatures(lib_shared_region, sig_shared_region);

            if (is_all_ready())
            {
              *ready = EAR_POLICY_READY;
            } else {
              *ready = EAR_POLICY_CONTINUE;
            }
            return EAR_SUCCESS;
      }

      /* IO Use case */
      if (earl_phase_classification == APP_IO_BOUND)
      {
          verbose_master(POLICY_PHASES,"%sPOLICY%s I/O configuration", COL_BLU, COL_CLR);

          // TODO Below function always returns EAR_SUCCESS
          st = policy_io_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf);

          // TODO: GPU ?
          // TODO: avg_nf is not set on the above function
          *freq_set = avg_nf.cpu_freq[0];

          policy_new_phase(last_earl_phase_classification, my_sig);

          last_earl_phase_classification = earl_phase_classification;
          last_gpu_state = curr_gpu_state;

          policy_master_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf, ALL_PROCESSES);

          clean_signatures(lib_shared_region, sig_shared_region);

          if (is_all_ready())
          {
              *ready = EAR_POLICY_READY;
          } else
          {
              *ready = EAR_POLICY_CONTINUE;
          }
          return EAR_SUCCESS;
      }

      /* Restore configuration to COMP or MPI if needed */
      if ((earl_phase_classification == APP_COMP_BOUND) ||
          (earl_phase_classification == APP_MPI_BOUND))
      {
          /* Are we in a busy waiting phase ? */
          if (busy == BUSY_WAITING)
          {
              verbose_master(POLICY_PHASES, "%sPOLICY%s CPU Busy waiting configuration (%s)",
                             COL_BLU, COL_CLR, node_name);

              st = policy_busy_waiting_configuration(c, my_sig, freqs, &freqs_domain, &avg_nf);

              // TODO: avg_nf is not set on the above function
              *freq_set = avg_nf.cpu_freq[0];

              policy_master_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf, ALL_PROCESSES);

              last_gpu_state = curr_gpu_state;
              policy_new_phase(last_earl_phase_classification, my_sig);

              last_earl_phase_classification = APP_BUSY_WAITING;
              clean_signatures(lib_shared_region, sig_shared_region);

              if (is_all_ready())
              {
                  *ready = EAR_POLICY_READY;
              } else {
                  *ready = EAR_POLICY_CONTINUE;
              }

              return EAR_SUCCESS;
          }

          /* If not, we should restore to default settings */
          if ((last_earl_phase_classification == APP_BUSY_WAITING) ||
              (last_earl_phase_classification == APP_IO_BOUND) ||
              !(last_gpu_state & curr_gpu_state))
          {
              verbose_master(POLICY_PHASES, "%sPOLICY%s COMP-MPI configuration", COL_BLU, COL_CLR);

              st = policy_restore_comp_configuration(c, my_sig, freqs,
                                                     &freqs_domain, &avg_nf);

              *freq_set = avg_nf.cpu_freq[0];

              policy_new_phase(last_earl_phase_classification, my_sig);

              last_earl_phase_classification = earl_phase_classification;
              last_gpu_state = curr_gpu_state;

              if (earl_phase_classification == APP_COMP_BOUND) timestamp_getfast(&init_last_comp);

              clean_signatures(lib_shared_region, sig_shared_region);

              *ready = EAR_POLICY_CONTINUE;

              return st;
          }
      } // COMP_BOUND OR MPI_BOUND
    } // EAR phase decisions

    /* Here we are COMP or MPI bound AND not busy waiting. */
    verbose_master(POLICY_PHASES,
            "%sPOLICY%s COMP or MPI bound and not busy waiting: Applying node policy...",
            COL_BLU, COL_CLR);
    policy_new_phase(last_earl_phase_classification, my_sig);

    last_earl_phase_classification     = earl_phase_classification;

    if (earl_phase_classification == APP_COMP_BOUND) timestamp_getfast(&init_last_comp);
    is_cpu_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &policy_cpu_bound);
    is_mem_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &policy_mem_bound);

    if (fake_io_phase)                 last_earl_phase_classification = APP_IO_BOUND;
    if (fake_bw_phase)                 last_earl_phase_classification = APP_BUSY_WAITING;

    last_gpu_state = curr_gpu_state;

    if (polsyms_fun.node_policy_apply)
    {
        /* Initializations */
        signature_t node_sig;
        signature_copy(&node_sig, my_sig);

        // We remove here the GPU power, as models are not trained with it.
        node_sig.DC_power = sig_node_power(my_sig);
        signature_copy(&policy_last_local_signature, &node_sig);

        if ((cpu_ready != EAR_POLICY_READY) &&
            (freqs_domain.cpu == POL_GRAIN_CORE))
        {
            memset(freqs->cpu_freq, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
        }

        /* CPU specific function is applied here */
        st = polsyms_fun.node_policy_apply(c, &node_sig, freqs, &cpu_ready);

        if (cpu_ready) compute_policy_savings(&node_sig, freqs, &freqs_domain);

        /* Depending on the policy status and granularity we adapt the CPU freq selection */
        if (is_all_ready() || is_try_again())
        {
            /* This function checks the powercap and the processor affinity,
             * finally it sets the CPU freq. asking the eard to do it. */
            st = policy_master_freq_selection(c, &node_sig, freqs, &freqs_domain, &avg_nf, ALL_PROCESSES);
            *freq_set = avg_nf.cpu_freq[0];
        } /* Stop*/
    } else if (polsyms_fun.app_policy_apply != NULL)
    {
        *ready = EAR_POLICY_GLOBAL_EV;
    } else
    {
        if (c != NULL)
        {
            *freq_set = DEF_FREQ(c->app->def_freq);
            cpu_ready = EAR_POLICY_READY;
        }
    }

    if (is_all_ready())
    {
        *ready = EAR_POLICY_READY;
    } else
    {
        *ready = EAR_POLICY_CONTINUE;
    }

    if (VERB_ON(POLICY_INFO))
    {
        if (*ready == EAR_POLICY_READY)
        {
            verbose_master(POLICY_INFO, "%sNode policy ready%s", COL_GRE, COL_CLR);
        }
        verbose_master(POLICY_INFO + 1, "%sPOLICY%s CPU freq. policy ready: %d\n"
                "GPU policy ready: %d\nGlobal node readiness: %d", COL_BLU, COL_CLR,
                cpu_ready, gpu_ready, *ready);
    }

    clean_signatures(lib_shared_region, sig_shared_region);

    return EAR_SUCCESS;
}


state_t policy_get_default_freq(node_freqs_t *freq_set)
{
    if (masters_info.my_master_rank < 0)
    {
        return EAR_SUCCESS;
    }

    if (polsyms_fun.get_default_freq != NULL) {
        return polsyms_fun.get_default_freq(&my_pol_ctx, freq_set, NULL);
    }

    verbose(POLICY_INFO + 1, "%sWARNING%s The policy plug-in has no symbol for getting the default frequency. Default frequency is %lu",
            COL_RED, COL_CLR, DEF_FREQ(my_pol_ctx.app->def_freq));

    // Below code overwrites the approach where only the index 0 of the destination array was filled.
    // A conflict appeared for a plug-in with fine-grain cpu domain but not get_default_freq implementation.

    // TODO: As the logic of this function is always the same,
    // it would be preferably if we avoid iterating all times we enter into this function.
    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++)
    {
        freq_set->cpu_freq[i] = DEF_FREQ(my_pol_ctx.app->def_freq);
    }

    return EAR_SUCCESS;
}


/* This function sets the default freq for both (CPU and GPU policies). */
state_t policy_set_default_freq(signature_t *sig)
{
    signature_t s;

    /* WARNING: sig can be NULL */

    if (masters_info.my_master_rank < 0 ) return EAR_SUCCESS;
    verbose_master(2,"policy_set_default_freq");

#if USE_GPUS
    s.gpu_sig.num_gpus = my_pol_ctx.num_gpus;
#endif

    if (polsyms_fun.restore_settings != NULL) {

        state_t st = polsyms_fun.restore_settings(&my_pol_ctx, sig, &nf);
        if (state_ok(st)) {
            return policy_master_freq_selection(&my_pol_ctx, &s, &nf, &freqs_domain, &avg_nf, ALL_PROCESSES);
        } else {
            return st;
        }
    } else {
        return EAR_ERROR;
    }
}

static void compute_energy_savings(signature_t *curr,signature_t *prev)
{
	// float performance_prev, performance_curr;
	float psaving = 0;
	float tpenalty = 0;
	float esaving = 0;
  ullong saving_elapsed;
#if 0
	double tflops;
#endif
//
	
	sig_ext_t *sig_ext = curr->sig_ext;

  if (energy_policy_saving == 0) return;

  #if 0

	if ((prev->Gflops == 0) || (curr->Gflops == 0)) {
		esaving = 0;
		return;
	}

	/* The Energy saving is the ration bettween GFlops/W */
	performance_prev = prev->Gflops/prev->DC_power;
	performance_curr = curr->Gflops/curr->DC_power;


// #if 0
    if (state_fail(compute_job_node_gflops(sig_shared_region,
                    lib_shared_region->num_processes, &tflops))) {
        verbose(3, "%sWARNING%s Error on computing job's node GFLOP/s rate: %s",
                COL_RED, COL_CLR, state_msg);
    }

    verbose_master(2, "MR[%d] Curr performance %.4f (%.2f/%.2f) Last performance %.4f (%.2f/%.2f)", masters_info.my_master_rank, \
            performance_curr, curr->Gflops, curr->DC_power, \
            performance_prev, prev->Gflops, prev->DC_power); 
// #endif

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
  verbose_master(POLICY_SAVINGS_LVL, "Policy savins[%s]: Using elapsed time %llu", node_name, saving_elapsed);


	sig_ext->saving   = esaving * saving_elapsed;
	sig_ext->psaving  = psaving * saving_elapsed;
	sig_ext->tpenalty = tpenalty * saving_elapsed;

    REPORT_SAVING(ENERGY_SAVING, (llong) esaving);
    REPORT_SAVING(POWER_SAVING,  (llong) psaving);
  //REPORT_SAVING(PERF_PENALTY , (ulong)tpenalty);

}

/* Uncomment this option to use energy savings estimations to decide if policy is ok */
//#define USE_ENERGY_SAVING
/* This function checks the CPU freq selection accuracy (only CPU), ok is an output */
state_t policy_ok(signature_t *curr, signature_t *prev, int *ok)
{
    if (masters_info.my_master_rank < 0) {
        *ok = 1;
        return EAR_SUCCESS;
    }

    if (curr == NULL || prev == NULL) {
        *ok = 1;
        // TODO: Should we report OPT_OK event value?
        return_msg(EAR_ERROR, Generr.input_null);
    }

    polctx_t *c = &my_pol_ctx;
    state_t ret, retg = EAR_SUCCESS;
    uint num_ready;
    uint jobs_in_node;

    sig_ext_t *sig_ext = curr->sig_ext;
    sig_ext->saving = 0;

    /* Update node info */
    /* update_earl_node_mgr_info(); */
    nodemgr_get_num_jobs_attached(&jobs_in_node);

    // By now, only the master will reach this instruction.
    verbose(2, "%sPOLICY OK%s There are %u jobs in the node", COL_BLU, COL_CLR, jobs_in_node);

    if (jobs_in_node > 1) {
      verbose_jobs_in_node(2, node_mgr_data, node_mgr_job_info);
    }
    /* End update */

    /* estimate_power_and_gbs(curr,lib_shared_region,sig_shared_region, node_mgr_job_info, &new_sig, PER_JOB , -1); */
    verbose(3, "Job's GB/s: %.2lf - Job's Power: %.2lf", curr->GBS, curr->DC_power);

    if (polsyms_fun.ok != NULL) {
        /* We wait for node signature computation */
        if (!are_signatures_ready(lib_shared_region, sig_shared_region, &num_ready)) {
            *ok = 1;

            REPORT_OPT_ACCURACY(OPT_OK);
            
            verbose(2, "%sPOLICY OK%s Not all signatures are ready.", COL_GRE, COL_CLR);

            return EAR_SUCCESS;
        }

        pc_support_compute_next_state(c, &my_pol_ctx.app->pc_opt, curr);

        ret = polsyms_fun.ok(c, curr, prev, ok);

        char *ok_color = COL_GRE;

        if (*ok == 0) {
            // REPORT_OPT_ACCURACY(OPT_NOT_OK);

            policy_new_phase(last_earl_phase_classification, curr);

            last_earl_phase_classification = APP_COMP_BOUND;
            last_gpu_state = _GPU_Comp;

            timestamp_getfast(&init_last_comp);

            ok_color = COL_RED;
        }
        verbose(2, "%sPOLICY OK%s Decided by the policy plug-in.", ok_color, COL_CLR);

#if 0
        /* Migrated to states */
        /* And then we mark as not ready */
        verbose(3, "%sPOLICY OK%s Cleaning signatures...", COL_BLU, COL_CLR);
        clean_signatures(lib_shared_region,sig_shared_region);
#endif

        // TODO By now, retg is not used since its definition
        if ((ret != EAR_SUCCESS) || (retg != EAR_SUCCESS)) {

            uint opt_acc_val = (*ok) ? OPT_OK : OPT_NOT_OK;
            REPORT_OPT_ACCURACY(opt_acc_val);

            return EAR_ERROR;
        }
    } else {

        *ok = 1;

        verbose(2, "%sPOLICY OK%s The policy plug-in does not implement this method.", COL_GRE, COL_CLR);
    }

    /* If signatures are ok, not different phase, we compute the energy savings estimations */
    if (!is_monitoring && *ok) {

        compute_energy_savings(curr, prev);

        #if USE_ENERGY_SAVING
        if (sig_ext->saving < 0) {

          *ok = 0;

          verbose(2, "%sPOLICY OK%s We are not saving energy.", COL_RED, COL_CLR);
        }
        #endif
    }

    // We finally report the OPT_ACCURACY
    uint opt_acc_val = (*ok) ? OPT_OK : OPT_NOT_OK;
    REPORT_OPT_ACCURACY(opt_acc_val);

    return EAR_SUCCESS;
}

/* This function returns the maximum mumber of attempts to select the optimal CPU frequency */
state_t policy_max_tries(int *intents)
{ 
    polctx_t *c=&my_pol_ctx;
    if (masters_info.my_master_rank < 0 ){*intents = 100000; return EAR_SUCCESS;}
    if (polsyms_fun.max_tries!=NULL){
        return	polsyms_fun.max_tries( c,intents);
    }else{
        *intents=1;
        return EAR_SUCCESS;
    }
}


/* This function is executed at application end */
state_t policy_end()
{
    polctx_t *c = &my_pol_ctx;

    verbose_master(2,"Total number of MPI calls %llu", total_mpi);

#if MPI_OPTIMIZED
    if (ear_mpi_opt) {
        verbose(1, "[%d] Total number of MPI calls with optimization: %u", my_node_id,  total_mpi_optimized);
    }
#endif

#if DEBUG_CPUFREQ_COST
    verbose_master(1, "Overhead of cpufreq setting %lu us: Total calls %lu: Cost per call %f",
                   total_cpufreq_cost_us, calls_cpufreq, (float)total_cpufreq_cost_us/(float)calls_cpufreq);
#endif

    policy_reset_priority();
    if (is_master && exclusive_mode)
    {
        policy_restore_cpus_governor();
    }

    preturn(polsyms_fun.end, c);
}


/** LOOPS */
/* This function is executed  at loop init or period init */
state_t policy_loop_init(loop_id_t *loop_id)
{
    preturn(polsyms_fun.loop_init, &my_pol_ctx, loop_id);
}

/* This function is executed at each loop end */
state_t policy_loop_end(loop_id_t *loop_id)
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.loop_end,c, loop_id);
}

/* This function is executed at each loop iteration or beginning of a period */
state_t policy_new_iteration(signature_t *sig)
{
      

    // TODO: Here we are doing things useless in monitoring mode.
    /* This validation is common to all policies */
    /* Check cpu idle or gpu idle */
    uint last_phase_io_bnd = (last_earl_phase_classification == APP_IO_BOUND);
    uint last_phase_bw     = (last_earl_phase_classification == APP_BUSY_WAITING);


    if (last_phase_io_bnd || last_phase_bw) {
        verbose_master(POLICY_INFO , "%sPOLICY%s Validating CPU phase: (GFlop/s = %.2lf, IO = %.2lf CPI %.2lf)",
                COL_BLU, COL_CLR, sig->Gflops, sig->IO_MBS, sig->CPI );

        uint restore = 0;

        if (last_phase_io_bnd && (sig->IO_MBS < phases_limits.io_th))         restore = 1; 
        if (last_phase_bw     && (sig->CPI > phases_limits.cpi_busy_waiting)) restore = 1;
        /* WARNING: Below condition won't be accomplished never. */
        if ((sig->Gflops/lib_shared_region->num_cpus) > phases_limits.gflops_busy_waiting)                  restore = 1;

        if (restore){
            verbose_master(POLICY_INFO, "Phase has changed, restoring");
            /* We must reset the configuration */
            policy_new_phase(last_earl_phase_classification, sig);
            last_earl_phase_classification = APP_COMP_BOUND;
            last_gpu_state = _GPU_Comp;
            timestamp_getfast(&init_last_comp);
            REPORT_OPT_ACCURACY(OPT_TRY_AGAIN);
            return EAR_ERROR;
        }
    }

#if USE_GPUS
    if (last_gpu_state & _GPU_Idle) {

        verbose_master(POLICY_GPU, "%sPOLICY%s Validating GPU phase: (#GPUs) %u",
                COL_BLU, COL_CLR, sig->gpu_sig.num_gpus);

        for (int i = 0; i < sig->gpu_sig.num_gpus; i++){
            if (sig->gpu_sig.gpu_data[i].GPU_util > 0){
                last_gpu_state = _GPU_Comp;
                REPORT_OPT_ACCURACY(OPT_TRY_AGAIN);
                return EAR_ERROR;
            }
        }
    }
#endif

    state_t st = EAR_SUCCESS;
    if (polsyms_fun.new_iter != NULL) {
        st = polsyms_fun.new_iter(&my_pol_ctx, sig);
    }
    return st;
}

/** MPI CALLS */

/* This function is executed at the beginning of each mpi call: Warning! it could introduce a lot of overhead*/
static signature_t aux_per_process;
static node_freq_domain_t per_process_dom = {
  .cpu = POL_GRAIN_CORE,
  .mem = POL_NOT_SUPPORTED,
  .gpu = POL_NOT_SUPPORTED,
  .gpumem = POL_NOT_SUPPORTED};



state_t policy_mpi_init(mpi_call call_type)
{
#if POLICY_MPI_OPT
    int process_id = NO_PROCESS;

    polctx_t *c = &my_pol_ctx;

    total_mpi++;

    if (polsyms_fun.mpi_init != NULL) {

        if (state_ok(polsyms_fun.mpi_init(c, call_type, &per_process_node_freq, &process_id))) {

            if (process_id != NO_PROCESS) {

                total_mpi_optimized++;

                if (state_fail(policy_freq_selection(c, &aux_per_process,
                               &per_process_node_freq,  &per_process_dom, NULL, process_id))) {

                    verbose(2, "%sERROR%s CPU freq. selection failure at MPI call init.",
                            COL_RED, COL_CLR);

                    return EAR_ERROR;

                }
            }
        } else {
            verbose(2, "%sERROR%s At MPI call init: %s", COL_RED, COL_CLR, state_msg);
        }
    } else {
        return EAR_SUCCESS;
    }
#endif
    return EAR_SUCCESS;
}

state_t policy_restore_to_mpi_pstate(uint index)
{
#if !SINGLE_CONNECTION && MPI_OPTIMIZED
  polctx_t *c = &my_pol_ctx;
  ulong f = sig_shared_region[my_node_id].mpi_freq;
  if (index != process_get_target_pstate()){
    pstate_t *sel_pstate, *pstate_list;
    pstate_list = metrics_get(MGT_CPUFREQ)->avail_list;
    sel_pstate = &pstate_list[index];
    f = sel_pstate->khz;
  }
  if (traces_are_on()){
    traces_generic_event(ear_my_rank, my_node_id , TRA_CPUF_RAMP_UP, f);
  }
  verbose(2, "restore CPU freq in process %d to %lu", my_node_id, f);
  per_process_node_freq.cpu_freq[my_node_id] = f;
  if (state_fail(policy_freq_selection(c, &aux_per_process,
      &per_process_node_freq,  &per_process_dom, NULL, my_node_id))) {
      verbose(2, "%sERROR%s CPU freq. restore in MPI opt", COL_RED, COL_CLR);
      return EAR_ERROR;
  }
#endif
  return EAR_SUCCESS;
}


/* This function is executed after each mpi call:
 * Warning! it could introduce a lot of overhead. */
state_t policy_mpi_end(mpi_call call_type)
{
#if POLICY_MPI_OPT
    int process_id = NO_PROCESS;

    state_t st;

    polctx_t *c = &my_pol_ctx;
    if (polsyms_fun.mpi_end != NULL) {

        st = polsyms_fun.mpi_end(c,call_type, &per_process_node_freq, &process_id);
        if (state_fail(st)) {
            verbose(2, "%sERROR%s at policy MPI call end: %s", COL_RED, COL_CLR, state_msg);
        }

        if (process_id != NO_PROCESS) {
            if (state_fail(policy_freq_selection(c, &aux_per_process, 
                            &per_process_node_freq, &per_process_dom, NULL, process_id))){

                verbose(2, "CPU freq failure in policy_mpi_end");
            }
        }

        return st;
    } else return EAR_SUCCESS;
#else
    return EAR_SUCCESS;
#endif
}

/** GLOBAL EVENTS */

/* This function is executed when a reconfiguration needs to be done*/
state_t policy_configure()
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.configure, c);
}

/* This function is executed forces a given frequency independently of the policy */
state_t policy_force_global_frequency(ulong new_f)
{
    ulong *freq_set = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    polctx_t *c=&my_pol_ctx;
    ear_frequency = new_f;
    int i;

    if (masters_info.my_master_rank>=0)
    {
        c->app->def_freq = new_f;
        for (i = 0 ; i < lib_shared_region->num_processes; i++) {
            freq_set[i] = new_f;
        }
        if (ear_affinity_is_set){
            from_proc_to_core(freq_set, lib_shared_region->num_processes, &freq_per_core, arch_desc.top.cpu_count, NULL, NULL, ALL_PROCESSES);
        }else{
            for (i=0;i<MAX_CPUS_SUPPORTED;i++) {
                freq_per_core[i] = freq_set[0];
            }
        }
        *(c->ear_frequency) = frequency_set_with_list(0, freq_per_core);
        if (*(c->ear_frequency) == 0) {
            *(c->ear_frequency) = freq_set[0];
        }
        // eards_change_freq(ear_frequency);
    }
    return EAR_SUCCESS;
}

state_t policy_get_current_freq(node_freqs_t *freq_set)
{
    if (freq_set != NULL) node_freqs_copy(freq_set,&nf);
    return EAR_SUCCESS;
}


static state_t fill_mgt_api_pstate_data(uint api_id, pstate_t **pstate_lst, uint *pstate_cnt)
{
    debug("Requesting MGT_CPUFREQ (%u) vs. MGT_IMCFREQ (%u): %u",
            MGT_CPUFREQ, MGT_IMCFREQ, api_id);

    if (pstate_lst && pstate_cnt)
    {
        if (api_id != MGT_CPUFREQ && api_id != MGT_IMCFREQ) {
            return_msg(EAR_ERROR, Generr.api_incompatible);
        }

        // Get mgt API from metrics
        const metrics_t *mgt_api = metrics_get(api_id);

        if (mgt_api == NULL) {
            return_msg(EAR_ERROR, Generr.api_undefined);
        } else if (mgt_api->avail_list != NULL) {
            *pstate_lst = (pstate_t *) mgt_api->avail_list;
            *pstate_cnt = mgt_api->avail_count;

            return EAR_SUCCESS;
        }

        return_msg(EAR_ERROR, Generr.api_initialized);
    } else
    {
        return_msg(EAR_ERROR, Generr.input_null)
    }
}


static state_t from_proc_to_core(ulong *process_cpu_freqs_khz, int process_cnt, ulong **core_cpu_freqs_khz,
        int cpu_cnt, ulong *cpu_freq_max, ulong *cpu_freq_min, int proc_id)
{
    int init_rank, end_rank;
#if 0
    uint cpus_affected = 0;
#endif
    if (proc_id == ALL_PROCESSES){
      init_rank = 0;
      end_rank = process_cnt;
    }else{
      init_rank = proc_id;
      end_rank  = proc_id + 1;
    }
    if (proc_id >= 0){
      verbose(3, "from_proc_to_core  [%d] Freq %.2f", proc_id, (float)process_cpu_freqs_khz[proc_id]/(float)1000000.0);
    }
    if (proc_id == ALL_PROCESSES && (masters_info.my_master_rank < 0)){
        verbose(2, "Not master setting CPUfreq for ALL (%.2f)", (float)process_cpu_freqs_khz[0]/(float)1000000.0);
    }
    if (process_cpu_freqs_khz == NULL || process_cnt < 0 || core_cpu_freqs_khz == NULL
            || *core_cpu_freqs_khz == NULL || cpu_cnt < 0) {
        debug("ERROR: invalid arguments");
        return EAR_ERROR;
    }

    int verb_lvl = POLICY_INFO;

    static uint first_print = 1;
    if (first_print) {
        verb_lvl = 1;
        first_print = 0;
    }

    if (proc_id == ALL_PROCESSES){
      verbose_master(verb_lvl, "Mapping frequencies from %d processes to %d CPUs...", process_cnt, cpu_cnt);
    }

    memset(cpu_must_be_normalized, 0, MAX_CPUS_SUPPORTED * sizeof(uint));

    /* For AMD we compute the max per socket */
    if (arch_desc.top.vendor == VENDOR_AMD) {
        memset(max_freq_per_socket, 0, MAX_SOCKETS_SUPPORTED * sizeof(ulong));
    }
    /* This is a special use case, ignoring the mask. Using freq from proc 0 to all the CPUs */
		if (ignore_affinity_mask){
          // The frequency is filtered by the minimum permitted in the EAR configuration
          ulong cpu_freq_khz = ear_max(process_cpu_freqs_khz[0], (ulong) min_cpufreq);
          debug("Warning: Ignoring the affinity mask. Using %lu in all the cores ", cpu_freq_khz);
			    for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++) (*core_cpu_freqs_khz)[cpu_idx] = cpu_freq_khz;
          if (cpu_freq_max != NULL) *cpu_freq_max = cpu_freq_khz;
          if (cpu_freq_min != NULL) *cpu_freq_min = cpu_freq_khz;
			    return EAR_SUCCESS;
		}

    /* This is the default use case */

    for (int proc_lrank = init_rank; proc_lrank < end_rank; proc_lrank++)
    {
#if SHOW_DEBUGS
        if (process_cpu_freqs_khz[proc_lrank] < (ulong) min_cpufreq) {
            debug("The selected frequency (%lu) for process %d is lower than the minimum"
                    " permitted (%lu). Filtering...", process_cpu_freqs_khz[proc_lrank], proc_lrank, (ulong) min_cpufreq);
        }
#endif // SHOW_DEBUGS

        // The frequency is filtered by the minimum permitted in the EAR configuration
        ulong cpu_freq_khz = ear_max(process_cpu_freqs_khz[proc_lrank], (ulong) min_cpufreq);
		process_cpu_freqs_khz[proc_lrank] = cpu_freq_khz;

        /* Compute the maximum and minimum if required */
        if (cpu_freq_max != NULL) {
            *cpu_freq_max = ear_max(cpu_freq_khz, *cpu_freq_max);
        }
        if (cpu_freq_min != NULL) {
            *cpu_freq_min = ear_min(cpu_freq_khz, *cpu_freq_min);
        }

        uint verb = (cpu_freq_khz != init_def_cpufreq_khz); // Verbose only when the frequency to set is
                                                            // different from the initial
        if (verb && (proc_id == ALL_PROCESSES)) {
            verbosen_master(verb_lvl, "Process %d: %lu kHz", proc_lrank, cpu_freq_khz);
            verbosen_master(verb_lvl + 1, " -> CPUs:")
        }


        /* This is the default use case. EAR uses the CPU affinity mask */
        int sem_ret_val = sem_wait(lib_shared_lock_sem);
        if (sem_ret_val < 0)
        {
            verbose(2, "%sWARNING%s Locking semaphor for cpu mask read failed: %d", COL_YLW, COL_CLR, errno);
        }

        cpu_set_t process_mask = sig_shared_region[proc_lrank].cpu_mask;

        // We only want to increase the semaphor number if the previous lock was successful.
        if (!sem_ret_val)
        {
            if (sem_post(lib_shared_lock_sem) < 0)
            {
                verbose(2, "%sWARNING%s Unlocking semaphor for cpu mask read failed: %d",
                        COL_YLW, COL_CLR, errno);
            }
        }

        for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++) {
            if (CPU_ISSET(cpu_idx, &process_mask)) {
                if (verb) {
                    verbosen_master(verb_lvl + 1, " %d", cpu_idx);
                }
                (*core_cpu_freqs_khz)[cpu_idx] = cpu_freq_khz;

                max_freq_per_socket[arch_desc.top.cpus[cpu_idx].socket_id] =
                    ear_max(max_freq_per_socket[arch_desc.top.cpus[cpu_idx].socket_id],
                            cpu_freq_khz);

                cpu_must_be_normalized[cpu_idx] = 1;
#if 0
                cpus_affected++;
#endif
            }
        }
        if (verb) {
            verbosen_master(verb_lvl,"\n");
        }
    }
    if (arch_desc.top.vendor == VENDOR_AMD) {

        for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++) {

          // The maximum CPU freq. for the socket which the CPU belongs to.
          ulong max_cpufreq = max_freq_per_socket[arch_desc.top.cpus[cpu_idx].socket_id];

          if (cpu_must_be_normalized[cpu_idx] &&
              (*core_cpu_freqs_khz)[cpu_idx] != max_cpufreq) {

            (*core_cpu_freqs_khz)[cpu_idx] = max_cpufreq;

            verbose_master(verb_lvl,
                "Increasing the CPU %d freq. to %lu because it's on AMD node at socket %d.",
                cpu_idx, max_cpufreq, arch_desc.top.cpus[cpu_idx].socket_id);
          }
        }
    }
#if 0
    if (proc_id >= 0){
        verbose(2,"[%d] Changing CPU freq for %d cpus", proc_id, cpus_affected);
    }
#endif

    return EAR_SUCCESS;
}

static uint must_set_policy_cpufreq(pc_app_info_t *pc_data, settings_conf_t *settings_conf)
{
    debug("must set policy cpufreq (first policy try %u)", first_policy_try);
    if (first_policy_try) {
        debug("Letting the first try to the power policy cpufreq");
        first_policy_try = 0;
        return 1;
    }

    if (pc_data == NULL)
        return 1;
    if ((pc_data->cpu_mode == PC_DVFS)
            && (settings_conf->pc_opt.current_pc <= POWER_CAP_UNLIMITED))
        return 1;
    if (pc_data->cpu_mode == PC_POWER)
        return 1;

    return 0;
}

static ulong compute_avg_freq(ulong *freq_list, int n)
{
    ulong total = 0;

    for (int i = 0; i < n; i++)
    {
        total += freq_list[i];
    }

    return total / n;
}

static void verbose_freq_per_core(int verb_lvl, ulong **core_cpu_freqs_khz, int cpu_cnt)
{
    if (verb_level >= verb_lvl) {

        verbose_master(verb_lvl, "Selected frequency per Processor/CPU:");

        for (int i = 0; i < cpu_cnt; i++)
        {
            if ((*core_cpu_freqs_khz)[i] > 0) {
                verbosen_master(verb_lvl, "CPU[%d]=%.3f ", i, (float) (*core_cpu_freqs_khz)[i] / GHZ_TO_KHZ);
            } else {
                verbosen_master(verb_lvl, "CPU[%d]= --", i);
            }
        }
        verbose_master(verb_lvl, " ");
    }
}

static state_t policy_cpu_freq_selection(polctx_t *c, signature_t *my_sig,
        node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id)
{
    ulong *freq_set = freqs->cpu_freq;

    int i;
    char pol_grain_str[16];

    verbose_master(POLICY_INFO + 1, "%sPOLICY%s CPU freq. selection.", COL_BLU, COL_CLR);

#if DEBUG_CPUFREQ_COST
    timestamp init_cpufreq_cost, end_cpufreq_cost;
    ulong elapsed_cpufreq_cost_us;

    timestamp_getfast(&init_cpufreq_cost);
#endif

    if (dom->cpu == POL_GRAIN_CORE) sprintf(pol_grain_str, "GRAIN_CORE");
    else                            sprintf(pol_grain_str, "GRAIN_NODE");

    verbose_master(3, "%-12s: %s\n%-12s: %d", "Policy level",
            pol_grain_str, "Affinity", ear_affinity_is_set);

    ulong max_cpufreq_sel_khz = 0, min_cpufreq_sel_khz = 1000000000;
    memset(freq_per_core, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);

    // Assumption: If affinity is set for master, it
    // is set for all, we could check individually
    if ((dom->cpu == POL_GRAIN_CORE) && (ear_affinity_is_set)) {
        from_proc_to_core(freq_set, lib_shared_region->num_processes, &freq_per_core,
                arch_desc.top.cpu_count, &max_cpufreq_sel_khz, &min_cpufreq_sel_khz, process_id);
    } else {
        if (process_id == ALL_PROCESSES) {
            debug("Setting same freq in all node %lu", freq_set[0]);

            for (i = 1; i < MAX_CPUS_SUPPORTED; i++) {
                freq_set[i] = freq_set[0];
            }

            if (ear_affinity_is_set) {
                from_proc_to_core(freq_set, lib_shared_region->num_processes, &freq_per_core,
                        arch_desc.top.cpu_count, &max_cpufreq_sel_khz, &min_cpufreq_sel_khz, process_id);
            } else {
                for (i = 0; i < MAX_CPUS_SUPPORTED; i++) {
                    freq_per_core[i] = freq_set[0];
                }
            }
        }
    }

    if  (process_id == ALL_PROCESSES) {
        fill_cpus_out_of_cgroup(exclusive_mode);
    }

    verbose_freq_per_core(POLICY_INFO , &freq_per_core, arch_desc.top.cpu_count);

    // We decided arbitrarely to put show the minimum CPU freq. selected.
    *(c->ear_frequency) = max_cpufreq_sel_khz;

#if SHOW_DEBUGS
    debug("pc_app_info_data %p", pc_app_info_data);
    if (pc_app_info_data != NULL) {
        debug("pc_app_info_data->cpu_mode (%d) == PC_DVFS ? %d",pc_app_info_data->cpu_mode, pc_app_info_data->cpu_mode == PC_DVFS);
    }
    debug("system_conf->pc_opt.current_pc (%d) <= POWER_CAP_UNLIMITED (%d) ? %d", system_conf->pc_opt.current_pc, POWER_CAP_UNLIMITED, system_conf->pc_opt.current_pc <= POWER_CAP_UNLIMITED);
#endif

	// POWER_CAP_UNLIMITED is equal to 1, so the CPU freq is set if powercap is 0 (disabled) or unlimited (1)
	  
    if (must_set_policy_cpufreq(pc_app_info_data, system_conf)) {
       if ((masters_info.my_master_rank >=0) && (process_id == ALL_PROCESSES) ){
        verbose(POLICY_INFO, "%sPOLICY%s Setting a CPU frequency (kHz) range of "
                "[%lu, %lu] GHz...", COL_BLU, COL_CLR, min_cpufreq_sel_khz, max_cpufreq_sel_khz);
       }
			 uint do_change = 1;
			 #if 0
			 if ((process_id != ALL_PROCESSES) && !node_freqs_are_diff(DOM_CPU, freqs, &last_nf)){
					//verbose(0,"not changing freq because they are the same");
					do_change = 0;
				}
       #endif
        

        if (do_change){
          frequency_set_with_list(0, freq_per_core); // Setting the selected CPU frequency
        }
				#if 0
				if ((process_id != ALL_PROCESSES) && do_change) node_freqs_copy(&last_nf, freqs);
				#endif

    } else {
        verbose_master(POLICY_INFO, "%sPOLICY%s Requesting a CPU frequency (kHz) range of "
                "[%lu, %lu] GHz...", COL_BLU, COL_CLR, min_cpufreq_sel_khz, max_cpufreq_sel_khz);
    }
    if (process_id == ALL_PROCESSES){
      if ((pc_app_info_data != NULL) && (pc_app_info_data->cpu_mode == PC_DVFS)) {
        /* Warning: POL_GRAIN_CORE not supported */
        pcapp_info_set_req_f(pc_app_info_data, freq_per_core, MAX_CPUS_SUPPORTED);
      }

      int freq_cnt = (dom->cpu == POL_GRAIN_CORE) ? lib_shared_region->num_processes : 1;
      if (avg_freq != NULL){
				avg_freq->cpu_freq[0] = compute_avg_freq(freq_set, freq_cnt);
			}

#if DEBUG_CPUFREQ_COST
      /* For debugging purposes */
      timestamp_getfast(&end_cpufreq_cost);
      elapsed_cpufreq_cost_us = timestamp_diff(&end_cpufreq_cost, &init_cpufreq_cost, TIME_USECS);
      total_cpufreq_cost_us += elapsed_cpufreq_cost_us;
      calls_cpufreq++;
#endif
    }

    return EAR_SUCCESS;
}

static state_t policy_mem_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    if ((freqs == NULL) || (freqs->imc_freq == NULL)) {
        verbose_master(POLICY_INFO, "%sERROR%s policy_mem_freq_selection NULL", COL_RED, COL_CLR);
        return EAR_SUCCESS;
    }
    verbose_master(POLICY_INFO, "(Lower/Upper) requested IMC pstate (%lu/%lu)",
            freqs->imc_freq[IMC_MAX],freqs->imc_freq[IMC_MIN]);

    uint *imc_up, *imc_low;

    if (dyn_unc && metrics_get(MGT_IMCFREQ)->ok) {
        imc_up = calloc(imc_devices, sizeof(uint));
        imc_low = calloc(imc_devices, sizeof(uint));

        /* We are using the same imc pstates for all the devices for now */
#if SHOW_DEBUGS
            // Below code assumes the same IMC freq. was selected for all devices!
            if (max_policy_imcfreq_ps < freqs->imc_freq[0 * IMC_VAL + IMC_MAX]) {
                debug("The selected upper IMC P-State (%lu) is greather than the maximum"
                        " permitted (%u). Fitering...",
                        freqs->imc_freq[0 * IMC_VAL + IMC_MAX], max_policy_imcfreq_ps);
            }
#endif
        for (int sid = 0; sid < imc_devices; sid++) {
            imc_low[sid] = ear_min((uint) freqs->imc_freq[sid * IMC_VAL + IMC_MAX], max_policy_imcfreq_ps);
            imc_up[sid]  = freqs->imc_freq[sid * IMC_VAL + IMC_MIN];	

            // Check whether selected IMC upper bound is correct
            if (imc_up[sid] < imc_min_pstate[sid]) { 
                verbose_master(POLICY_INFO, "%sERROR%s IMC upper bound is greather"
                        " than max %u", COL_RED, COL_CLR, imc_up[sid]);
                return EAR_SUCCESS;
            }

            // Check whether selected IMC lower bound is correct
            if (imc_low[sid] > imc_max_pstate[sid]) {
                verbose_master(POLICY_INFO, "%sERROR%s IMC lower bound is lower"
                        " than minimum %u", COL_RED, COL_CLR, imc_low[sid]);
                return EAR_SUCCESS;
            }

            if (imc_up[sid] != ps_nothing) {
                verbose_master(POLICY_INFO, "Setting IMC freq (Upper/Lower) [SID=%d] to (%llu/%llu)",
                        sid, imc_pstates[imc_low[sid]].khz, imc_pstates[imc_up[sid]].khz);
            } else {
                verbose_master(POLICY_INFO, "Setting IMC freq (Upper) [SID=%d] (%llu) and min",
                        sid, imc_pstates[imc_low[sid]].khz);
            }
        }

        if (state_fail(mgt_imcfreq_set_current_ranged_list(NULL, imc_low, imc_up)))
        {
            uint sid = 0;
            if (imc_up[sid] != ps_nothing)
            {
                verbose_master(2, "Error setting IMC freq to %llu and %llu",
                               imc_pstates[imc_low[0]].khz, imc_pstates[imc_up[0]].khz);
            } else
            {
                verbose_master(2, "Error setting IMC freq to %llu and min",
                               imc_pstates[imc_low[0]].khz);
            }
        }

        free(imc_up);
        free(imc_low);
    } else {
        verbose_master(2, "IMC not compatible");
    }

    return EAR_SUCCESS;
}


static state_t policy_gpu_mem_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    return EAR_SUCCESS;
}


//state_t policy_gpu_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs, int *ready)
static state_t policy_gpu_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    ulong *gpu_f = freqs->gpu_freq;
    int i;
    if (c->num_gpus == 0) return EAR_SUCCESS;

#if USE_GPUS
    if (verb_level >= POLICY_GPU) {
        for (i=0; i < c->num_gpus; i++) {
            verbosen_master(POLICY_GPU," GPU freq[%d] = %.3f ",
                    i, (float) gpu_f[i] / GHZ_TO_KHZ);
        }
        verbose_master(POLICY_GPU," ");
    }

    // TODO: Powercap limits are checked here but this code could be moved to a function.
    pc_app_info_data->num_gpus_used = my_sig->gpu_sig.num_gpus;
    memcpy(pc_app_info_data->req_gpu_f, gpu_f, c->num_gpus * sizeof(ulong));

    // TODO We must filter the GPUs I'm using.
   if (state_fail(mgt_gpu_freq_limit_set(no_ctx, gpu_f))) {
        verbose_master(POLICY_GPU, "%sERROR%s setting GPU frequency. %d: %s",
                COL_RED, COL_CLR, intern_error_num, intern_error_str);
    }
		if (avg_freq != NULL){
    	avg_freq->gpu_freq[0] = compute_avg_freq(gpu_f, my_sig->gpu_sig.num_gpus);
		}
#endif
    return EAR_SUCCESS;
}


static state_t policy_master_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id)
{
  if (masters_info.my_master_rank < 0) return EAR_SUCCESS;
  return policy_freq_selection(c, my_sig, freqs, dom, avg_freq, process_id);
}


static state_t policy_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id)
{
#if SINGLE_CONNECTION
    if (masters_info.my_master_rank < 0 ) {
        return EAR_SUCCESS;
    }
#endif

    if (dom->cpu != POL_NOT_SUPPORTED) {
        policy_cpu_freq_selection(c, my_sig, freqs, dom, avg_freq, process_id);
    }
    if (process_id == ALL_PROCESSES){
        if (dom->mem != POL_NOT_SUPPORTED) {
            policy_mem_freq_selection(c, my_sig, freqs, dom, avg_freq);
        }
        if (dom->gpu != POL_NOT_SUPPORTED) {
            policy_gpu_freq_selection(c, my_sig, freqs, dom, avg_freq);
        }
        if (dom->gpumem != POL_NOT_SUPPORTED) {
            policy_gpu_mem_freq_selection(c, my_sig, freqs, dom, avg_freq);
        }
    }

    return EAR_SUCCESS;
}


static void verbose_policy_ctx(int verb_lvl, polctx_t *policy_ctx)
{
    // TODO: Improve the function body to minimize the number of I/O operations.
    if (verb_level >= verb_lvl) {
        verbose_master(verb_lvl, "\n%*s%s%*s", 1, "", "--- EAR policy context ---", 1, "");

        verbose_master(verb_lvl, "%-18s: %u",  "user_type",          policy_ctx->app->user_type);
        verbose_master(verb_lvl, "%-18s: %s",  "policy_name",        policy_ctx->app->policy_name);
        verbose_master(verb_lvl, "%-18s: %lf", "policy th",          policy_ctx->app->settings[0]);
        verbose_master(verb_lvl, "%-18s: %lu", "def_freq",           DEF_FREQ(policy_ctx->app->def_freq));
        verbose_master(verb_lvl, "%-18s: %u",  "def_pstate",         policy_ctx->app->def_p_state);
        verbose_master(verb_lvl, "%-18s: %d",  "reconfigure",        policy_ctx->reconfigure->force_rescheduling);
        verbose_master(verb_lvl, "%-18s: %lu", "user_selected_freq", policy_ctx->user_selected_freq);
        verbose_master(verb_lvl, "%-18s: %lu", "reset_freq_opt",     policy_ctx->reset_freq_opt);
        verbose_master(verb_lvl, "%-18s: %lu", "ear_frequency",      *(policy_ctx->ear_frequency));
        verbose_master(verb_lvl, "%-18s: %u",  "num_pstates",        policy_ctx->num_pstates);
        verbose_master(verb_lvl, "%-18s: %u",  "use_turbo",          policy_ctx->use_turbo);
        verbose_master(verb_lvl, "%-18s: %u",  "num_gpus",           policy_ctx->num_gpus);

        verbose_master(verb_lvl, "%*s%s%*s\n", 1, "", "--------------------------", 1, "");
    }
}


static void verbose_time_in_phases(signature_t *sig)
{
  sig_ext_t *se = (sig_ext_t *)sig->sig_ext;
  if (se != NULL){
    verbose_master(2, "............Phases");
    for (uint ph = 0; ph < EARL_MAX_PHASES; ph++){
      if (se->earl_phase[ph].elapsed){ 
        verbose_master(2, "Phase[%s] elapsed %llu", phase_to_str(ph), se->earl_phase[ph].elapsed);
      }
    }
    verbose_master(2, "............END-Phases");
  }
}


static void policy_setup_priority()
{
    uint  user_cpuprio_list_count;       // Count of user-prvided priority list
    uint *user_cpuprio_list;             // User-provided list of priorities.

    if ((user_cpuprio_list = (uint *) envtoat(FLAG_PRIO_CPUS, NULL, &user_cpuprio_list_count, ID_UINT)) != NULL)
    {
        // The user provided a CPU priority list per CPU involved in the job

        use_cpuprio = 1;

        verbose_cpuprio_list(POLICY_INFO - 1, user_cpuprio_list, user_cpuprio_list_count);

        // Set user-provided list of priorities
        if (state_fail(policy_setup_priority_per_cpu(user_cpuprio_list,
                                                     user_cpuprio_list_count)))
        {
            use_cpuprio = 0;

            verbose(POLICY_INFO, "%sERROR%s Setting up priority list: %s",
                    COL_RED, COL_CLR, state_msg);
        }
    } else if ((user_cpuprio_list = (uint *) envtoat(FLAG_PRIO_TASKS, NULL, &user_cpuprio_list_count, ID_UINT)) != NULL)
    {
        // The user provided a priority list for each task involved in the job.

        use_cpuprio = 1;

        verbose_cpuprio_list(POLICY_INFO - 1, user_cpuprio_list, user_cpuprio_list_count);

        if (state_fail(policy_setup_priority_per_task(user_cpuprio_list,
                                                      user_cpuprio_list_count)))
        {

            use_cpuprio = 0;

            verbose(POLICY_INFO, "%sERROR%s Setting up priority list: %s",
                    COL_RED, COL_CLR, state_msg);
        }
    } else {
        // If the user didn't set these flags, it is not needed get cpuprio enabled.
        use_cpuprio = 0;
    }
}


static state_t policy_setup_priority_per_cpu(uint *setup_prio_list, uint setup_prio_list_count)
{
    if (setup_prio_list && setup_prio_list_count)
    {
        const metrics_t *cpuprio_api = metrics_get(API_ISST);
        if (cpuprio_api)
        {
            state_t ret = set_priority_list((uint *) cpuprio_api->current_list,
                                            cpuprio_api->devs_count,
                                            (const uint *) setup_prio_list,
                                            setup_prio_list_count,
                                            (const cpu_set_t*) &lib_shared_region->node_mask);

            if (state_ok(ret))
            {
                if (VERB_ON(POLICY_INFO))
                {
                    verbose_master(POLICY_INFO, "Setting user-provided CPU priorities...");
                    mgt_cpufreq_prio_data_print((cpuprio_t *) cpuprio_api->avail_list,
                            (uint *) cpuprio_api->current_list, verb_channel);
                }

                if (!mgt_cpufreq_prio_is_enabled())
                {
                    // We enable priority system in the case it is not yet created.
                    if (state_fail(mgt_cpufreq_prio_enable()))
                    {
                        return_msg(EAR_ERROR, Generr.api_uninitialized);
                    }
                }

                if (state_fail(mgt_cpufreq_prio_set_current_list((uint *) cpuprio_api->current_list)))
                {
                    return_msg(EAR_ERROR, "Setting CPU PRIO list.");
                }

            } else {
                char err_msg[64];
                snprintf(err_msg, sizeof(err_msg), "Setting priority list (%s)", state_msg);
                return_msg(EAR_ERROR, err_msg);
            }
        } else {
            // If we don't have an API, we won't set any priority configuration.
            return_msg(EAR_ERROR, Generr.api_undefined);
        }
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static state_t policy_setup_priority_per_task(uint *setup_prio_list, uint setup_prio_list_count)
{
    if (setup_prio_list && setup_prio_list_count)
    {
        const metrics_t *cpuprio_api = metrics_get(API_ISST);
        if (cpuprio_api)
        {
            for (int i = 0; i < lib_shared_region->num_processes; i++)
            {
                const cpu_set_t *mask = (const cpu_set_t *) &sig_shared_region[i].cpu_mask;

                int prio_list_idx = sig_shared_region[i].mpi_info.rank;
                if (prio_list_idx < setup_prio_list_count)
                {
                    uint prio = setup_prio_list[prio_list_idx];

                    if (state_fail(policy_set_prio_with_mask((uint *) cpuprio_api->current_list, cpuprio_api->devs_count, prio, mask)))
                    {
                        char err_msg[64];
                        snprintf(err_msg, sizeof(err_msg), "Setting priority list for task %d (%s)", prio_list_idx, state_msg);
                        return_msg(EAR_ERROR, err_msg);
                    }
                } else {
                    // Set PRIO_SAME on CPUs of that rank not indexed by the priority list.
                    if (state_fail(policy_set_prio_with_mask((uint *) cpuprio_api->current_list, cpuprio_api->devs_count, PRIO_SAME, mask)))
                    {
                        char err_msg[64];
                        snprintf(err_msg, sizeof(err_msg), "Setting priority list for task %d (%s)", prio_list_idx, state_msg);
                        return_msg(EAR_ERROR, err_msg);
                    }
                }
            }

            // Set to PRIO_SAME all CPUs not in node mask
            policy_set_prio_with_mask_inverted((uint *) cpuprio_api->current_list, cpuprio_api->devs_count, PRIO_SAME, (const cpu_set_t *) &lib_shared_region->node_mask);

            if (VERB_ON(POLICY_INFO - 1))
            {
                verbose_master(0, "%sPOLICY%s Setting CPU priorities...", COL_BLU, COL_CLR);
                mgt_cpufreq_prio_data_print((cpuprio_t *) cpuprio_api->avail_list,
                                            (uint *) cpuprio_api->current_list, verb_channel);
            }

            if (!mgt_cpufreq_prio_is_enabled())
            {
                // We enable priority system in the case it is not yet enabled.
                if (state_fail(mgt_cpufreq_prio_enable()))
                {
                    return_msg(EAR_ERROR, Generr.api_uninitialized);
                }
            }

            if (state_fail(mgt_cpufreq_prio_set_current_list((uint *) cpuprio_api->current_list)))
            {
                return_msg(EAR_ERROR, "Setting CPU PRIO list.");
            }
        } else {
            // If we don't have an API, we won't set any priority configuration.
            return_msg(EAR_ERROR, Generr.api_undefined);
        }
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static state_t set_priority_list(uint *dst_prio_list, uint dst_prio_list_count, const uint *src_prio_list,
                                 uint src_prio_list_count, const cpu_set_t *mask)
{
    if (dst_prio_list && src_prio_list && mask) {

        int prio_list_idx = 0; // i-th src_prio_list value
        int dev_idx = 0;       // i-th dst_prio_list value

        // We iterate at most min(src, dst)
        for (dev_idx = 0; dev_idx < dst_prio_list_count && prio_list_idx < src_prio_list_count; dev_idx++)
        {
            dst_prio_list[dev_idx] = CPU_ISSET(dev_idx, mask) ? src_prio_list[prio_list_idx++] : PRIO_SAME;
        }

        // If there are remaining indexes of dst not set, we set the idle priority
        while (dev_idx < dst_prio_list_count)
        {
            dst_prio_list[dev_idx++] = PRIO_SAME;
        }

    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static state_t policy_set_prio_with_mask(uint *dst_prio_list, uint dst_prio_list_count, uint priority, const cpu_set_t *mask)
{
    if (dst_prio_list && mask)
    {
        for (int i = 0; i < dst_prio_list_count; i++)
        {
            if (CPU_ISSET(i, mask))
            {
                dst_prio_list[i] = priority;
            }
        }
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static state_t policy_set_prio_with_mask_inverted(uint *dst_prio_list, uint dst_prio_list_count, uint priority, const cpu_set_t *mask)
{
    if (dst_prio_list && mask)
    {
        for (int i = 0; i < dst_prio_list_count; i++)
        {
            if (!CPU_ISSET(i, mask))
            {
                dst_prio_list[i] = priority;
            }
        }
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static state_t policy_reset_priority()
{
    const metrics_t *cpuprio_api = metrics_get(API_ISST);
    if (is_master && cpuprio_api)
    {
        if (cpuprio_api->ok && use_cpuprio && mgt_cpufreq_prio_is_enabled())
        {
            verbose_master(POLICY_INFO - 1, "Resetting priority list...");
            if (state_ok(reset_priority_list_with_mask((uint *) cpuprio_api->current_list,
                         cpuprio_api->devs_count, &lib_shared_region->node_mask)))
            {
                if (VERB_ON(POLICY_INFO))
                {
                    char buffer[SZ_BUFFER_EXTRA];
                    mgt_cpuprio_data_tostr((cpuprio_t *) cpuprio_api->avail_list,
                                           (uint *) cpuprio_api->current_list, buffer, SZ_BUFFER_EXTRA);

                    verbose_master(POLICY_INFO, "CPUPRIO list of priorities \n %s", buffer);
                }
                if (state_fail(mgt_cpufreq_prio_set_current_list((uint *) cpuprio_api->current_list)))
                {
                    verbose_master(POLICY_INFO, "%sERROR%s Setting CPU PRIO list.", COL_RED, COL_CLR);
                }
            } else {
                verbose_master(POLICY_INFO, "%sERROR%s Resetting priority list: %s",
                               COL_RED, COL_CLR, state_msg);
            }
        }
    }
    return EAR_SUCCESS;
}


static state_t reset_priority_list_with_mask(uint *prio_list, uint prio_list_count, const cpu_set_t *mask)
{
    if (prio_list && mask)
    {
        if (prio_list_count) {
            for (int i = 0; i < prio_list_count; i++)
            {
                prio_list[i] = CPU_ISSET(i, mask) ? 0 : PRIO_SAME;
            }
        } else {
            return_msg(EAR_ERROR, Generr.arg_outbounds);
        }
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static void verbose_cpuprio_list(int verb_lvl, uint *cpuprio_list, uint cpuprio_list_count)
{
    if (VERB_ON(verb_lvl))
    {
        verbosen_master(0, "CPUPRIO list read:");

        if (cpuprio_list)
        {
            int i;
            for (i = 0; i < cpuprio_list_count - 1; i++)
            {
                verbosen_master(0, " %u", cpuprio_list[i]);
            }

            verbose_master(0, " %u", cpuprio_list[i]);
        }
    }
}


static state_t policy_set_cpus_governor(uint exclusive)
{
#define free_return_err(msg) \
        free(governor_list_before_init);\
        governor_list_before_init = NULL;\
        return_msg(EAR_ERROR, msg);

#define check(st, msg) \
    if (state_fail(st)) {free_return_err(msg);}

    // Allocate the governor list
    if (state_fail(mgt_cpufreq_data_alloc(NULL, &governor_list_before_init)))
    {
        return_msg(EAR_ERROR, "Allocating governor list.");
    }

    // Retrieve the current governor list
    check(mgt_cpufreq_governor_get_list(no_ctx, governor_list_before_init),
          "Retrieving the governor list.");

    // Verbose governor list
    if (VERB_ON(POLICY_INFO))
    {
        verbosen(POLICY_INFO , "List of governors of %u CPUs: ", lib_shared_region->num_cpus);
        for (int i = 0; i < metrics_get(MGT_CPUFREQ)->devs_count; i++)
        {
            verbosen(POLICY_INFO , " %u", governor_list_before_init[i]);;
        }
        verbose(POLICY_INFO , " ");
    }

    // Set userspace governor on CPUs not used by the job

    cpu_set_t cpus_to_be_set;
    if (exclusive){
      set_mask_all_ones(&cpus_to_be_set);
    }else{
      memcpy(&cpus_to_be_set, &lib_shared_region->node_mask, sizeof(cpu_set_t));
    }

    verbosen(POLICY_INFO , "Node used CPUs: ");
    verbose_affinity_mask(POLICY_INFO , (const cpu_set_t *) &lib_shared_region->node_mask, MAX_CPUS_SUPPORTED);

    verbosen(POLICY_INFO , "CPUs to be set: ");
    verbose_affinity_mask(POLICY_INFO , (const cpu_set_t *) &cpus_to_be_set, MAX_CPUS_SUPPORTED);

    // Getting a valid ID for 'userspace' governor
    char *governor_str = "userspace";
    uint governor_int;
    check(governor_toint(governor_str, &governor_int),
          "Converting governor from str to int.");

    // Setting the governor
    #if VERSION_MAJOR >= 4 && VERSION_MINOR >= 3
		//#warning mgt_cpufreq_governor_set_mask compiled
    check(mgt_cpufreq_governor_set_mask(no_ctx, governor_int, cpus_to_be_set),
          "Setting governor.");
		#endif
    return EAR_SUCCESS;
}


static state_t policy_restore_cpus_governor()
{
    if (governor_list_before_init)
    {
        mgt_cpufreq_governor_set_list(no_ctx, governor_list_before_init);

        free(governor_list_before_init);
        governor_list_before_init = NULL;
    }

    return EAR_SUCCESS;
}
