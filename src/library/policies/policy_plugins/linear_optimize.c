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



#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
// #define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/hardware/topology.h>
#include <common/output/verbose.h>
#include <common/environment.h>

#include <management/cpufreq/frequency.h>

#include <daemon/local_api/eard_api.h>
#include <daemon/powercap/powercap_status.h>

#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/api/clasify.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/generic.h>
#if MPI_OPTIMIZED
#include <common/system/monitor.h>
#endif

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define FREQ_DEF(f) (!ext_def_freq?f:ext_def_freq)
#else
#define FREQ_DEF(f) f
#endif


extern uint cpu_ready, gpu_ready;
// extern gpu_state_t curr_gpu_state;
extern uint last_earl_phase_classification;
extern ear_classify_t phases_limits;

static int min_pstate;

/*  Frequency management */
static node_freqs_t optimize_def_freqs;
static node_freqs_t last_nodefreq_sel;

/* Configuration */
typedef enum {
  CPI      = 0,
  GFLOPS   = 1,
  GFLOPS_W = 2,
  GBS_W    = 3,
  GPU      = 4,
} opt_metric_t;

typedef enum{
  max_value = 0,
  min_value = 1,
  ratio_value = 2,
}opt_type_t;
static opt_metric_t metric_to_optimize = CPI;
static opt_type_t   metric_strategy    = min_value;
static char optimize_criteria[1024];

extern ulong *cpufreq_diff;


static uint num_processes;
static ulong nominal_node;

extern uint try_turbo_enabled;
extern uint policy_cpu_bound;
extern uint policy_mem_bound;
extern uint ear_mpi_opt;

#define FIRST     0
#define SEARCHING 1

static uint optimize_state = FIRST;
static uint optimize_num_pstates;
static uint optimize_num_gpus = 0;


/*  Policy/Other */
static int optimize_readiness =  !EAR_POLICY_READY;
static signature_t *my_app;


// No energy models extra vars
static signature_t *sig_list;
static uint *sig_ready;

static polsym_t gpus;
static const ulong **gpuf_pol_list;
static const uint *gpuf_pol_list_items;



static topology_t me_top;
static state_t policy_no_models_go_next_optimize(int curr_pstate,int *ready,node_freqs_t *freqs,unsigned long num_pstates);
static uint policy_no_models_is_better_optimize(signature_t * curr_sig, signature_t *prev_sig);
static void policy_no_models_stop_searching(int curr_pstate, int *ready,node_freqs_t *freqs);

static void policy_end_summary(int verb_lvl);
static state_t policy_mpi_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);
static state_t policy_mpi_end_optimize(node_freqs_t *freqs, int *process_id);

#define FLAG_METRIC_TO_OPTIMIZE   "EAR_METRIC_TO_OPTIMIZE"
#define FLAG_STRATEGY_TO_OPTIMIZE "EAR_STRATEGY_TO_OPTIMIZE"

#define metric_is(x)    (x == CPI?"CPI":(x == GFLOPS?"GFLOPS":(x == GFLOPS_W?"GFLOPS_W":(x == GBS_W?"GBS_W":"GPU"))))
#define strategy_is(x)  (x == max_value?"MAX":(x == min_value?"MIN":"RATIO"))

static void update_optimize_options(char *metric,char * strategy)
{
  if (metric != NULL){
    if (strcmp(metric, "CPI") == 0)       metric_to_optimize = CPI;
    if (strcmp(metric, "GFLOPS") == 0)    metric_to_optimize = GFLOPS;
    if (strcmp(metric, "GFLOPS_W") == 0)  metric_to_optimize = GFLOPS_W;
    if (strcmp(metric, "GBS_W") == 0)     metric_to_optimize = GBS_W;
    if (strcmp(metric, "GPU") == 0)       metric_to_optimize = GPU;
  }
  if (strategy != NULL){
    if (strcmp(strategy, "MAX") == 0)       metric_strategy = max_value;
    if (strcmp(strategy, "MIN") == 0)       metric_strategy = min_value;
    if (strcmp(strategy, "RATIO") == 0)     metric_strategy = ratio_value;
  }
  sprintf(optimize_criteria,"Metric=%s Strategy=%s", metric_is(metric_to_optimize), strategy_is(metric_strategy));
}

static void int_reset_data()
{
  verbose_master(2,"%sOptimize reset data%s",COL_RED,COL_CLR);
  optimize_state = FIRST;
  memset(sig_ready, 0 , sizeof(uint)* optimize_num_pstates);
    
}

/* This policy doesn't use models at all, optimize one specific CPU metric */

state_t policy_init(polctx_t *c)
{
    if (c == NULL)
    {
        return EAR_ERROR;
    }

    char *cmetric_to_optimize   = ear_getenv(FLAG_METRIC_TO_OPTIMIZE);
    char *cstrategy_to_optimize = ear_getenv(FLAG_STRATEGY_TO_OPTIMIZE);

    /* This function checks the metric and strategy (max, min) */
    update_optimize_options(cmetric_to_optimize, cstrategy_to_optimize);

    int i = 0;

    topology_init(&me_top);

    debug("optimize init");

#if MPI_OPTIMIZED
    //if (is_mpi_enabled())
    if (module_mpi_is_enabled())
    {
        uint block_type;
        mpi_app_init(c);
        is_blocking_busy_waiting(&block_type);
        verbose_master(2, "Busy waiting MPI: %u", block_type == BUSY_WAITING_BLOCK);
    }
#endif

    nominal_node = frequency_get_nominal_freq();
    verbose_master(3, "Nominal CPU freq. is %lu kHz", nominal_node);

    /* Local to the policy */
    my_app          = calloc(1, sizeof(signature_t));
    my_app->sig_ext =  (void *) calloc(1, sizeof(sig_ext_t));

    /* We store the signatures */
    sig_list  = (signature_t *) calloc(c->num_pstates,sizeof(signature_t));
    sig_ready = (uint *) calloc(c->num_pstates,sizeof(uint));
    optimize_num_pstates = c->num_pstates;

    if ((sig_list == NULL) || (sig_ready == NULL)) {
            return EAR_ERROR;
    }

    num_processes = lib_shared_region->num_processes;

    node_freqs_alloc(&optimize_def_freqs);
    node_freqs_alloc(&last_nodefreq_sel);



    /* Configuring default freqs */
    for (i=0; i < num_processes; i++) {
        optimize_def_freqs.cpu_freq[i] = frequency_pstate_to_freq(0);
    }


#if USE_GPUS
    memset(&gpus, 0, sizeof(gpus));
    optimize_num_gpus    = c->num_gpus;
    if (optimize_num_gpus) {
        if (policy_gpu_load(c->app, &gpus) != EAR_SUCCESS){
            verbose_master(2, "Error loading GPU policy");
        }
        verbose_master(2, "Initialzing GPU policy part");
        if (gpus.init != NULL) gpus.init(c);

        gpuf_pol_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
        gpuf_pol_list_items = (const uint *)   metrics_gpus_get(MGT_GPU)->avail_count;

        /* replace by default settings in GPU policy */
        for (i=0; i < optimize_num_gpus; i++){
            optimize_def_freqs.gpu_freq[i] = gpuf_pol_list[i][0];
        }
    }
    verbose_node_freqs(3, &optimize_def_freqs);
#endif

    node_freqs_copy(&last_nodefreq_sel, &optimize_def_freqs);
    debug("optimize init ends");

    return EAR_SUCCESS;
}


state_t policy_end(polctx_t *c)
{
    policy_end_summary(2); // Summary of optimization

    if (c != NULL) {
        return mpi_app_end(c);
    }

    return EAR_ERROR;
}


state_t policy_loop_init(polctx_t *c, loop_id_t *l)
{
    if (c != NULL) {
        // projection_reset(optimize_num_pstates);
        int_reset_data();

        return EAR_SUCCESS;
    }

    return EAR_ERROR;
}


state_t policy_loop_end(polctx_t *c,loop_id_t *l)
{
    return EAR_SUCCESS;
}


state_t policy_apply(polctx_t *c, signature_t *sig, node_freqs_t *freqs, int *ready)
{

    int gready; // GPU ready
    ulong curr_freq,  curr_pstate;
    sig_ext_t *se     = (sig_ext_t *) sig->sig_ext;
    optimize_readiness = EAR_POLICY_CONTINUE;

    if ((c == NULL) || (c->app == NULL)) {
        *ready = EAR_POLICY_CONTINUE;
        return EAR_ERROR;
    }

    verbose_master(2, "%sCPU COMP phase%s", COL_BLU, COL_CLR);

    verbose_master(0, "%sOptimize...........starts.......(%s) status %s %s", COL_GRE, optimize_criteria, (optimize_state == FIRST?"FIRST":"SEARCHING"), COL_CLR);


    /* GPU Policy : It's independent of the CPU and IMC frequency selection */
    if (gpus.node_policy_apply != NULL) {
        // Basic support for GPUs
        verbose_master(2, "%sOptimize:GPU%s ",COL_GRE,COL_CLR);
        gpus.node_policy_apply(c, sig, freqs, &gready);
        gpu_ready = gready;
    } else {
        gpu_ready = EAR_POLICY_READY;
    }

    /* This use case applies when IO bound for example */
    if (cpu_ready == EAR_POLICY_READY) {
        return EAR_SUCCESS;
    }


    if (c->use_turbo) {
        min_pstate = 0;
    } else {
        min_pstate = frequency_closest_pstate(c->app->max_freq); // TODO: Migrate to the new API
    }


    // This is the frequency at which we were running
    // sig is the job signature adapted to the node for the energy models
    signature_copy(my_app, sig);
    memcpy(my_app->sig_ext, se, sizeof(sig_ext_t));

    if (c->pc_limit > 0) {
        verbose_master(2, "Powercap node limit set to %u", c->pc_limit);
        curr_freq = frequency_closest_high_freq(my_app->avg_f, 1);
    } else {
        curr_freq = *(c->ear_frequency);
    }

    curr_pstate = frequency_closest_pstate(curr_freq);

    // Added for the use case where energy models are not used
    sig_ready[curr_pstate] = 1;
    signature_copy(&sig_list[curr_pstate], sig);

    /* If default, go to next pstate*/
    /* Else check to go to next */
    if (optimize_state == FIRST){
      verbose_master(2 , "Optimize, FIRST state: going to next ");
      policy_no_models_go_next_optimize(curr_pstate,&optimize_readiness ,freqs,optimize_num_pstates);
      optimize_state = SEARCHING;
    }else{
      if (curr_pstate == 0){
        optimize_readiness = EAR_POLICY_READY;
        verbose_master(2 , "%sWarning Optimize policy: curr pstate 0 and SEARCH state%s",COL_RED,COL_CLR);
        freqs->cpu_freq[0] = frequency_pstate_to_freq(curr_pstate);
        optimize_state = FIRST;
      }else{
        uint go_next;
        go_next = policy_no_models_is_better_optimize(&sig_list[curr_pstate], &sig_list[curr_pstate -1]);
        if (go_next) policy_no_models_go_next_optimize(curr_pstate,&optimize_readiness ,freqs,optimize_num_pstates);
        else{         
          optimize_state = FIRST;
          policy_no_models_stop_searching(curr_pstate, &optimize_readiness, freqs);
        }
      }
    }
    verbose_master(2, "Optimize end: Freq selected %lu state %s ", freqs->cpu_freq[0], (optimize_state == FIRST?"FIRST":"SEARCHING"));
    /* Turbo is pending*/
#if MPI_OPTIMIZED
    /* For MPI optn */
    for (uint lp = 0; lp < num_processes; lp++){
      sig_shared_region[lp].mpi_freq = freqs->cpu_freq[0];
    }
#endif

    /* If we use the same CPU freq for all cores, we set for all processes, not really needed because domain in NODE */
    set_all_cores(freqs->cpu_freq, num_processes, freqs->cpu_freq[0]);

    *ready = optimize_readiness;
    return EAR_SUCCESS;
}


state_t policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *last_sig, int *ok)
{
    *ok = 1;
    if ((c==NULL) || (curr_sig==NULL) || (last_sig==NULL))
    {
        return EAR_ERROR;
    }
    return EAR_SUCCESS;

}


state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set, signature_t * s)
{
    debug("policy_get_default_freq");
    if (c != NULL)
    {
        node_freqs_copy(freq_set, &optimize_def_freqs);
        // WARNING: A function designed as a getter calls a function designed as a setter.
        if ((gpus.restore_settings != NULL) && (s != NULL)) {
            gpus.restore_settings(c, s, freq_set);
        }
    } else {
        return EAR_ERROR;
    }

    return EAR_SUCCESS;
}


state_t policy_max_tries(polctx_t *c,int *intents)
{
    *intents = 100;
    return EAR_SUCCESS;
}


state_t policy_domain(polctx_t *c,node_freq_domain_t *domain)
{
    domain->cpu                  = POL_GRAIN_NODE;
    domain->mem                  = POL_NOT_SUPPORTED;

#if USE_GPUS
    if (optimize_num_gpus) domain->gpu = POL_GRAIN_CORE;
    else             domain->gpu = POL_NOT_SUPPORTED;
#else
    domain->gpu                  = POL_NOT_SUPPORTED;
#endif

    domain->gpumem = POL_NOT_SUPPORTED;

    return EAR_SUCCESS;
}


state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (c != NULL) {
        state_t st = mpi_call_init(c, call_type);
        policy_mpi_init_optimize(c, call_type, freqs, process_id);
        return st;
    } else {
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}


state_t policy_mpi_end(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (c != NULL)
    {
        policy_mpi_end_optimize(freqs, process_id);
        return mpi_call_end(c, call_type);
    }
    else return EAR_ERROR;
}


state_t policy_cpu_gpu_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    if (c == NULL) return EAR_ERROR;
    verbose_master(2,"%sCPU-GPU  phase%s",COL_BLU,COL_CLR);
    cpu_ready = EAR_POLICY_READY;
    ulong def_freq = FREQ_DEF(c->app->def_freq);
    uint def_pstate = frequency_closest_pstate(def_freq);
    for (uint i=0;i<num_processes;i++) freqs->cpu_freq[i] = frequency_pstate_to_freq(def_pstate);
    memcpy(last_nodefreq_sel.cpu_freq,freqs->cpu_freq,MAX_CPUS_SUPPORTED*sizeof(ulong));
#if USE_GPUS
    if (gpus.cpu_gpu_settings != NULL){
      return gpus.cpu_gpu_settings(c, my_sig, freqs);
    }else{
      /* GPU mem is pending */
      for (uint i = 0; i < optimize_num_gpus; i++) {
          freqs->gpu_freq[i] = optimize_def_freqs.gpu_freq[i];
      }
      gpu_ready = EAR_POLICY_READY;
    }
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif

    return EAR_SUCCESS;
}


state_t policy_io_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs)
{
    if (c == NULL) return EAR_ERROR;

    const uint min_ener_io_veb_lvl = 2; // Used to control the verbose level in this function

    verbose_master(min_ener_io_veb_lvl, "%sOptimize%s CPU I/O settings", COL_BLU, COL_CLR);

    cpu_ready = EAR_POLICY_READY;
    int_reset_data();

    for (uint i = 0; i < num_processes; i++) {
        freqs->cpu_freq[i] = optimize_def_freqs.cpu_freq[i];

        signature_t local_sig;
        signature_from_ssig(&local_sig, &sig_shared_region[i].sig);

        uint io_bound;
        is_io_bound(&local_sig, sig_shared_region[i].num_cpus, &io_bound);

        uint busy_waiting;

        if (!io_bound) {
            // TODO: Why?
            // is_cpu_busy_waiting(&local_sig, sig_shared_region[i].num_cpus, &busy_waiting); Oriol
            is_process_busy_waiting(&sig_shared_region[i].sig, &busy_waiting); // Julita
            if (busy_waiting) {
                freqs->cpu_freq[i] = frequency_pstate_to_freq(optimize_num_pstates - 1);
            }
        } else if (me_top.vendor == VENDOR_INTEL) {
             freqs->cpu_freq[i] = frequency_pstate_to_freq(optimize_num_pstates - 1);
        }

        char ssig_buff[256];
        ssig_tostr(&sig_shared_region[i].sig, ssig_buff, sizeof(ssig_buff));

        verbose_master(min_ener_io_veb_lvl, "%s.[%d] [I/O config: %u, I/O busy waiting config: %u] %s",
                node_name, i, io_bound, busy_waiting, ssig_buff);
        
        /* We copy in shared memory to be used later in MPI init/end if needed. */
	    sig_shared_region[i].new_freq = freqs->cpu_freq[i];
    }

    memcpy(last_nodefreq_sel.cpu_freq, freqs->cpu_freq, MAX_CPUS_SUPPORTED * sizeof(ulong));


#if USE_GPUS
    if (gpus.io_settings != NULL){
      return gpus.io_settings(c, my_sig, freqs);
    }else{
      gpu_ready = EAR_POLICY_READY;
    }
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif
    /* GPU mem is pending */

    return EAR_SUCCESS;
}


state_t policy_busy_wait_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    if (c == NULL)
    {
        return EAR_ERROR;
    }

    const uint min_ener_bw_veb_lvl = 2; // Used to control the verbose level in this function

    verbose_master(min_ener_bw_veb_lvl, "%sOptimize%s CPU Busy waiting settings", COL_BLU, COL_CLR);

    cpu_ready = EAR_POLICY_READY;
    int_reset_data();

    for (uint i = 0; i < num_processes; i++) { 

        signature_t local_sig;
        signature_from_ssig(&local_sig, &sig_shared_region[i].sig);

        uint busy_waiting;
        // is_cpu_busy_waiting(&local_sig, sig_shared_region[i].num_cpus, &busy_waiting);
        is_process_busy_waiting(&sig_shared_region[i].sig, &busy_waiting);
        char ssig_buff[256];
        ssig_tostr(&sig_shared_region[i].sig, ssig_buff, sizeof(ssig_buff));
        if (busy_waiting) {
            verbose_master(min_ener_bw_veb_lvl, "%s.[%d] Busy waiting config %s", node_name, i, ssig_buff);
            freqs->cpu_freq[i] = frequency_pstate_to_freq(optimize_num_pstates - 1);
        } else {
            verbose_master(min_ener_bw_veb_lvl, "%s.[%d] NO Busy waiting config %s ", node_name, i, ssig_buff);
            freqs->cpu_freq[i] = optimize_def_freqs.cpu_freq[i];
        }
        

        /* We copy in shared  memory to be used later in MPI init/end if needed */
        sig_shared_region[i].new_freq = freqs->cpu_freq[i] ;
    }

    copy_cpufreq_sel(last_nodefreq_sel.cpu_freq, freqs->cpu_freq, sizeof(ulong)*MAX_CPUS_SUPPORTED);
    verbose_master(2,"GPU policy defined %s", (gpus.busy_wait_settings == NULL?"NO":"YES"));
#if USE_GPUS
    if (gpus.busy_wait_settings != NULL){
      return gpus.busy_wait_settings(c, my_sig, freqs);
    }else{
      gpu_ready = EAR_POLICY_READY;
    }
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif
    /* GPU mem is pending */
    return EAR_SUCCESS;
}


state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    /* WARNING: my_sig can be NULL */
    if (c != NULL) {
        state_t st = EAR_SUCCESS;
        verbose_master(2,"min_energy policy_restore_settings");
        node_freqs_copy(freqs, &optimize_def_freqs);

        if (gpus.restore_settings != NULL){
            st = gpus.restore_settings(c,my_sig,freqs);
        }
        node_freqs_copy(&last_nodefreq_sel,freqs);
        int_reset_data();

        return st;
    } else {
        return EAR_ERROR;
    }
}


state_t policy_new_iteration(polctx_t *c,signature_t *sig)
{
    if (gpus.new_iter != NULL){
        /* Basic support for GPUS */
        return gpus.new_iter(c,sig);
    }
    return EAR_SUCCESS;
}




static state_t policy_no_models_go_next_optimize(int curr_pstate,int *ready,node_freqs_t *freqs,unsigned long num_pstates)
{
    ulong *best_freq = freqs->cpu_freq;
    int next_pstate;
    if ((curr_pstate  <(num_pstates-1))){
        *ready      = EAR_POLICY_TRY_AGAIN;
        next_pstate = curr_pstate + 1;
    }else{
        *ready      = EAR_POLICY_READY;
        next_pstate = curr_pstate;
    }
    *best_freq = frequency_pstate_to_freq(next_pstate);
    return EAR_SUCCESS;
}

static void policy_no_models_stop_searching(int curr_pstate, int *ready,node_freqs_t *freqs)
{
  freqs->cpu_freq[0] = frequency_pstate_to_freq(curr_pstate - 1);
  *ready      = EAR_POLICY_READY;
}


static uint policy_no_models_is_better_optimize(signature_t * curr_sig, signature_t *prev_sig)
{
  double curr_data, old_data;
  uint   result;

  switch (metric_to_optimize){
    case CPI      : curr_data = curr_sig->CPI;old_data = prev_sig ->CPI; break;
    case GFLOPS   : curr_data = curr_sig->Gflops;old_data = prev_sig->Gflops; break;
    case GFLOPS_W : curr_data = curr_sig->Gflops/curr_sig->DC_power;old_data = prev_sig->Gflops/prev_sig->DC_power; break;
    case GBS_W    : curr_data = curr_sig->GBS/curr_sig->DC_power;old_data = prev_sig->GBS/prev_sig->DC_power; break;
    case GPU      : return 0;
  }

  switch (metric_strategy){
    case min_value: result = (curr_data < old_data); break;
    case max_value: result = (curr_data > old_data); break;
    case ratio_value: {
      float penalty = 100.0 - ((float)curr_sig->def_f/(float)prev_sig->def_f)*100.0;
      float benefit = 100.0 - (float)(curr_data/old_data)*100.0;
      result = (benefit > penalty);
      verbose_master(2,"%sOptimize ratio penalty %f benefit %f%s", COL_GRE, penalty, benefit, COL_CLR);
    }
  }
  verbose_master(2,"%sOptimize CURR metric %lf PREV metric %lf evaluation %u%s", COL_GRE,curr_data, old_data, result,COL_CLR);
  return result;
}


static state_t policy_mpi_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
#if !SINGLE_CONNECTION && MPI_OPTIMIZED
    // You can implement optimization at MPI call entry here.

    if (ear_mpi_opt &&
      (lib_shared_region->num_processes > 1)){
      return EAR_SUCCESS;
    }    
#endif
    return EAR_SUCCESS;
}


static void policy_end_summary(int verb_lvl)
{
#if !SINGLE_CONNECTION && MPI_OPTIMIZED
    // You can show a summary of your optimization at MPI call level here.
#endif
}


static state_t policy_mpi_end_optimize(node_freqs_t *freqs, int *process_id)
{
#if !SINGLE_CONNECTION && MPI_OPTIMIZED
    // You can implement optimization at MPI call exit here.
    if (ear_mpi_opt &&
      (lib_shared_region->num_processes > 1)){
    }
#endif

    return EAR_SUCCESS;
}
