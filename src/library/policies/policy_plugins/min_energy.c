/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
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
#include <library/loader/module_mpi.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/generic.h>

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define FREQ_DEF(f) (!ext_def_freq ? f : ext_def_freq)
#else
#define FREQ_DEF(f) f
#endif

#define SELECT_CPUFREQ 100
#define SELECT_IMCFREQ 101
#define COMP_IMCREF 102
#define TRY_TURBO_CPUFREQ 103

#define EXTRA_TH 0.05

/* IMC management */
extern uint dyn_unc;
extern double imc_extra_th;
extern uint *imc_max_pstate, *imc_min_pstate;
extern uint imc_num_pstates, imc_devices;
static uint imc_set_by_hw = 1;
extern uint max_policy_imcfreq_ps;
extern int lib_shared_lock_fd;

extern uint cpu_ready, gpu_ready;
// extern gpu_state_t curr_gpu_state;
extern uint last_earl_phase_classification;
extern ear_classify_t phases_limits;

static int min_pstate;
static uint ref_imc_pstate;
static uint last_imc_pstate;

static imc_data_t *imc_data;

/*  Frequency management */
static uint last_cpu_pstate;
static node_freqs_t min_energy_def_freqs;
static node_freqs_t last_nodefreq_sel;
extern ulong *cpufreq_diff;

static uint num_processes;
static ulong nominal_node;
static node_freq_domain_t me_freq_dom;

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

/* Config */
extern uint enable_load_balance;
extern uint use_turbo_for_cp;
extern uint try_turbo_enabled;
extern uint use_energy_models;

extern uint policy_cpu_bound;
extern uint policy_mem_bound;

static uint network_use_imc = 1; /*!< This variable controls the UFS when the application is communication intensive, set to 1 means UFS will not be applied. */

#if MPI_OPTIMIZED
extern sem_t *lib_shared_lock_sem;
extern uint ear_mpi_opt;
#endif

/*  Policy/Other */
static uint min_energy_state = SELECT_CPUFREQ;
static int min_energy_readiness = !EAR_POLICY_READY;
static signature_t *my_app;

// No energy models extra vars
static signature_t *sig_list;
static uint *sig_ready;

static polsym_t gpu_plugin;
static const ulong **gpuf_pol_list;
static const uint *gpuf_pol_list_items;

static signature_t cpufreq_signature;

static uint first_time = 1;

static topology_t me_top;

static state_t int_policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *last_sig, int *ok);

static state_t policy_no_models_go_next_me(int curr_pstate, int *ready, node_freqs_t *freqs, ulong num_pstates);

static uint policy_no_models_is_better_min_energy(signature_t *curr_sig, signature_t *prev_sig, double time_ref, double cpi_ref, double gbs_ref, double penalty_th);

static void policy_end_summary(int verb_lvl);

static state_t policy_mpi_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);

static state_t policy_mpi_end_optimize(node_freqs_t *freqs, int *process_id);

static state_t create_policy_domain(polctx_t *c, node_freq_domain_t *domain);

static energy_model_t cpu_energy_model;

state_t policy_init(polctx_t *c)
{
    if (c == NULL)
    {
        return EAR_ERROR;
    }

    create_policy_domain(c, &me_freq_dom);

    char *cnetwork_use_imc = ear_getenv(FLAG_NTWRK_IMC);
    char *cimc_set_by_hw = ear_getenv(FLAG_LET_HW_IMC);

    if (use_energy_models)
    {
      cpu_energy_model = energy_model_load_cpu_model(c->app, &arch_desc);

        if (!cpu_energy_model)
        {
            error_lib("Loading energy model");
            use_energy_models = 0;
        }
        else
        {
            if (!energy_model_any_projection_available(cpu_energy_model))
            {
                error_lib("Not all projections available");
                use_energy_models = 0;
            }
        }
    }
    else
    {
        cpu_energy_model = NULL;
    }
    verbose_master(2, "%sMIN ENERGY%s Using energy models: %u", COL_BLU, COL_CLR, use_energy_models);

    int i, sid = 0;

    topology_init(&me_top);

    debug("min_energy init");

    if (module_mpi_is_enabled())
    {
        mpi_app_init(c);

        uint block_type;
        is_blocking_busy_waiting(&block_type);

        verbose_master(2, "Busy waiting MPI: %u", block_type == BUSY_WAITING_BLOCK);
    }

    /*  Config env variables */
    if (cimc_set_by_hw != NULL)
        imc_set_by_hw = atoi(cimc_set_by_hw);
    if (cnetwork_use_imc != NULL)
        network_use_imc = atoi(cnetwork_use_imc);
    verbose_master(2, "Dynamic Uncore freq. management: %d\n      Using HW selection as ref: %d\n              Extra IMC penalty: %.2lf",
                   dyn_unc, imc_set_by_hw, imc_extra_th);
    verbose_master(2, "               Network uses IMC: %u", network_use_imc);

    nominal_node = frequency_get_nominal_freq();
    verbose_master(3, "Nominal CPU freq. is %lu kHz", nominal_node);

    // MPI optimization features setting up can go here...

    my_app = calloc(1, sizeof(signature_t));
    my_app->sig_ext = (void *)calloc(1, sizeof(sig_ext_t));

    /* We store the signatures for the case where models are not used */
    sig_list = (signature_t *)calloc(c->num_pstates, sizeof(signature_t));
    sig_ready = (uint *)calloc(c->num_pstates, sizeof(uint));

    if ((sig_list == NULL) || (sig_ready == NULL))
    {
        return EAR_ERROR;
    }

    imc_data = calloc(c->num_pstates * NUM_UNC_FREQ, sizeof(imc_data_t));

    num_processes = lib_shared_region->num_processes;

    percs_mpi = calloc(num_processes, sizeof(double));
    last_percs_mpi = calloc(num_processes, sizeof(double));

    critical_path = calloc(num_processes, sizeof(uint));
    last_critical_path = calloc(num_processes, sizeof(uint));

    node_freqs_alloc(&min_energy_def_freqs);
    node_freqs_alloc(&last_nodefreq_sel);

    last_mpi_stats_perc_mpi_sd = last_mpi_stats_perc_mpi_mean =
        last_mpi_stats_perc_mpi_mag = last_mpi_stats_perc_mpi_median = 0;

    /* Configuring default freqs */
    for (i = 0; i < num_processes; i++)
    {
        min_energy_def_freqs.cpu_freq[i] = *(c->ear_frequency);
    }

    for (sid = 0; sid < imc_devices; sid++)
    {
        min_energy_def_freqs.imc_freq[sid * IMC_VAL + IMC_MAX] = imc_min_pstate[sid];
        min_energy_def_freqs.imc_freq[sid * IMC_VAL + IMC_MIN] = imc_max_pstate[sid];
    }

#if USE_GPUS
    memset(&gpu_plugin, 0, sizeof(gpu_plugin));
    if (c->num_gpus)
    {
        if (policy_gpu_load(c->app, &gpu_plugin) != EAR_SUCCESS)
        {
            verbose_master(2, "Error loading GPU policy");
        }
        verbose_master(2, "Initialzing GPU policy part");
        if (gpu_plugin.init != NULL)
            gpu_plugin.init(c);

        gpuf_pol_list = (const ulong **)metrics_gpus_get(MGT_GPU)->avail_list;
        gpuf_pol_list_items = (const uint *)metrics_gpus_get(MGT_GPU)->avail_count;

        /* replace by default settings in GPU policy */
        for (i = 0; i < c->num_gpus; i++)
        {
            min_energy_def_freqs.gpu_freq[i] = gpuf_pol_list[i][0];
        }
    }
    verbose_node_freqs(3, &min_energy_def_freqs);
#endif

    node_freqs_copy(&last_nodefreq_sel, &min_energy_def_freqs);
    debug("min_energy init ends");

    return EAR_SUCCESS;
}

state_t policy_end(polctx_t *c)
{
    policy_end_summary(2); // Summary of optimization

    if (use_energy_models)
    {
        energy_model_dispose(cpu_energy_model);
    }

    if (c != NULL)
    {
        return mpi_app_end(c);
    }

    return EAR_ERROR;
}

state_t policy_loop_init(polctx_t *c, loop_id_t *l)
{
    if (c != NULL)
    {
        // projection_reset(c->num_pstates);

        if (!use_energy_models)
        {
            memset(sig_ready, 0, sizeof(uint) * c->num_pstates);
        }

        return EAR_SUCCESS;
    }

    return EAR_ERROR;
}


state_t policy_apply(polctx_t *c, signature_t *sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq = freqs->cpu_freq; // Output freqs

    int gready;   // GPU ready
    uint sid = 0; // socket id

    /*  Used by policy */
    double power_ref, time_ref, cpi_ref, gbs_ref;
    double max_penalty, local_penalty, base_penalty;

    ulong best_freq;
    ulong curr_freq, curr_imc_freq, curr_pstate;
    ulong def_pstate, def_freq;
    ulong prev_pstate;

    uint cbound, mbound;

    uint curr_imc_pstate;
    uint eUFS = dyn_unc;

    uint turbo_set = 0;

    sig_ext_t *se = (sig_ext_t *)sig->sig_ext;
    my_node_mpi_calls = se->mpi_stats;

    // TODO: Can we assign this values calling some function who return values based on current architecture?
    ulong max_cpufreq_sel = 0;
    ulong min_cpufreq_sel = 10000000;
    min_energy_readiness = EAR_POLICY_CONTINUE;

    if ((c == NULL) || (c->app == NULL))
    {
        *ready = EAR_POLICY_CONTINUE;
        return EAR_ERROR;
    }

    verbose_master(2, "%sCPU COMP phase%s", COL_BLU, COL_CLR);

    verbose_master(2, "%sMin_energy_to_solution...........starts.......%s", COL_GRE, COL_CLR);

    /* GPU Policy : It's independent of the CPU and IMC frequency selection */
    if (gpu_plugin.node_policy_apply != NULL)
    {
        // Basic support for GPUs
        gpu_plugin.node_policy_apply(c, sig, freqs, &gready);
        gpu_ready = gready;
    }
    else
    {
        gpu_ready = EAR_POLICY_READY;
    }

    /* This use case applies when IO bound for example */
    if (cpu_ready == EAR_POLICY_READY)
    {
        return EAR_SUCCESS;
    }

    max_penalty = c->app->settings[0];
    base_penalty = max_penalty;

    if (c->use_turbo)
    {
        min_pstate = 0;
    }
    else
    {
        min_pstate = frequency_closest_pstate(c->app->max_freq); // TODO: Migrate to the new API
    }

    // Default values
    def_freq = FREQ_DEF(c->app->def_freq);
    def_pstate = frequency_closest_pstate(def_freq);

    // This is the frequency at which we were running
    // sig is the job signature adapted to the node for the energy models
    signature_copy(my_app, sig);
    memcpy(my_app->sig_ext, se, sizeof(sig_ext_t));

    if (c->pc_limit > 0)
    {
        verbose_master(2, "Powercap node limit set to %u", c->pc_limit);
        curr_freq = frequency_closest_high_freq(my_app->avg_f, 1);
    }
    else
    {
        curr_freq = *(c->ear_frequency);
    }

    curr_pstate = frequency_closest_pstate(curr_freq);

    // Added for the use case where energy models are not used
    if (!use_energy_models)
    {
        sig_ready[curr_pstate] = 1;
        signature_copy(&sig_list[curr_pstate], sig);
    }

    /* We save data in imc_data structure for policy comparison.
     * We are computing the imc_pstate based on the avg_imc_freq, no looking at selection. */

    pstate_t tmp_pstate;
    curr_imc_freq = avg_to_khz(my_app->avg_imc_f);

    if (state_fail(pstate_freqtops_upper((pstate_t *)imc_pstates, imc_num_pstates,
                                         curr_imc_freq, &tmp_pstate)))
    {
        verbose_master(2, "%sWarning%s Current Avg IMC freq. %lu could not be converted to psate. %llu was set.",
                       COL_YLW, COL_CLR, curr_imc_freq, tmp_pstate.khz);
    }

    curr_imc_pstate = tmp_pstate.idx;
    if (eUFS)
    {
        debug("CPU pstate %lu IMC freq %lu IMC pstate %u", curr_pstate, my_app->avg_imc_f, curr_imc_pstate);
        copy_imc_data_from_signature(imc_data, curr_pstate, curr_imc_pstate, my_app);
    }

    int policy_stable;

    /* This function computes the load balance, the critical path etc */
    int_policy_ok(c, sig, &cpufreq_signature, &policy_stable);
    debug("Policy ok returns %d", policy_stable);

    /* If there is some change in the Signature, we apply default values and computes again the signature */

    /* If we are not stable, we start again */
    if ((min_energy_state != SELECT_CPUFREQ) && !policy_stable)
    {
        debug("%spolicy not stable%s", COL_RED, COL_CLR);
        min_energy_state = SELECT_CPUFREQ;
        // TODO: we could select another phase here if we have the information
        last_earl_phase_classification = APP_COMP_BOUND;
    }

    /* STEP 1: CPU frequency selection phase */
    if ((min_energy_state == SELECT_CPUFREQ) || (eUFS == 0))
    {
        /**** SELECT_CPUFREQ ****/
        verbose_master(2, "%sSelecting CPU frequency %s. eUFS %u", COL_BLU, COL_CLR, eUFS);

        last_cpu_pstate = curr_pstate;
        last_imc_pstate = curr_imc_pstate;

        /* Classification */
        //  NEW CLASSIFY
        // is_cpu_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &cbound);
        // is_mem_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &mbound);
        /* Computed in policy */
        cbound = policy_cpu_bound;
        mbound = policy_mem_bound;

        verbose_master(2, "App CPU bound %u memory bound %u", cbound, mbound);

        // If is not the default P_STATE selected in the environment, a projection
        // is made for the reference P_STATE in case the coefficents were available.
        // NEW: This code is new because projections from lower pstates are reporting bad cpu freqs
        if (use_energy_models && !are_default_settings(&last_nodefreq_sel, &min_energy_def_freqs))
        {
            verbose_node_freqs(3, &last_nodefreq_sel);
            verbose_node_freqs(3, &min_energy_def_freqs);

            verbose_master(2, "Setting default conf for ME");

            set_default_settings(freqs, &min_energy_def_freqs);
            if (gpu_plugin.restore_settings != NULL)
            {
                gpu_plugin.restore_settings(c, sig, freqs);
            }

            node_freqs_copy(&last_nodefreq_sel, freqs);

            *ready = EAR_POLICY_TRY_AGAIN;

            return EAR_SUCCESS;
        }
        verbose_master(2, "Min_energy per process set to %u", freq_per_core);

        // Here we are running at the default freqs.
        for (uint i = 0; i < nump; i++)
        {
            verbose_master(3, "Proc. %d - Freq. per core: %d", i, freq_per_core);

            /* This variable is set internally  by int_policy_ok function. It depends on the load balance of the application */
            local_penalty = base_penalty;
            if (freq_per_core)
            {
                /* 1.2: Per process CPU frequency selection */
                ulong eff_p_cpuf, process_avgcpuf, cpus_cnt_ret;

                pstate_freqtoavg(sig_shared_region[i].cpu_mask, lib_shared_region->avg_cpufreq,
                                 metrics_get(MET_CPUFREQ)->devs_count, &process_avgcpuf, &cpus_cnt_ret);

                debug("Process %d: avg. cpufreq %lu num cpus %lu", i, process_avgcpuf, cpus_cnt_ret);

                signature_from_ssig(my_app, &sig_shared_region[i].sig);

                double lgbs, lpower;

                lgbs = my_app->GBS;
                lpower = my_app->DC_power;

                my_app->GBS = lgbs * num_processes;
                my_app->DC_power = lpower * num_processes;

                /* Critical path is computed in int_policy_ok */
                if (critical_path[i] == 1)
                {
                    // Process was selected to be in the critical path
                    if (cbound)
                    {
                        /* If the process is part of the critical path and is CPU bound */
                        eff_p_cpuf = frequency_closest_high_freq(process_avgcpuf, 0);
                        if (eff_p_cpuf < process_avgcpuf)
                        {
                            eff_p_cpuf = frequency_closest_high_freq(process_avgcpuf + 100000, 0);
                        }
                        if (use_turbo_for_cp)
                        {
                            /* 1.2.1: Use turbo for critial path process */
                            new_freq[i] = frequency_pstate_to_freq(0);
                        }
                        else
                        {
                            /* 1.2.2: Se select the maximum CPU freq or the maximum avg CPU freq (avx512 case) */
                            new_freq[i] = ear_min(eff_p_cpuf, frequency_pstate_to_freq(1));
                        }
                        verbose_master(2, "%sCPU freq for CP process %d is %lu (!MBOUND use_turbo %u MIN_MPI %u)%s",
                                       COL_RED, i, new_freq[i], use_turbo_for_cp, percs_mpi[i] == percs_mpi[min_mpi], COL_CLR);

                        continue;
                    }
                    else
                    {
                        /* The process if part of the critical path BUT is memory bound. We increase the penalty supported */
                        local_penalty = base_penalty / 2;
                        ;
                    }
                }
                else if (critical_path[i] == 2)
                {
                    /* Lot of MPI: Not used */
                    local_penalty = 0.5;
                }
                else
                {
                    /* Mix process : A per-process penalty is computed */
                    //local_penalty = base_penalty + EXTRA_TH * (my_node_mpi_calls[i].perc_mpi - my_node_mpi_calls[min_mpi].perc_mpi) / 10.0;
                    local_penalty = base_penalty ;
                }
            }
            if (use_energy_models)
            {
                /* If energy models are availables, we use them for CPU frequency selection. */
                curr_freq = def_freq;
                curr_pstate = def_pstate;
                compute_reference(my_app, cpu_energy_model, &curr_freq, &def_freq, &best_freq, &time_ref, &power_ref);
                verbose_master(3, "Time ref %lf Power ref %lf Freq ref %lu", time_ref, power_ref, best_freq);
								//verbose_master(3, "Min energy: cur pstate %u min pstate %u max pstate %u", curr_pstate, min_pstate, c->num_pstates);
                compute_cpu_freq_min_energy(my_app, cpu_energy_model, best_freq, time_ref, power_ref, local_penalty, curr_pstate, min_pstate, c->num_pstates, &new_freq[i]);
                if (freq_per_core)
                {
                    verbose_master(2, "PROC[%u][%u] CPI %.2f TPI %.2f GFlops %.2f Power %.2f PMPI %.2f penalty %.2lf", MASTER_ID, i,
                                   sig_shared_region[i].sig.CPI, sig_shared_region[i].sig.TPI, sig_shared_region[i].sig.Gflops, sig_shared_region[i].sig.DC_power,
                                   my_node_mpi_calls[i].perc_mpi, local_penalty);
                }
                else
                {
                    verbose_master(2, "APP[%u][%u] CPI %.2f GBS %.2lf TPI %.2f GFlops %.2f Power %.2f  penalty %.2lf", MASTER_ID, i,
                                   my_app->CPI, my_app->GBS, my_app->TPI, my_app->Gflops, my_app->DC_power, local_penalty);
                }
            }
            else
            {
                // If application is compute bound, we don't reduce CPU freq
                if (cbound)
                {
                    *new_freq = frequency_pstate_to_freq(def_pstate);
                    *ready = EAR_POLICY_READY;
                    debug("Application cbound. Next freq %lu Next state ready %d", *new_freq, *ready);
                }
                else
                {
                    /* If energy models are not available, we just execute a linear search */
                    if (sig_ready[def_pstate] == 0)
                    {
                        *new_freq = def_freq;
                        *ready = EAR_POLICY_TRY_AGAIN;
                    }
                    else
                    {
                        signature_t *prev_sig;
                        time_ref = sig_list[def_pstate].time;
                        cpi_ref = sig_list[def_pstate].CPI;
                        gbs_ref = sig_list[def_pstate].GBS;
                        // time_max = time_ref * (1 + base_penalty);
                        /* This is the normal case */
                        if (curr_pstate != def_pstate)
                        {
                            prev_pstate = curr_pstate - 1;
                            prev_sig = &sig_list[prev_pstate];
                            if (policy_no_models_is_better_min_energy(my_app, prev_sig, time_ref,
                                                                      cpi_ref, gbs_ref, base_penalty))
                            {
                                policy_no_models_go_next_me(curr_pstate, ready, freqs, c->num_pstates);
                            }
                            else
                            {
                                *new_freq = frequency_pstate_to_freq(prev_pstate);
                                *ready = EAR_POLICY_READY;
                            }
                        }
                        else
                        {
                            policy_no_models_go_next_me(curr_pstate, ready, freqs, c->num_pstates);
                        }
                    }
                }
            }

            verbose_master(2, "CPU freq for process %d is %lu", i, new_freq[i]);
            max_cpufreq_sel = ear_max(max_cpufreq_sel, new_freq[i]);
            min_cpufreq_sel = ear_min(min_cpufreq_sel, new_freq[i]);
        }

#if MPI_OPTIMIZED
        // You can fill here the mpi_freq field of the sig_shared_region if you are optimizing
        // the application at MPI call level.
        for (uint lp = 0; lp < num_processes; lp++)
        {
            ulong f = new_freq[0];
            if (freq_per_core)
            {
                f = new_freq[lp];
            }
            sig_shared_region[lp].mpi_freq = f;
        }
#endif

        /* If we use the same CPU freq for all cores, we set for all processes */
        if (!freq_per_core)
        {
            set_all_cores(new_freq, num_processes, new_freq[0]);
        }

        /* We check if turbo is an option */
        if ((min_cpufreq_sel == nominal_node) && cbound && try_turbo_enabled)
        {
            turbo_set = 1;
            verbose_master(2, "Using turbo because it is cbound and turbo enabled");
            set_all_cores(new_freq, num_processes, frequency_pstate_to_freq(0));
        }

        copy_cpufreq_sel(last_nodefreq_sel.cpu_freq, new_freq, sizeof(ulong) * num_processes);

        /* You can copy here in shared memory to be used later in MPI init/end if needed */

        /* We will use this signature to detect changes in the sig compared when the one used to compute the CPU freq */
        verbose_master(3, "Updating cpufreq_signature");

        se = (sig_ext_t *)sig->sig_ext;
        signature_copy(&cpufreq_signature, sig);
        memcpy(cpufreq_signature.sig_ext, se, sizeof(sig_ext_t));

        /* Phase 2:  IMC freq selection */
        if ((use_energy_models || *ready == EAR_POLICY_READY) &&
            eUFS && !((earl_phase_classification == APP_MPI_BOUND) && network_use_imc))
        {

            /* If we are compute bound, and CPU freq is nominal, we can reduce the IMC freq
             * IMC_MAX means the maximum frequency, lower pstate,
             * IMC_MIN means minimum frequency and maximum pstate */
            if ((min_cpufreq_sel == nominal_node) && cbound)
            {
                uint lowm;
                low_mem_activity(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &lowm);
                if (lowm)
                {
                    debug("Low memory activity");
                    for (sid = 0; sid < imc_devices; sid++)
                    {
                        freqs->imc_freq[sid * IMC_VAL + IMC_MAX] = imc_max_pstate[sid] - 1;
                    }
                }
                else
                {
                    debug("GBS GT GBS_BUSY_WAITING selecting the 0.25 pstate...");
                    for (sid = 0; sid < imc_devices; sid++)
                    {
                        freqs->imc_freq[sid * IMC_VAL + IMC_MAX] = select_imc_pstate(imc_num_pstates, 0.25);
                    }
                }
            }
            else
            {
                debug("min selected CPU freq %lu - nominal CPU freq %lu - cbound %u",
                      min_cpufreq_sel, nominal_node, cbound);
                for (sid = 0; sid < imc_devices; sid++)
                {
                    freqs->imc_freq[sid * IMC_VAL + IMC_MAX] = imc_min_pstate[sid];
                }
            }
            // ps_nothing means we will not change it. We could use the imc_max_pstate
            if (imc_set_by_hw)
            {
                for (sid = 0; sid < imc_devices; sid++)
                    freqs->imc_freq[sid * IMC_VAL + IMC_MIN] = imc_max_pstate[sid];
            }
            else
            {
                for (sid = 0; sid < imc_devices; sid++)
                    freqs->imc_freq[sid * IMC_VAL + IMC_MIN] =
                        ear_min(freqs->imc_freq[sid * IMC_VAL + IMC_MAX] + 1, imc_num_pstates - 1);
            }
            sid = 0;
            debug("%sIMC freq selection%s %lu-%lu", COL_GRE, COL_CLR,
                  imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MAX]].khz,
                  imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MIN]].khz);
        }
        else
        {
            verbose_master(3,
                           "%sNot IMC freq selection%s : using energy models %u eUFS %u MPI bound = %u Network_IMC %u",
                           COL_RED, COL_CLR, use_energy_models, eUFS, earl_phase_classification == APP_MPI_BOUND, network_use_imc);
            /* If application is network bound, we don't reduce the uncore frequency */
            if ((use_energy_models || *ready == EAR_POLICY_READY) &&
                eUFS && (earl_phase_classification == APP_MPI_BOUND) && network_use_imc)
            {
                eUFS = 0;
                for (sid = 0; sid < imc_devices; sid++)
                {
                    freqs->imc_freq[sid * IMC_VAL + IMC_MAX] = imc_min_pstate[sid];
                    freqs->imc_freq[sid * IMC_VAL + IMC_MIN] = imc_min_pstate[sid] + 1;
                }
            }
        }

        /* Next state selection */
        if (turbo_set)
        {
            // TODO: what happen then if user does not activate IMC controller? The policy will also go to SELECT_IMCFREQ state
            min_energy_state = TRY_TURBO_CPUFREQ;
            *ready = EAR_POLICY_TRY_AGAIN;
        }
        else
        {
            if (!eUFS)
            {
                /* If we are not using energy models, `ready` has been set by `no_models` policy */
                if (use_energy_models)
                {
                    *ready = EAR_POLICY_READY;
                }
                min_energy_state = SELECT_CPUFREQ;
            }
            else
            {
                /* if we are using `no_models` policy, it tells us whether is ready */
                if (use_energy_models || *ready == EAR_POLICY_READY)
                {
                    uint i = 0;
                    while ((i < num_processes) && (last_nodefreq_sel.cpu_freq[i] == def_freq))
                        i++;
                    // TODO: why we check here for cbound?
                    if (i < num_processes || cbound)
                    {
                        min_energy_state = COMP_IMCREF;
                    }
                    else
                    {
                        ref_imc_pstate = curr_imc_pstate;
                        min_energy_state = SELECT_IMCFREQ;
                    }
                    *ready = EAR_POLICY_TRY_AGAIN;
                }
                else
                {
                    min_energy_state = SELECT_CPUFREQ;
                }
            }
        }
        memcpy(last_nodefreq_sel.imc_freq, freqs->imc_freq, imc_devices * IMC_VAL * sizeof(ulong));
    }
    else if ((min_energy_state == COMP_IMCREF) || (min_energy_state == TRY_TURBO_CPUFREQ))
    {
        /**** COMP_IMCREF ***/
        ref_imc_pstate = curr_imc_pstate;
        for (sid = 0; sid < imc_devices; sid++)
        {
            freqs->imc_freq[sid * IMC_VAL + IMC_MAX] = curr_imc_pstate;
            freqs->imc_freq[sid * IMC_VAL + IMC_MIN] = imc_max_pstate[sid];
        }
        sid = 0;
        verbose_master(2, "%sComputing reference%s cpufreq %lu imc %llu-%llu", COL_BLU, COL_CLR, new_freq[0],
                       imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MAX]].khz,
                       imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MIN]].khz);

        min_energy_state = SELECT_IMCFREQ;
        *ready = EAR_POLICY_TRY_AGAIN;

        memcpy(freqs->cpu_freq, last_nodefreq_sel.cpu_freq, sizeof(ulong) * num_processes);
        memcpy(last_nodefreq_sel.imc_freq, freqs->imc_freq, imc_devices * IMC_VAL * sizeof(ulong));
    }
    else if ((min_energy_state == SELECT_IMCFREQ) && eUFS)
    {
        /**** IMC_FREQ ***/
        verbose_master(2, "%sSelecting IMC freq%s: nominal %d - last CPU %u,IMC %u -- current CPU %lu, IMC %u",
                       COL_BLU, COL_CLR, min_pstate, last_cpu_pstate, last_imc_pstate, curr_pstate, curr_imc_pstate);

        uint increase_imc = must_increase_imc(imc_data, curr_pstate, curr_imc_pstate, curr_pstate,
                                              ref_imc_pstate, my_app, imc_extra_th);

        /* If we are over the limit, we increase the IMC */
        if (increase_imc)
        {
            uint must_start_again = must_start(imc_data, curr_pstate, curr_imc_pstate, curr_pstate, ref_imc_pstate, my_app);
            debug("%sWarning, passing the imc_th limit: start_again %u", COL_RED, must_start_again);
            for (sid = 0; sid < imc_devices; sid++)
            {
                freqs->imc_freq[sid * IMC_VAL + IMC_MAX] = ((freqs->imc_freq[sid * IMC_VAL + IMC_MAX] > 0) ? (freqs->imc_freq[sid * IMC_VAL + IMC_MAX] - 1) : 0);
                freqs->imc_freq[sid * IMC_VAL + IMC_MIN] = imc_max_pstate[sid];
            }
            sid = 0;
            verbose_master(2, "Selecting IMC range %llu - %llu%s",
                           imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MAX]].khz,
                           imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MIN]].khz, COL_CLR);

            if (!must_start_again)
                *ready = EAR_POLICY_READY;
            else
                *ready = EAR_POLICY_TRY_AGAIN;

            min_energy_state = SELECT_CPUFREQ;
        }
        else
        {
            /* IMC_MAX is max frequency, lower bound p-state */
            /* IMC_MIN is min frequency, upper bound p-state */
            for (sid = 0; sid < imc_devices; sid++)
            {
                // Lower bound IMC p-state index
                int min_ps_idx = sid * IMC_VAL + IMC_MAX;
                // The lower bound IMC p-state won't be greather than the maximum
                // retrieved by hardware.
                // freqs->imc_freq[min_ps_idx] = ear_min(imc_max_pstate[sid],
                //                                       freqs->imc_freq[min_ps_idx] + 1);
                freqs->imc_freq[min_ps_idx] = ear_min(imc_max_pstate[sid],
                                                      curr_imc_pstate + 1);

                // Lower bound IMC p-state index
                int max_ps_idx = sid * IMC_VAL + IMC_MIN;
                freqs->imc_freq[max_ps_idx] = imc_max_pstate[sid];
            }

            sid = 0; // We are assuming all sockets use the same frequency range.

            debug("%sTrying a lower IMC freq %llu-%llu%s", COL_GRE,
                  imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MAX]].khz,
                  imc_pstates[freqs->imc_freq[sid * IMC_VAL + IMC_MIN]].khz, COL_CLR);

            // Check whether the selected p-state is not greather than the maximum configured.
            int selected_ps_leq_max = freqs->imc_freq[sid * IMC_VAL + IMC_MAX] <= max_policy_imcfreq_ps;

            // The maximum p-state permitted is equal to the maximum p-state accepted by the device.
            int max_ps_config_eq_dev = imc_min_pstate[sid] == max_policy_imcfreq_ps;

            // Check whether the selected p-state is different than the selected one in the last iteration.
            // This is needed on cases where the maximum configured is equal to the maximum of the socket,
            // otherwise the policy would try again and again to increase to a p-state that she thinks it is valid.
            int selected_ps_eq_last = freqs->imc_freq[sid * IMC_VAL + IMC_MAX] == curr_imc_pstate;

            // The selected p-state is the minimum permitted by the device.
            int selected_ps_eq_minfreq = freqs->imc_freq[sid * IMC_VAL + IMC_MAX] == imc_max_pstate[sid];

            if (selected_ps_leq_max && !(max_ps_config_eq_dev && selected_ps_eq_minfreq && selected_ps_eq_last))
            {
                *ready = EAR_POLICY_TRY_AGAIN;
            }
            else
            {
                *ready = EAR_POLICY_READY;
                min_energy_state = SELECT_CPUFREQ;
            }
        }
        memcpy(freqs->cpu_freq, last_nodefreq_sel.cpu_freq, sizeof(ulong) * num_processes);
        memcpy(last_nodefreq_sel.imc_freq, freqs->imc_freq, imc_devices * IMC_VAL * sizeof(ulong));
    }
    verbose_master(2, "Next min_energy state %u next policy state %u", min_energy_state, *ready);
    min_energy_readiness = *ready;

    if (cpu_ready && use_energy_models)
    {
        compute_policy_savings(cpu_energy_model, sig, freqs, &me_freq_dom);
    }

    return EAR_SUCCESS;
}

state_t policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *last_sig, int *ok)
{
    *ok = 0;
    if ((c == NULL) || (curr_sig == NULL) || (last_sig == NULL))
    {
        return EAR_ERROR;
    }
    return int_policy_ok(c, curr_sig, last_sig, /*freqs, */ ok);
}

state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set, signature_t *s)
{
    if (c != NULL)
    {
        node_freqs_copy(freq_set, &min_energy_def_freqs);
        // WARNING: A function designed as a getter calls a function designed as a setter.
        if ((gpu_plugin.restore_settings != NULL) && (s != NULL))
        {
            gpu_plugin.restore_settings(c, s, freq_set);
        }
    }
    else
    {
        return EAR_ERROR;
    }

    return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c, int *intents)
{
    *intents = 1;
    return EAR_SUCCESS;
}

state_t policy_domain(polctx_t *c, node_freq_domain_t *domain)
{
    return create_policy_domain(c, domain);
}

state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (c != NULL)
    {
        state_t st = mpi_call_init(c, call_type);
        policy_mpi_init_optimize(c, call_type, freqs, process_id);
        return st;
    }
    else
    {
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
    else
        return EAR_ERROR;
}

state_t policy_cpu_gpu_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs)
{
    if (c == NULL)
        return EAR_ERROR;
    verbose_master(2, "%sCPU-GPU  phase%s", COL_BLU, COL_CLR);
    cpu_ready = EAR_POLICY_READY;
    ulong def_freq = FREQ_DEF(c->app->def_freq);
    uint def_pstate = frequency_closest_pstate(def_freq);
    for (uint i = 0; i < num_processes; i++)
        freqs->cpu_freq[i] = frequency_pstate_to_freq(def_pstate);
    memcpy(last_nodefreq_sel.cpu_freq, freqs->cpu_freq, MAX_CPUS_SUPPORTED * sizeof(ulong));
    if (dyn_unc)
    {
        for (uint sock_id = 0; sock_id < arch_desc.top.socket_count; sock_id++)
        {
            freqs->imc_freq[sock_id * IMC_VAL + IMC_MAX] = imc_min_pstate[sock_id];
            freqs->imc_freq[sock_id * IMC_VAL + IMC_MIN] = imc_min_pstate[sock_id];
        }
        memcpy(last_nodefreq_sel.imc_freq, freqs->imc_freq, imc_devices * IMC_VAL * sizeof(ulong));
    }
#if USE_GPUS
    if (gpu_plugin.cpu_gpu_settings != NULL)
    {
        return gpu_plugin.cpu_gpu_settings(c, my_sig, freqs);
    }
    else
    {
        /* GPU mem is pending */
        for (uint i = 0; i < c->num_gpus; i++)
        {
            freqs->gpu_freq[i] = min_energy_def_freqs.gpu_freq[i];
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
    if (c == NULL)
        return EAR_ERROR;

    const uint min_ener_io_veb_lvl = 2; // Used to control the verbose level in this function

    verbose_master(min_ener_io_veb_lvl, "%sMIN ENERGY%s CPU I/O settings", COL_BLU, COL_CLR);

    cpu_ready = EAR_POLICY_READY;

    for (uint i = 0; i < num_processes; i++)
    {
        freqs->cpu_freq[i] = min_energy_def_freqs.cpu_freq[i];

        signature_t local_sig;
        signature_from_ssig(&local_sig, &sig_shared_region[i].sig);

        uint io_bound;
        is_io_bound(&local_sig, sig_shared_region[i].num_cpus, &io_bound);

        uint busy_waiting;

        if (!io_bound)
        {
            is_process_busy_waiting(&sig_shared_region[i].sig, &busy_waiting); // Julita
            if (busy_waiting)
            {
                freqs->cpu_freq[i] = frequency_pstate_to_freq(c->num_pstates - 1);
            }
        }
        else if (me_top.vendor == VENDOR_INTEL)
        {
            freqs->cpu_freq[i] = frequency_pstate_to_freq(c->num_pstates - 1);
        }

        char ssig_buff[256];
        ssig_tostr(&sig_shared_region[i].sig, ssig_buff, sizeof(ssig_buff));

        verbose_master(min_ener_io_veb_lvl, "%s.[%d] [I/O config: %u, I/O busy waiting config: %u] %s",
                       node_name, i, io_bound, busy_waiting, ssig_buff);

        /* We copy in shared memory to be used later in MPI init/end if needed. */
        sig_shared_region[i].new_freq = freqs->cpu_freq[i];
    }

    memcpy(last_nodefreq_sel.cpu_freq, freqs->cpu_freq, MAX_CPUS_SUPPORTED * sizeof(ulong));

    if (dyn_unc)
    {
        for (uint sock_id = 0; sock_id < arch_desc.top.socket_count; sock_id++)
        {
            freqs->imc_freq[sock_id * IMC_VAL + IMC_MAX] = imc_min_pstate[sock_id];
            freqs->imc_freq[sock_id * IMC_VAL + IMC_MIN] = imc_min_pstate[sock_id];
        }
        memcpy(last_nodefreq_sel.imc_freq, freqs->imc_freq, imc_devices * IMC_VAL * sizeof(ulong));
    }

#if USE_GPUS
    if (gpu_plugin.io_settings != NULL)
    {
        return gpu_plugin.io_settings(c, my_sig, freqs);
    }
    else
    {
        gpu_ready = EAR_POLICY_READY;
    }
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif
    /* GPU mem is pending */

    return EAR_SUCCESS;
}

state_t policy_busy_wait_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs)
{
    if (c == NULL)
    {
        return EAR_ERROR;
    }

    const uint min_ener_bw_veb_lvl = 2; // Used to control the verbose level in this function

    verbose_master(min_ener_bw_veb_lvl, "%sMIN ENERGY%s CPU Busy waiting settings", COL_BLU, COL_CLR);

    cpu_ready = EAR_POLICY_READY;

    for (uint i = 0; i < num_processes; i++)
    {

        signature_t local_sig;
        signature_from_ssig(&local_sig, &sig_shared_region[i].sig);

        uint busy_waiting;
        // is_cpu_busy_waiting(&local_sig, sig_shared_region[i].num_cpus, &busy_waiting);
        is_process_busy_waiting(&sig_shared_region[i].sig, &busy_waiting);
        char ssig_buff[256];
        ssig_tostr(&sig_shared_region[i].sig, ssig_buff, sizeof(ssig_buff));
        if (busy_waiting)
        {
            verbose_master(min_ener_bw_veb_lvl, "%s.[%d] Busy waiting config %s", node_name, i, ssig_buff);
            freqs->cpu_freq[i] = frequency_pstate_to_freq(c->num_pstates - 1);
        }
        else
        {
            verbose_master(min_ener_bw_veb_lvl, "%s.[%d] NO Busy waiting config %s ", node_name, i, ssig_buff);
            freqs->cpu_freq[i] = min_energy_def_freqs.cpu_freq[i];
        }

        /* We copy in shared  memory to be used later in MPI init/end if needed */
        sig_shared_region[i].new_freq = freqs->cpu_freq[i];
    }

    copy_cpufreq_sel(last_nodefreq_sel.cpu_freq, freqs->cpu_freq, sizeof(ulong) * MAX_CPUS_SUPPORTED);
    if (dyn_unc)
    {
        for (uint sock_id = 0; sock_id < arch_desc.top.socket_count; sock_id++)
        {
            freqs->imc_freq[sock_id * IMC_VAL + IMC_MAX] = imc_max_pstate[sock_id];
            freqs->imc_freq[sock_id * IMC_VAL + IMC_MIN] = imc_max_pstate[sock_id];
        }
        memcpy(last_nodefreq_sel.imc_freq, freqs->imc_freq, imc_devices * IMC_VAL * sizeof(ulong));
    }
#if USE_GPUS
    if (gpu_plugin.busy_wait_settings != NULL)
    {
        return gpu_plugin.busy_wait_settings(c, my_sig, freqs);
    }
    else
    {
        gpu_ready = EAR_POLICY_READY;
    }
    return EAR_SUCCESS;
#else
    gpu_ready = EAR_POLICY_READY;
#endif
    /* GPU mem is pending */
    return EAR_SUCCESS;
}

state_t policy_restore_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs)
{
    /* WARNING: my_sig can be NULL */
    if (c != NULL)
    {
        state_t st = EAR_SUCCESS;
        verbose_master(2, "min_energy policy_restore_settings");
        node_freqs_copy(freqs, &min_energy_def_freqs);

        if (gpu_plugin.restore_settings != NULL)
        {
            st = gpu_plugin.restore_settings(c, my_sig, freqs);
        }
        node_freqs_copy(&last_nodefreq_sel, freqs);

        return st;
    }
    else
    {
        return EAR_ERROR;
    }
}

state_t policy_new_iteration(polctx_t *c, signature_t *sig)
{
    if (gpu_plugin.new_iter != NULL)
    {
        /* Basic support for GPUS */
        return gpu_plugin.new_iter(c, sig);
    }
    return EAR_SUCCESS;
}

static state_t int_policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *last_sig, int *ok)
{
    debug("%spolicy ok evaluation%s", COL_YLW, COL_CLR);
    *ok = 1;

    sig_ext_t *se = (sig_ext_t *)curr_sig->sig_ext;
    my_node_mpi_calls = se->mpi_stats;
    my_node_mpi_types_calls = se->mpi_types;

    /*  Check for load balance data */
    if (module_mpi_is_enabled() && enable_load_balance && use_energy_models) {

        verbose_mpi_calls_types(3, my_node_mpi_types_calls);

        uint unbalance;
        double mean, sd, mag;
        if (state_ok(mpi_support_evaluate_lb(my_node_mpi_calls, num_processes, percs_mpi,
                                             &mean, &sd, &mag, &unbalance)))
        {
            /* This function sends mpi_summary to other nodes */
            // chech_node_mpi_summary();
            /*  Check whether application has changed from unbalanced to balanced and viceversa */
            if (unbalance != signature_is_unbalance)
            {
                if (!unbalance)
                {
                    verbose_master(2, "%sApplication becomes balanced!%s", COL_GRE, COL_CLR);
                    signature_is_unbalance = 0;
                }
                else
                {
                    verbose_info("%sApplication becomes unbalanced%s", COL_RED, COL_CLR);
                    signature_is_unbalance = 1;
                    *ok = 0;
                }
            }
            for (uint i = 0; i < num_processes; i++)
            {
                sig_shared_region[i].unbalanced = 0;
                sig_shared_region[i].perc_MPI = 0;
            }

            if (unbalance)
            {
                /* These variables are global to the policy */
                freq_per_core = 1;
                nump = num_processes;

                double median;
                /*  Select critical path */
                mpi_support_select_critical_path(critical_path, percs_mpi, num_processes, mean,
                                                 &median, &max_mpi, &min_mpi);

                /* We check if policy ok because if it is 0, it means that app became unbalanced from balanced
                 * state and we don't need to check variation between policy iterations  */
                if (!first_time && *ok == 1)
                {

                    mpi_support_verbose_perc_mpi_stats(2, last_percs_mpi, num_processes,
                                                       last_mpi_stats_perc_mpi_mean, last_mpi_stats_perc_mpi_median,
                                                       last_mpi_stats_perc_mpi_sd, last_mpi_stats_perc_mpi_mag);

                    double similarity;

                    if (VERB_ON(3))
                    {
                        /*  This code has not been encapsulated because is (by now) only for verbose purposes */
                        mpi_stats_evaluate_similarity(percs_mpi, last_percs_mpi, num_processes, &similarity);
                        verbose_master(3, "(Orientation) Similarity between last and current: %lf", similarity);
                        /* *************************************************************************** */
                    }

                    uint mpi_changed;
                    mpi_support_mpi_changed(mag, last_mpi_stats_perc_mpi_mag, critical_path, last_critical_path,
                                            num_processes, &similarity, &mpi_changed);

                    if (mpi_changed)
                    {
                        verbose_info("%sMPI stats changed. Starting again%s",
                                       COL_RED, COL_CLR);
                        *ok = 0;
                    }
                }
                else if (*ok)
                {
                    first_time = 0;
                }

                /*  Save current stats and critical path to be used later */
                memcpy(last_percs_mpi, percs_mpi, sizeof(double) * num_processes);
                memcpy(last_critical_path, critical_path, sizeof(uint) * num_processes);
                last_mpi_stats_perc_mpi_sd = sd;
                last_mpi_stats_perc_mpi_mean = mean;
                last_mpi_stats_perc_mpi_mag = mag;
                last_mpi_stats_perc_mpi_median = median;

                /* This is critical path for each process */
                for (uint i = 0; i < num_processes; i++)
                {
                    sig_shared_region[i].perc_MPI = percs_mpi[i];
                    sig_shared_region[i].unbalanced = (critical_path[i] ? 0 : 1);
                }
            }
            else
            {
                nump = 1;
                freq_per_core = 0;
            }
        }
        else
        {
            nump = 1;
            freq_per_core = 0;
        }
    }
    else
    {
        nump = 1;
        freq_per_core = 0;
    }

    if ((*ok == 1) && (min_energy_state != SELECT_CPUFREQ))
    {
        /* TODO: check if application is unbalanced */
        if (use_energy_models && freq_per_core == 0 && signatures_different(curr_sig, last_sig, "min_energy", &cpu_energy_model, min_pstate))
        {
            verbose_info("%sSignature is too different from the one used for CPU freq. Starting again (based on policy_sim)%s",
                            COL_RED, COL_CLR);
            *ok = 0;
        }
        else if (!use_energy_models && default_signatures_different(last_sig, curr_sig, 0.2))
        {
            verbose_info("%sSignature is too different from the one used for CPU freq. Starting again (based on metrics)%s",
                           COL_RED, COL_CLR);
            *ok = 0;
        }
    }
    if (*ok == 0)
    {
        first_time = 1;
    }
    else
    {
        compute_energy_savings(curr_sig, last_sig);

#if USE_ENERGY_SAVING
        if (curr_sig->sig_ext->saving < 0)
        {
            *ok = 0;

            verbose_info("%smin_energy:%s energy savings less than expected. Applying again", COL_RED, COL_CLR);
        }
#endif
    }

    return EAR_SUCCESS;
}

static state_t policy_no_models_go_next_me(int curr_pstate, int *ready, node_freqs_t *freqs, unsigned long num_pstates)
{
    ulong *best_freq = freqs->cpu_freq;
    int next_pstate;
    if ((curr_pstate < (num_pstates - 1)))
    {
        *ready = EAR_POLICY_TRY_AGAIN;
        next_pstate = curr_pstate + 1;
    }
    else
    {
        *ready = EAR_POLICY_READY;
        next_pstate = curr_pstate;
    }
    *best_freq = frequency_pstate_to_freq(next_pstate);
    return EAR_SUCCESS;
}

static uint policy_no_models_is_better_min_energy(signature_t *curr_sig, signature_t *prev_sig, double time_ref,
                                                  double cpi_ref, double gbs_ref, double penalty_th)
{
    double curr_energy, pre_energy;

    curr_energy = curr_sig->time * curr_sig->DC_power;
    pre_energy = prev_sig->time * prev_sig->DC_power;

    if (curr_energy > pre_energy)
    {
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

static state_t policy_mpi_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
#if MPI_OPTIMIZED
    // You can implement optimization at MPI call entry here.

    if (ear_mpi_opt &&
        (lib_shared_region->num_processes > 1))
    {
        return EAR_SUCCESS;
    }
#endif
    return EAR_SUCCESS;
}

static void policy_end_summary(int verb_lvl)
{
#if MPI_OPTIMIZED
    // You can show a summary of your optimization at MPI call level here.
#endif
}

static state_t policy_mpi_end_optimize(node_freqs_t *freqs, int *process_id)
{
#if MPI_OPTIMIZED
    // You can implement optimization at MPI call exit here.
    if (ear_mpi_opt &&
        (lib_shared_region->num_processes > 1))
    {
    }
#endif

    return EAR_SUCCESS;
}

static state_t create_policy_domain(polctx_t *c, node_freq_domain_t *domain)
{
    if (c && domain)
    {
        domain->cpu = POL_GRAIN_CORE;
        if (dyn_unc)
            domain->mem = POL_GRAIN_NODE;
        else
            domain->mem = POL_NOT_SUPPORTED;

#if USE_GPUS
        if (c->num_gpus)
            domain->gpu = POL_GRAIN_CORE;
        else
            domain->gpu = POL_NOT_SUPPORTED;
#else
        domain->gpu = POL_NOT_SUPPORTED;
#endif

        domain->gpumem = POL_NOT_SUPPORTED;

        return EAR_SUCCESS;
    }
    else
    {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}
