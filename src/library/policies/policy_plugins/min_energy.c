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

// #define SHOW_DEBUGS 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/environment.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/api/clasify.h>
#include <library/metrics/metrics.h>
#include <library/metrics/signature_ext.h>
#include <management/cpufreq/frequency.h>
#include <common/types/projection.h>
#include <daemon/local_api/eard_api.h>
#include <daemon/powercap/powercap_status.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/generic.h>
#include <library/metrics/gpu.h>

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define FREQ_DEF(f) (!ext_def_freq?f:ext_def_freq)
#else
#define FREQ_DEF(f) f
#endif

/*  IMC management */
extern uint dyn_unc;
extern double imc_extra_th;
extern uint *imc_max_pstate,*imc_min_pstate;
extern uint imc_num_pstates, imc_devices;
static uint imc_set_by_hw = 1;

static int min_pstate;
static uint ref_imc_pstate;
static uint last_imc_pstate;

static imc_data_t *imc_data;

/*  Utility */
ulong avg_to_khz(ulong freq_khz);

/*  Frequency management */
static uint last_cpu_pstate;
static node_freqs_t min_energy_def_freqs;
static node_freqs_t last_nodefreq_sel;

static uint num_processes;
static ulong nominal_node;

/*  Load balance management */
extern int my_globalrank;
static int nump;

/* These pointers are computed in metrics.c */
static mpi_information_t *my_node_mpi_calls;
static mpi_calls_types_t *my_node_mpi_types_calls;

static int max_mpi, min_mpi;
static int freq_per_core = 0;

static uint signature_is_unbalance = 0;

static double *percs_mpi, *last_percs_mpi;
static uint *critical_path, *last_critical_path;

static double last_mpi_stats_perc_mpi_sd, last_mpi_stats_perc_mpi_mean,
              last_mpi_stats_perc_mpi_mag, last_mpi_stats_perc_mpi_median;

/*  Config */
extern uint enable_load_balance;
extern uint use_turbo_for_cp;
extern uint try_turbo_enabled;
extern uint use_energy_models;

static ulong min_cpufreq = 0;
static uint network_use_imc = 1; /* This variable controls the UFS when the application is communication intensive , set to 1 means UFS will not be applied*/

#define SELECT_CPUFREQ 		100
#define SELECT_IMCFREQ 		101
#define COMP_IMCREF		 	102
#define TRY_TURBO_CPUFREQ   103

#define EXTRA_TH    0.05

/*  Policy/Other */
static uint min_energy_state = SELECT_CPUFREQ;
static signature_t *my_app;

// No energy models extra vars
static signature_t *sig_list;
static uint *sig_ready;

static polsym_t gpus;
static const ulong **gpuf_pol_list;
static const uint *gpuf_pol_list_items;

static signature_t cpufreq_signature;

static uint block_type;
static uint first_time = 1;

extern uint cpu_ready, gpu_ready, gidle;
extern uint last_earl_phase_classification;

state_t policy_init(polctx_t *c)
{
    // char *cuse_imc_ref;
    char *cmin_cpufreq = NULL; // getenv(SCHED_MIN_CPUFREQ);
    char *cnetwork_use_imc = NULL; // getenv(SCHED_NETWORK_USE_IMC);
    char *cimc_set_by_hw = NULL; // getenv(SCHED_LET_HW_CTRL_IMC);

    int i, sid = 0;

    if (c!=NULL){
        imc_data = calloc(c->num_pstates*NUM_UNC_FREQ,sizeof(imc_data_t));
        nominal_node = frequency_get_nominal_freq();
        verbose_master(2,"Nominal is %lu",nominal_node);
        if (is_mpi_enabled()){
            mpi_app_init(c);
            is_blocking_busy_waiting(&block_type);
            verbose_master(1,"Busy_waiting MPI set to %u",block_type == BUSY_WAITING_BLOCK);
        }
        /*  Config env variables */
        if (state_ok(read_config_env(cmin_cpufreq, SCHED_MIN_CPUFREQ))){
            min_cpufreq = atoi(cmin_cpufreq);
        }
        if (min_cpufreq) verbose_master(2,"Min CPUfreq is limited = %lu",min_cpufreq);

        if (state_ok(read_config_env(cimc_set_by_hw, SCHED_LET_HW_CTRL_IMC))) read_config(&imc_set_by_hw, cimc_set_by_hw);
        verbose_master(2,"Dyn UNC Freq mgt set to %d. Using HW selection as ref set to %d. Using an extra IMC penalty of %.2lf"
                ,dyn_unc,imc_set_by_hw,imc_extra_th);

        if (state_ok(read_config_env(cnetwork_use_imc, SCHED_NETWORK_USE_IMC))){
            read_config(&network_use_imc, cnetwork_use_imc);
        }
        verbose_master(2,"Network uses IMC set to %u",network_use_imc);

        num_processes = lib_shared_region->num_processes;

        my_app = calloc(1,sizeof(signature_t));
        my_app->sig_ext =  (void *)calloc(1,sizeof(sig_ext_t));

        /* We store the signatures for the case where models are not used */
        if (!use_energy_models){
            sig_list  = (signature_t *)calloc(c->num_pstates,sizeof(signature_t));
            sig_ready = (uint *)calloc(c->num_pstates,sizeof(uint));

            if ((sig_list==NULL) || (sig_ready==NULL)) return EAR_ERROR;

        }

        percs_mpi = calloc(num_processes, sizeof(double));
        last_percs_mpi = calloc(num_processes, sizeof(double));

        critical_path = calloc(num_processes,sizeof(uint));
        last_critical_path = calloc(num_processes,sizeof(uint));

        node_freqs_alloc(&min_energy_def_freqs);
        node_freqs_alloc(&last_nodefreq_sel);

        last_mpi_stats_perc_mpi_sd = last_mpi_stats_perc_mpi_mean =
            last_mpi_stats_perc_mpi_mag = last_mpi_stats_perc_mpi_median = 0;

        /* Configuring default freqs */
        for (i=0;i<num_processes;i++) {
            min_energy_def_freqs.cpu_freq[i] = *(c->ear_frequency);
        }

        for (sid=0; sid < imc_devices; sid ++){
            min_energy_def_freqs.imc_freq[sid*IMC_VAL+IMC_MAX] = imc_min_pstate[sid];
            min_energy_def_freqs.imc_freq[sid*IMC_VAL+IMC_MIN] = imc_max_pstate[sid];
        }

#if USE_GPUS
        memset(&gpus,0,sizeof(gpus));
        if (c->num_gpus){
            if (policy_gpu_load(c->app,&gpus) != EAR_SUCCESS){
                verbose_master(2,"Error loading GPU policy");
            }
            verbose_master(2,"Initialzing GPU policy part");
            if (gpus.init != NULL) gpus.init(c);
            if (gpu_lib_freq_list(&gpuf_pol_list, &gpuf_pol_list_items) != EAR_SUCCESS){
                verbose_master(2,"Error asking for gpu freq list");
            }
            /* replace by default settings in GPU policy */
            for (i=0;i<c->num_gpus;i++){
                min_energy_def_freqs.gpu_freq[i] = gpuf_pol_list[i][0];
            }
        }
        verbose_node_freqs(2,&min_energy_def_freqs);
#endif
        node_freqs_copy(&last_nodefreq_sel, &min_energy_def_freqs);
        return EAR_SUCCESS;
    }else return EAR_ERROR;
}


// TODO: no_models policy does not have policy end func
state_t policy_end(polctx_t *c)
{
    if (c != NULL) return  mpi_app_end(c);
    else return EAR_ERROR;
}

state_t policy_loop_init(polctx_t *c,loop_id_t *l)
{
    if (c!=NULL){
        projection_reset(c->num_pstates);
        if (!use_energy_models){
            memset(sig_ready,0,sizeof(uint)*c->num_pstates);
        }
        return EAR_SUCCESS;
    }else{
        return EAR_ERROR;
    }
}

state_t policy_loop_end(polctx_t *c,loop_id_t *l)
{
    if ((c!=NULL) && (c->reset_freq_opt))
    {
        *(c->ear_frequency) = eards_change_freq(FREQ_DEF(c->app->def_freq));
    }
    return EAR_SUCCESS;
}

static state_t int_policy_ok(signature_t *curr_sig,signature_t *last_sig,int *ok);
static state_t policy_no_models_go_next_me(int curr_pstate,int *ready,node_freqs_t *freqs,unsigned long num_pstates);
static uint policy_no_models_is_better_min_energy(signature_t * curr_sig, signature_t *prev_sig, double time_ref,
        double cpi_ref, double gbs_ref, double penalty_th);

state_t policy_apply(polctx_t *c,signature_t *sig,node_freqs_t *freqs,int *ready)
{
    /*  Selected freqs */
    ulong *new_freq=freqs->cpu_freq;

    /*  Other */
    int gready;
    uint sid = 0;

    /*  Used by policy */
    double power_ref,time_ref,max_penalty, cpi_ref, gbs_ref;

    ulong best_freq;
    ulong curr_freq, curr_imc_freq;
    ulong curr_pstate,def_pstate,def_freq, prev_pstate; // prev_pstate added
    sig_ext_t *se;

    uint curr_imc_pstate;
    uint eUFS = dyn_unc;

    double sig_vpi, node_sig_vpi, max_vpi, local_penalty, base_penalty;
    uint local_process_with_high_vpi;

    uint cbound, mbound, turbo_set = 0;

    se = (sig_ext_t *)sig->sig_ext;
    my_node_mpi_calls = se->mpi_stats;

    // TODO: Can we assign this values calling some function who return values based on current architecture?
    ulong max_cpufreq_sel = 0, min_cpufreq_sel = 10000000;

    double lvpi;

    if ((c == NULL) || (c->app == NULL)){
        *ready=EAR_POLICY_CONTINUE;
        return EAR_ERROR;
    }
    verbose_master(2,"%sCPU COMP phase%s",COL_BLU,COL_CLR);

    verbose_master(2,"%sMin_energy_to_solution...........starts.......%s",COL_GRE,COL_CLR);

    if (gpus.node_policy_apply != NULL){
        /* Basic support for GPUS */
        gpus.node_policy_apply(c,sig,freqs,&gready);
        gpu_ready = gready;
    }else{
        gpu_ready = EAR_POLICY_READY;
    }

    if (cpu_ready == EAR_POLICY_READY){
        return EAR_SUCCESS;
    }

    max_penalty = c->app->settings[0];
    base_penalty = max_penalty;

    if (c->use_turbo) min_pstate = 0;
    else min_pstate = frequency_closest_pstate(c->app->max_freq);


    // Default values
    def_freq = FREQ_DEF(c->app->def_freq);
    def_pstate = frequency_closest_pstate(def_freq);

    // This is the frequency at which we were running
    signature_copy(my_app,sig);
    memcpy(my_app->sig_ext,se,sizeof(sig_ext_t));

#ifdef POWERCAP
    if (c->pc_limit > 0){
        verbose_master(2,"Powercap node limit set to %u",c->pc_limit);
        curr_freq = frequency_closest_high_freq(my_app->avg_f,1);
    }else{
        curr_freq = *(c->ear_frequency);
    }
#else
    curr_freq = *(c->ear_frequency);
#endif

    curr_pstate = frequency_closest_pstate(curr_freq);
    // Added for the use case where energy models are not used
    if (!use_energy_models){
        sig_ready[curr_pstate]=1;
        signature_copy(&sig_list[curr_pstate], sig);
    }

    /* We save data in imc_data structure for policy comparison. We are computing the imc_pstate based on the avg_imc_freq,
     * no looking at selection */
    pstate_t tmp_pstate;
    curr_imc_freq = avg_to_khz(my_app->avg_imc_f);
    if (avgfreq_to_pstate((pstate_t *)imc_pstates,imc_num_pstates,curr_imc_freq,&tmp_pstate) != EAR_SUCCESS){
        verbose_master(2,"Current Avg IMC freq %lu cannot be converted to psate",curr_imc_freq);
    }
    curr_imc_pstate = tmp_pstate.idx;
    if (eUFS){
        debug("CPU pstate %lu IMC freq %lu IMC pstate %u",curr_pstate,my_app->avg_imc_f,curr_imc_pstate);
        copy_imc_data_from_signature(imc_data,curr_pstate,curr_imc_pstate,my_app);
    }

    int policy_stable;
    int_policy_ok(sig, &cpufreq_signature, &policy_stable);
    debug("Policy ok returns %d",policy_stable);

    /* If we are not stable, we start again */
    if ((min_energy_state != SELECT_CPUFREQ) && !policy_stable){
        debug("%spolicy not stable%s", COL_RED, COL_CLR);
        min_energy_state = SELECT_CPUFREQ;
        // TODO: we could select another phase here if we have the information
        last_earl_phase_classification = APP_COMP_BOUND;
    }

    if ((min_energy_state == SELECT_CPUFREQ) || (eUFS == 0)){
        /**** SELECT_CPUFREQ ****/
        verbose_master(2,"%sSelecting CPU frequency %s. eUFS %u", COL_BLU, COL_CLR, eUFS);

        last_cpu_pstate = curr_pstate;
        last_imc_pstate = curr_imc_pstate;
        is_cpu_bound(my_app, &cbound);
        is_mem_bound(my_app, &mbound);
        debug("App CPU bound %u memory bound %u",cbound,mbound);

        // If is not the default P_STATE selected in the environment, a projection
        // is made for the reference P_STATE in case the coefficents were available.
        // NEW: This code is new because projections from lower pstates are reporting bad cpu freqs
        if (use_energy_models && !are_default_settings(&last_nodefreq_sel,&min_energy_def_freqs)){
            verbose_node_freqs(2,&last_nodefreq_sel);
            verbose_node_freqs(2,&min_energy_def_freqs);

            set_default_settings(freqs,&min_energy_def_freqs);
            if (gpus.restore_settings != NULL) {
                gpus.restore_settings(c,sig,freqs);
            }

            node_freqs_copy(&last_nodefreq_sel,freqs);

            verbose_master(2,"Setting default conf for ME");
            *ready = EAR_POLICY_TRY_AGAIN;
            return EAR_SUCCESS;
        }
        verbose_master(2,"Min_energy per process set to %u",freq_per_core);

        /* here we are running at the default freqs */
        for (uint i=0;i<nump;i++){
            verbose_master(3,"Processing proc %d freq per core = %d",i,freq_per_core);
            if (!freq_per_core){
                compute_total_node_instructions(lib_shared_region,sig_shared_region,&my_app->instructions);
                compute_total_node_flops(lib_shared_region,sig_shared_region,my_app->FLOPS);
                compute_total_node_cycles(lib_shared_region,sig_shared_region,&my_app->cycles);
                compute_vpi(&sig_vpi,sig);
                compute_vpi(&node_sig_vpi,my_app);
                local_process_with_high_vpi = compute_max_vpi(lib_shared_region,sig_shared_region,&max_vpi);
                debug("Node signature MR[%d] CPI %.3lf VPI %.3lf/Node CPI %.3lf VPI %.3lf/MAX VPI %.3lf",
                        my_globalrank,sig->CPI,sig_vpi,(double)my_app->cycles/(double)my_app->instructions,node_sig_vpi,
                        max_vpi);
                my_app->CPI = (double)my_app->cycles/(double)my_app->instructions;

                /* With these two lines, we force the VPI of the signature to the most loaded in the node */
                my_app->instructions = sig_shared_region[local_process_with_high_vpi].sig.instructions;
                memcpy(my_app->FLOPS,sig_shared_region[local_process_with_high_vpi].sig.FLOPS,sizeof(ull)*FLOPS_EVENTS);
                local_penalty = base_penalty;
            }else{
                compute_ssig_vpi(&lvpi, &sig_shared_region[i].sig);
                ulong eff_p_cpuf, process_avgcpuf;
                process_avg_cpu_freq(&sig_shared_region[i].cpu_mask,cpufreq_data_per_core,&process_avgcpuf);
                from_minis_to_sig(my_app,&sig_shared_region[i].sig);

                /*
                 * verbose_master(2,"Per-processSig MR[%d][%d] CPI %.3lf AvgCPUF %.2f (%lf,%lf) LVPI=%.3lf CP=%u",
                 * my_globalrank,i,my_app->CPI,(float)process_avgcpuf/1000000.0,my_node_mpi_calls[i].perc_mpi,
                 * my_node_mpi_calls[min_mpi].perc_mpi, lvpi, critical_path[i]);
                 */

                if (critical_path[i] == 1){
                    /* Critical path */
                    if (!mbound){
                        eff_p_cpuf = frequency_closest_high_freq(process_avgcpuf,0);
                        if (eff_p_cpuf < process_avgcpuf){
                            eff_p_cpuf = frequency_closest_high_freq(process_avgcpuf+100000,0);
                        }
                        if (use_turbo_for_cp){
                            new_freq[i] = frequency_pstate_to_freq(0);
                        }else{
                            new_freq[i] = ear_min(eff_p_cpuf, frequency_pstate_to_freq(1));
                        }
                        verbose_master(2,"CPU freq for CP process %d is %lu (!MBOUND use_turbo %u MIN_MPI %u)",
                                i,new_freq[i],use_turbo_for_cp,percs_mpi[i] == percs_mpi[min_mpi]);
                        continue;
                    }else{
                        local_penalty = base_penalty / 2;
                    }
                }else if (critical_path[i] == 2){
                    /* Lot of MPI */
                    local_penalty = 0.5;
                    sig_shared_region[i].unbalanced = 1;
                }else{
                    /* Mix process */
                    local_penalty = base_penalty + EXTRA_TH*(my_node_mpi_calls[i].perc_mpi
                            - my_node_mpi_calls[min_mpi].perc_mpi)/10.0;
                }
                verbose_master(2,"Penalty %f", local_penalty);
            }
            if (use_energy_models){
                /* If energy models are availables, we use them for CPU frequency selection */
                curr_freq = def_freq;
                curr_pstate = def_pstate;
                compute_reference(c,my_app,&curr_freq,&def_freq,&best_freq,&time_ref,&power_ref);
                verbose_master(3,"Time ref %lf Power ref %lf Freq ref %lu",time_ref,power_ref,best_freq);
                compute_cpu_freq_min_energy(c,my_app,best_freq,time_ref,power_ref,
                        local_penalty,curr_pstate,min_pstate,c->num_pstates,&new_freq[i]);
            }else{
                // If application is compute bound, we don't reduce CPU freq
                if (cbound){
                    *new_freq = frequency_pstate_to_freq(def_pstate);
                    *ready = EAR_POLICY_READY;
                    debug("Application cbound. Next freq %lu Next state ready %d", *new_freq, *ready);
                }else{
                    /* If energy models are not available, we just execute a linear search */
                    if (sig_ready[def_pstate] == 0){
                        debug("signature for default pstate %u is not ready", def_pstate);
                        *new_freq = def_freq;
                        *ready = EAR_POLICY_TRY_AGAIN;
                    }else{
                        signature_t *prev_sig;
                        time_ref = sig_list[def_pstate].time;
                        cpi_ref = sig_list[def_pstate].CPI;
                        gbs_ref = sig_list[def_pstate].GBS;
                        // time_max = time_ref * (1 + base_penalty);
                        /* This is the normal case */
                        if (curr_pstate != def_pstate){
                            prev_pstate = curr_pstate - 1;
                            prev_sig = &sig_list[prev_pstate];
                            if (policy_no_models_is_better_min_energy(my_app, prev_sig, time_ref,
                                        cpi_ref, gbs_ref, base_penalty)){
                                policy_no_models_go_next_me(curr_pstate, ready, freqs, c->num_pstates);
                            }else{
                                *new_freq = frequency_pstate_to_freq(prev_pstate);
                                *ready = EAR_POLICY_READY;
                            }
                        }else{
                            policy_no_models_go_next_me(curr_pstate, ready, freqs, c->num_pstates);
                        }
                    }
                }
            }
            if (min_cpufreq) new_freq[i] = ear_max(min_cpufreq, new_freq[i]);
            debug("CPU freq for process %d is %lu",i,new_freq[i]);
            max_cpufreq_sel = ear_max(max_cpufreq_sel, new_freq[i]);
            min_cpufreq_sel = ear_min(min_cpufreq_sel, new_freq[i]);
        }
        if (!freq_per_core){
            set_all_cores(new_freq, num_processes, new_freq[0]);
        }

        /* We check if turbo is an option */
        if ((min_cpufreq_sel == nominal_node) && cbound && try_turbo_enabled){
            turbo_set = 1;
            verbose_master(2,"Using turbo because it is cbound and turbo enabled");
            set_all_cores(new_freq, num_processes, frequency_pstate_to_freq(0));
        }

        copy_cpufreq_sel(last_nodefreq_sel.cpu_freq,new_freq,sizeof(ulong)*num_processes);

        /* We will use this signature to detect changes in the sig compared when the one used to compute the CPU freq */
        verbose_master(3,"Updating cpufreq_signature");
        se = (sig_ext_t *)sig->sig_ext;
        signature_copy(&cpufreq_signature, sig);
        memcpy(cpufreq_signature.sig_ext, se, sizeof(sig_ext_t));

        /* IMC freq selection */
        if ((use_energy_models || *ready==EAR_POLICY_READY) &&
                eUFS && !((earl_phase_classification == APP_MPI_BOUND) && network_use_imc)){

            /* If we are compute bound, and CPU freq is nominal, we can reduce the IMC freq */
            /* IMC_MAX means the maximum frequency, lower pstate , IMC_MIN means minimum frequency and maximum pstate */
            if ((min_cpufreq_sel == nominal_node) && cbound){
                if (sig->GBS <= GBS_BUSY_WAITING){
                    debug("GBS LEQ GBS_BUSY_WAITING (%lf, %lf)", sig->GBS, GBS_BUSY_WAITING);
                    for (sid=0; sid < imc_devices; sid ++) {
                        freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = imc_max_pstate[sid] - 1;
                    }
                }
                else{
                    debug("GBS GT GBS_BUSY_WAITING selecting the 0.25 pstate...");
                    for (sid=0; sid < imc_devices; sid ++){

                        freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = select_imc_pstate(imc_num_pstates, 0.25);
                    }
                }
            }else{
                debug("min selected CPU freq %lu - nominal CPU freq %lu - cbound %u",
                        min_cpufreq_sel, nominal_node, cbound);
                for (sid=0; sid < imc_devices; sid ++){
                    freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = imc_min_pstate[sid];
                }
            }
            /* ps_nothing means we will not change it. We could use the imc_max_pstate */
            if (imc_set_by_hw){
                for (sid=0; sid < imc_devices; sid ++) freqs->imc_freq[sid*IMC_VAL+IMC_MIN] = imc_max_pstate[sid];
            }else{
                for (sid=0; sid < imc_devices; sid ++)
                    freqs->imc_freq[sid*IMC_VAL+IMC_MIN] =
                        ear_min(freqs->imc_freq[sid*IMC_VAL+IMC_MAX] + 1, imc_num_pstates -1);
            }
            sid = 0;
            debug("%sIMC freq selection%s %lu-%lu",COL_GRE,COL_CLR,
                    imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MAX]].khz,
                    imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MIN]].khz);
        }else{
            verbose_master(3,"%sNot IMC freq selection%s : using energy models %u eUFS %u MPI bound = %u Network_IMC %u",
                    COL_RED,COL_CLR, use_energy_models, eUFS,earl_phase_classification == APP_MPI_BOUND,network_use_imc);
            /* If application is network bound, we don't reduce the uncore frequency */
            if ((use_energy_models || *ready==EAR_POLICY_READY) &&
                    eUFS && (earl_phase_classification == APP_MPI_BOUND) && network_use_imc){
                eUFS = 0;
                for (sid=0; sid < imc_devices; sid ++) {
                    freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = imc_min_pstate[sid];
                    freqs->imc_freq[sid*IMC_VAL+IMC_MIN] = imc_min_pstate[sid] + 1;
                }
            }
        }

        /* Next state selection */
        if (turbo_set){
            // TODO: what happen then if user does not activate IMC controller? The policy will also go to SELECT_IMCFREQ state
            min_energy_state = TRY_TURBO_CPUFREQ;
            *ready = EAR_POLICY_TRY_AGAIN;
        }else{
            if (!eUFS){
                /* If we are not using energy models, `ready` has been set by `no_models` policy */
                if (use_energy_models){
                    *ready = EAR_POLICY_READY;
                }
                min_energy_state = SELECT_CPUFREQ;
            }else{
                /* if we are using `no_models` policy, it tells us whether is ready */
                if (use_energy_models || *ready == EAR_POLICY_READY){
                    uint i = 0;
                    while((i<num_processes) && (last_nodefreq_sel.cpu_freq[i] == def_freq)) i++;
                    // TODO: why we check here for cbound?
                    if ((i < num_processes) || (cbound)){
                        min_energy_state = COMP_IMCREF;
                    }else{
                        ref_imc_pstate = curr_imc_pstate;
                        min_energy_state = SELECT_IMCFREQ;
                    }
                    *ready = EAR_POLICY_TRY_AGAIN;
                }else{
                    min_energy_state = SELECT_CPUFREQ;
                }
            }
        }
        memcpy(last_nodefreq_sel.imc_freq,freqs->imc_freq,imc_devices*IMC_VAL*sizeof(ulong));
    }else if ((min_energy_state == COMP_IMCREF) || (min_energy_state == TRY_TURBO_CPUFREQ)){
        /**** COMP_IMCREF ***/
        ref_imc_pstate = curr_imc_pstate;
        for (sid=0; sid < imc_devices; sid ++) {
            freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = curr_imc_pstate;
            freqs->imc_freq[sid*IMC_VAL+IMC_MIN] = imc_max_pstate[sid];
        }
        sid = 0;
        verbose_master(2,"%sComputing reference%s cpufreq %lu imc %llu-%llu",COL_BLU,COL_CLR,new_freq[0],
                imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MAX]].khz,
                imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MIN]].khz);

        min_energy_state = SELECT_IMCFREQ;
        *ready = EAR_POLICY_TRY_AGAIN;

        memcpy(freqs->cpu_freq,last_nodefreq_sel.cpu_freq,sizeof(ulong)*num_processes);
        memcpy(last_nodefreq_sel.imc_freq,freqs->imc_freq,imc_devices*IMC_VAL*sizeof(ulong));
    }else if ((min_energy_state == SELECT_IMCFREQ) && eUFS){
        /**** IMC_FREQ ***/
        verbose_master(2,"%sSelecting IMC freq%s: nominal %d - last CPU %u,IMC %u -- current CPU %lu, IMC %u",
                COL_BLU,COL_CLR,min_pstate,last_cpu_pstate,last_imc_pstate,curr_pstate,curr_imc_pstate);
        uint increase_imc = must_increase_imc(imc_data,curr_pstate,curr_imc_pstate,curr_pstate,
                ref_imc_pstate,my_app,imc_extra_th);

        /* If we are over the limit, we increase the IMC */
        if (increase_imc){
            uint must_start_again = must_start(imc_data,curr_pstate,curr_imc_pstate,curr_pstate,ref_imc_pstate,my_app);
            debug("%sWarning, passing the imc_th limit: start_again %u",COL_RED,must_start_again);
            for (sid=0; sid < imc_devices; sid ++) {
                freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = ((freqs->imc_freq[sid*IMC_VAL+IMC_MAX]>0)?
                        (freqs->imc_freq[sid*IMC_VAL+IMC_MAX]-1):0);
                freqs->imc_freq[sid*IMC_VAL+IMC_MIN] = imc_max_pstate[sid];
            }
            sid = 0;
            verbose_master(2,"Selecting IMC range %llu - %llu%s",
                    imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MAX]].khz,
                    imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MIN]].khz,COL_CLR);

            if (!must_start_again)  *ready = EAR_POLICY_READY;
            else 					*ready = EAR_POLICY_TRY_AGAIN;

            min_energy_state = SELECT_CPUFREQ;

        }else{
            /* IMC_MAX is max frequency, min_pstate */
            /* IMC_MIN is min frequency, max_pstate */
            for (sid=0; sid < imc_devices; sid ++) {
                freqs->imc_freq[sid*IMC_VAL+IMC_MAX] = ((freqs->imc_freq[sid*IMC_VAL+IMC_MAX] < imc_max_pstate[sid])?
                        freqs->imc_freq[sid*IMC_VAL+IMC_MAX]+1:imc_max_pstate[sid]);
                freqs->imc_freq[sid*IMC_VAL+IMC_MIN] = imc_max_pstate[sid];
            }
            sid = 0;
            debug("%sTrying a lower IMC freq %llu-%llu%s",COL_GRE,
                    imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MAX]].khz,
                    imc_pstates[freqs->imc_freq[sid*IMC_VAL+IMC_MIN]].khz,COL_CLR);
            /* We are assuming all sockets uses the same frequency range */
            if (freqs->imc_freq[sid*IMC_VAL+IMC_MAX] < imc_max_pstate[sid]) *ready = EAR_POLICY_TRY_AGAIN;
            else{
                *ready = EAR_POLICY_READY;
                min_energy_state = SELECT_CPUFREQ;
            }
        }
        memcpy(freqs->cpu_freq,last_nodefreq_sel.cpu_freq,sizeof(ulong)*num_processes);
        memcpy(last_nodefreq_sel.imc_freq,freqs->imc_freq,imc_devices*IMC_VAL*sizeof(ulong));
    }
    debug("Next min_energy state %u next policy state %u",min_energy_state,*ready);
    return EAR_SUCCESS;
}

static state_t int_policy_ok(signature_t *curr_sig,signature_t *last_sig,int *ok)
{

    debug("%spolicy ok evaluation%s", COL_YLW, COL_CLR);
    *ok = 1;

    sig_ext_t *se = (sig_ext_t *)curr_sig->sig_ext;
    my_node_mpi_calls 				= se->mpi_stats;
    my_node_mpi_types_calls   = se->mpi_types;

    /*  Check for load balance data */
    if (is_mpi_enabled() && enable_load_balance && use_energy_models){

        verbose_mpi_types(3,my_node_mpi_types_calls);

        uint unbalance;
        double mean, sd, mag;
        mpi_support_evaluate_lb(my_node_mpi_calls, num_processes, percs_mpi, &mean, &sd, &mag, &unbalance);


        debug("%sNode load unbalance%s: LB_TH %.2lf mean %.2lf\
                standard deviation %.2lf magnitude %.2lf unbalance ? %u", COL_MGT, COL_CLR, LB_TH, mean, sd, mag, unbalance);

        /* This function sends mpi_summary to other nodes */
        //chech_node_mpi_summary();
        /*  Check whether application has changed from unbalanced to balanced and viceversa */
        if (unbalance != signature_is_unbalance){
            if (!unbalance){
                verbose_master(2, "%sApplication becomes balanced!%s", COL_GRE, COL_CLR);
                signature_is_unbalance = 0;
            }else{
                verbose_master(2, "%sApplication becomes unbalanced%s", COL_RED, COL_CLR);
                signature_is_unbalance = 1;
                *ok=0;
            }
        }

        if (unbalance){
            /* These variables are global to the policy */
            freq_per_core = 1;
            nump = num_processes;

            double median;
            /*  Select critical path */
            mpi_support_select_critical_path(critical_path, percs_mpi, num_processes, mean, &median, &max_mpi, &min_mpi);

            /* We check if policy ok because if it is 0, it means that app became unbalanced from balanced
             * state and we don't need to check variation between policy iterations  */
            if (!first_time && *ok==1){

                mpi_support_verbose_perc_mpi_stats(2, last_percs_mpi, num_processes, last_mpi_stats_perc_mpi_mean,
                        last_mpi_stats_perc_mpi_median, last_mpi_stats_perc_mpi_sd, last_mpi_stats_perc_mpi_mag);


                /*  This code has not been encapsulated because is by (now) only for verbose purposes */
                double similarity;
                mpi_stats_evaluate_similarity(percs_mpi, last_percs_mpi, num_processes, &similarity);
                verbose_master(2, "(Orientation) Similarity between last and current: %lf", similarity);
                /* *************************************************************************** */

                uint mpi_changed;
                mpi_support_mpi_changed(mag, last_mpi_stats_perc_mpi_mag, critical_path, last_critical_path,
                        num_processes, &similarity, &mpi_changed);

                if (mpi_changed){
                    *ok = 0;
                }

            }else if (*ok){
                first_time = 0;
            }

            /*  Save current stats and critical path to be used later */
            memcpy(last_percs_mpi, percs_mpi, sizeof(double) * num_processes);
            memcpy(last_critical_path, critical_path, sizeof(uint) * num_processes);
            last_mpi_stats_perc_mpi_sd = sd;
            last_mpi_stats_perc_mpi_mean = mean;
            last_mpi_stats_perc_mpi_mag = mag;
            last_mpi_stats_perc_mpi_median = median;
        }else{
            nump = 1;
            freq_per_core = 0;
        }
    }else{
        nump = 1;
        freq_per_core = 0;
    }

    if (*ok == 1){
        /*  If signature has changed, we must start again */
        if (signatures_different(last_sig, curr_sig, 0.2)){
            verbose_master(2, "%sSignature is too different from the one used for CPU freq. starting again%s",
                    COL_RED, COL_CLR);
            *ok = 0;
        }
    }
    if (*ok == 0){
        first_time = 1;
    }

    return EAR_SUCCESS;
}

state_t policy_ok(polctx_t *c,signature_t *curr_sig,signature_t *last_sig,int *ok)
{
    state_t st;
    *ok = 0;
    if ((c==NULL) || (curr_sig==NULL) || (last_sig==NULL)) return EAR_ERROR;
    st = int_policy_ok(curr_sig,last_sig,ok);
    return st;
}

state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set,signature_t * s)
{
    if (c!=NULL){
        node_freqs_copy(freq_set,&min_energy_def_freqs);
        if ((gpus.restore_settings != NULL) && (s != NULL)) {
            gpus.restore_settings(c,s,freq_set);
        }

    }else return EAR_ERROR;
    return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
    *intents = 1;
    return EAR_SUCCESS;
}

state_t policy_domain(polctx_t *c,node_freq_domain_t *domain)
{
    domain->cpu = POL_GRAIN_CORE;
    if (dyn_unc) 	domain->mem = POL_GRAIN_NODE;
    else 			domain->mem = POL_NOT_SUPPORTED;
#if USE_GPUS
    if (c->num_gpus) domain->gpu = POL_GRAIN_CORE;
    else domain->gpu = POL_NOT_SUPPORTED;
#else
    domain->gpu = POL_NOT_SUPPORTED;
#endif
    domain->gpumem = POL_NOT_SUPPORTED;
    return EAR_SUCCESS;
}

state_t policy_mpi_init(polctx_t *c,mpi_call call_type)
{
    if (c != NULL) return mpi_call_init(c,call_type);
    else return EAR_ERROR;
}

state_t policy_mpi_end(polctx_t *c,mpi_call call_type)
{
    if (c != NULL) return mpi_call_end(c,call_type);
    else return EAR_ERROR;
}

state_t policy_io_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    if (c == NULL) return EAR_ERROR;
    verbose_master(2,"%sCPU IO phase%s",COL_BLU,COL_CLR);
    cpu_ready = EAR_POLICY_READY;
    for (uint i=0;i<num_processes;i++) freqs->cpu_freq[i] = frequency_pstate_to_freq(c->num_pstates - 1);
    memcpy(last_nodefreq_sel.cpu_freq,freqs->cpu_freq,MAX_CPUS_SUPPORTED*sizeof(ulong));
    if (dyn_unc){
        for (uint sock_id=0;sock_id<arch_desc.top.socket_count;sock_id++){
            freqs->imc_freq[sock_id*IMC_VAL+IMC_MAX] = imc_max_pstate[sock_id];
            freqs->imc_freq[sock_id*IMC_VAL+IMC_MIN] = imc_max_pstate[sock_id];
        }
        memcpy(last_nodefreq_sel.imc_freq,freqs->imc_freq,imc_devices*IMC_VAL*sizeof(ulong));
    }
#if USE_GPUS
    gpu_ready = EAR_POLICY_READY;
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif
    /* GPU mem is pending */

    return EAR_SUCCESS;
}

state_t policy_busy_wait_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    if (c == NULL) return EAR_ERROR;
    verbose_master(2,"%sCPU busy waiting phase%s",COL_BLU,COL_CLR);
    cpu_ready = EAR_POLICY_READY;
    for (uint i=0;i<num_processes;i++) freqs->cpu_freq[i] = frequency_pstate_to_freq(c->num_pstates - 1);
    copy_cpufreq_sel(last_nodefreq_sel.cpu_freq, freqs->cpu_freq, sizeof(ulong)*MAX_CPUS_SUPPORTED);
    if (dyn_unc){
        for (uint sock_id=0;sock_id<arch_desc.top.socket_count;sock_id++){
            freqs->imc_freq[sock_id*IMC_VAL+IMC_MAX] = imc_max_pstate[sock_id];
            freqs->imc_freq[sock_id*IMC_VAL+IMC_MIN] = imc_max_pstate[sock_id];
        }
        memcpy(last_nodefreq_sel.imc_freq,freqs->imc_freq,imc_devices*IMC_VAL*sizeof(ulong));
    }
#if USE_GPUS
    gpu_ready = EAR_POLICY_READY;
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif
    /* GPU mem is pending */
    return EAR_SUCCESS;
}

state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    if (c == NULL) return EAR_ERROR;
    node_freqs_copy(freqs, &min_energy_def_freqs);
    if (gpus.restore_settings != NULL){
        gpus.restore_settings(c,my_sig,freqs);
    }
    node_freqs_copy(&last_nodefreq_sel,freqs);

    return EAR_SUCCESS;
}

state_t policy_new_iteration(polctx_t *c,signature_t *sig)
{
    if (gpus.new_iter != NULL){
        /* Basic support for GPUS */
        return gpus.new_iter(c,sig);
    }
    return EAR_SUCCESS;
}

static state_t policy_no_models_go_next_me(int curr_pstate,int *ready,node_freqs_t *freqs,unsigned long num_pstates)
{
    ulong *best_freq = freqs->cpu_freq;
    int next_pstate;
    if ((curr_pstate<(num_pstates-1))){
        *ready=EAR_POLICY_TRY_AGAIN;
        next_pstate=curr_pstate+1;
    }else{
        *ready=EAR_POLICY_READY;
        next_pstate=curr_pstate;
    }
    *best_freq=frequency_pstate_to_freq(next_pstate);
    return EAR_SUCCESS;
}

static uint policy_no_models_is_better_min_energy(signature_t * curr_sig, signature_t *prev_sig, double time_ref,
        double cpi_ref, double gbs_ref, double penalty_th)
{
    double curr_energy, pre_energy;

    curr_energy=curr_sig->time*curr_sig->DC_power;
    pre_energy=prev_sig->time*prev_sig->DC_power;

    if (curr_energy>pre_energy){
        return 0;
    }

#if 0
    if (curr_sig->time<=time_max) return 1;
    return 0;
#endif

    double time_curr = curr_sig->time;
    double cpi_curr = curr_sig->CPI;
    double gbs_curr = curr_sig->GBS;
    return (1 - above_max_penalty(time_ref, time_curr, cpi_ref, cpi_curr, gbs_ref, gbs_curr, penalty_th));
}
