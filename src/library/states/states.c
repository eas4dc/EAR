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

#if MPI
#include <mpi.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
// #define SHOW_DEBUGS 1
#if !SHOW_DEBUGS
#define NDEBUG // Activate assert calls only in the debug mode.
#endif
#include <assert.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/colors.h>
#include <common/math_operations.h>
#include <common/types/log.h>
#include <common/types/application.h>
#include <common/environment.h>

#include <management/cpufreq/frequency.h>

#include <daemon/local_api/eard_api.h>

#include <report/report.h>

#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/common/verbose_lib.h>
#include <library/tracer/tracer.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/cpu_support.h>
#include <library/states/states_comm.h>
#include <library/states/states.h>
#include <library/api/clasify.h>

#define NEW_CPUFREQ_API 1 // Enables the usage of the new metrics API,
                          // instead of the deprecated frequency API.
#define VERBOSE_WARN    2 // Verbose level for warning messages.
#define VERB_SAV_PEN    3 // Verbose level for saving and penalty estimation.
#define VERB_SIGN_TRACK 3 // Verbose level for signature tracking.
#if 0
#define DYNAIS_CUTOFF	1 // Enables the dynamic DynAIS turning off.
#endif

#define MAX_SIG_TIME    10000000 // 10 s. It is a time limit for signature computation.

extern masters_info_t masters_info; // TODO: Where declared ?
extern float          ratio_PPN;    // TODO: Where declared ?

/* Defined at ear.c */
extern report_id_t    rep_id;

extern cluster_conf_t cconf;


/* Period time computation control. */
static llong comp_N_begin;
static llong comp_N_end;
static llong comp_N_time;

       ulong perf_accuracy_min_time = 1000000; // The minimum period time to be elapsed
                                               // before starting signature computation.

// TODO: usage
static uint begin_iter;
static uint N_iter;
extern uint ear_guided;

// TODO: usage
static uint tries_current_loop           = 0;
static uint tries_current_loop_same_freq = 0;

// TODO: usage
static uint perf_count_period            = 100;
static uint perf_count_period_10p;

/* Policy related variables. */
static uint         EAR_POLICY_STATE      = EAR_POLICY_NO_STATE; // Policy state ("state machine")
static int          policy_max_tries_cnt;                        // The maximum number of tries we
                                                                 // let a policy to change the CPU
                                                                 // frequency.
static ulong        policy_cpufreq_khz;
static signature_t  policy_sel_signature;                        // Stores the signature used by
static signature_t  tracer_signature;
                                                                 // the last policy application
                                                                 // which returned EAR_POLICY_READY

/* Other */
static int          master                       = -1;        // Indicates whether the process is the master.
static uint         EAR_STATE                    = NO_PERIOD; // State machine controller.
static uint         total_threads_cnt;                        // Total number of processes
                                                              // being executed at the node.
static uint         signatures_stables           = 0;
static uint         *sig_ready;                               // An array indexed by CPU
                                                              // p-state which indicated whether
                                                              // a signature at that p-state
                                                              // is ready.
static loop_t       curr_loop;                                // Stores the current loop data
                                                              // reported to the report module.
static signature_t  loop_signature;                           // Stores the current loop
                                                              // signature computed.
static node_freqs_t node_freqs;
static float        MAX_DYNAIS_OVERHEAD_DYN      = MAX_DYNAIS_OVERHEAD; // Maximum DynAIS
                                                                        // overhead permitted.
       uint         waiting_for_node_signatures  = 0;
       timestamp_t  time_last_signature;
       uint         signature_reported           = 0;

extern uint check_periodic_mode;

/** Returns whether \p cpi_a and \p cpi_b and \p gbs_a \p gbs_b are
 * equal, respectively, with a maximum difference of \p eq_thresh permitted. */
static uint are_cpi_gbs_eq(double cpi_a, double cpi_b, double gbs_a, double gbs_b, double eq_thresh);

#if DYNAIS_CUTOFF
/** This function checks whether this application is going to be
 * affected or not (an estimation based on CPI and GBS) by DynIAS.
 * Sets \p dynais_on to DYNAIS_ENABLED if needed. */
static void check_dynais_on(const signature_t *A, const signature_t *B, uint *dynais_on);
#endif // DYNAIS_CUTOFF

/** This function checks if there is too much overhead from DynAIS and sets it to OFF. */
static void check_dynais_off(ulong mpi_calls_iter, uint period, uint level, ulong event);

/** Returns wether the policy affected some metrics. */
static uint policy_had_effect(const signature_t *A, const signature_t *B);

/**TODO */
static void compute_perf_count_period(llong n_iters_time, llong n_iters_time_min,
        uint *loop_perf_count_period, uint *loop_perf_count_period_accel);

/** Restarts the beginning time counter of the period stored at \p time_init. */
static void restart_period_time_init(llong *time_init);

/** Reports \p my_loop through the loaded report plug-ins. */
static void report_loop_signature(uint iterations, loop_t *my_loop);

static void accumulate_energy_savings(signature_t *loops, signature_t *apps);
static void accumulate_phases_summary(signature_t *loops, signature_t *apps);


static ear_event_t curr_state_event;
extern uint report_earl_events;
extern report_id_t rep_id;


void fill_common_event(ear_event_t *ev)
{
	ev->jid 		= application.job.id;
	ev->step_id = application.job.step_id;
	strcpy(ev->node_id, application.node_id);
}

void fill_event(ear_event_t *ev, uint event, llong value)
{
  ev->event = event;
  ev->freq  = value;
  ev->timestamp = time(NULL);
}


/** Executed at application start. Reads EAR configuration environment variables.*/
void states_begin_job(int my_id, char *app_name)
{
    master = !my_id;

    char *max_dyn_ov = getenv("EAR_MAX_DYNAIS_OVERHEAD");
    if (max_dyn_ov != NULL) {
        MAX_DYNAIS_OVERHEAD_DYN = atof(max_dyn_ov);
        debug("Using dynamic limit for dynais %lf", MAX_DYNAIS_OVERHEAD_DYN);
    }

    node_freqs_alloc(&node_freqs);


    // The number of available pstates
#if NEW_CPUFREQ_API
    uint pstate_cnt = metrics_get(MGT_CPUFREQ)->avail_count;
#else
    uint pstate_cnt = frequency_get_num_pstates();
#endif

    sig_ready  = (uint *) calloc(pstate_cnt, sizeof(uint));

#if 0
    // Disabled code as calloc automatically inits data to 0
    for (uint i = 0; i < pstate_cnt; i++) {
        sig_ready[i] = 0;
    }
#endif

    fill_common_event(&curr_state_event);

#if ONLY_MASTER
    if (my_id) return;
#endif

    /* LOOP WAS CREATED HERE BEFORE */

    perf_accuracy_min_time = get_ear_performance_accuracy();
	verbose_master(2, "perf_accuracy_min_time default: %lu", perf_accuracy_min_time);

    ulong architecture_min_perf_accuracy_time;
    if (master && eards_connected()) {
        architecture_min_perf_accuracy_time = eards_node_energy_frequency();
        if (architecture_min_perf_accuracy_time == (ulong) EAR_ERROR) {
            architecture_min_perf_accuracy_time = perf_accuracy_min_time;
        }
        verbose_master(2, "eard perf_accuracy_min_time default %lu", architecture_min_perf_accuracy_time);
    } else {
        architecture_min_perf_accuracy_time = 1000000;
    }
    if ((architecture_min_perf_accuracy_time > perf_accuracy_min_time) && (architecture_min_perf_accuracy_time < MAX_SIG_TIME)) {
        perf_accuracy_min_time = architecture_min_perf_accuracy_time;
    }
    verbose_master(2, "perf_accuracy_min_time %lu", perf_accuracy_min_time);

    EAR_STATE = NO_PERIOD;

    policy_cpufreq_khz   = EAR_default_frequency;
    loop_signature.def_f = EAR_default_frequency;

    init_log();

    if (state_fail(policy_max_tries(&policy_max_tries_cnt))) {
        verbose_master(VERBOSE_WARN, "%sWARNING%s Policy max tries could not be read.", COL_RED, COL_CLR);
    }

    total_threads_cnt =  get_total_resources();
    debug("Total threads: %u", total_threads_cnt);
}

/** This function is executed at application end. */
void states_end_job(int my_id,  char *app_name)
{
    debug("EAR(%s) Ends execution.", app_name);
    node_freqs_free(&node_freqs);
    free(sig_ready);
    end_log();
}

/** This function is executed at each loop init */
void states_begin_period(int my_id, ulong event, ulong size, ulong level)
{
    EAR_STATE = TEST_LOOP;


    tries_current_loop           = 0;
    tries_current_loop_same_freq = 0;

    if (state_fail(loop_init(&curr_loop, application.job.id,
                    application.job.step_id, application.node_id, event, size, level))) {
        verbose_master(2, "%sEAR Error%s Creating loop: %s", COL_RED, COL_CLR, state_msg);
    }

    policy_loop_init(&curr_loop.id);

    // comp_N_begin = metrics_time();
    restart_period_time_init(&comp_N_begin);

    #if SINGLE_CONNECTION
    if (master) {
        traces_new_period(ear_my_rank, my_id, event);
    }
    #else
    traces_new_period(ear_my_rank, my_node_id, event);
    #endif

    loop_with_signature = 0;

#if NEW_CPUFREQ_API
    uint pstate_cnt = metrics_get(MGT_CPUFREQ)->avail_count;
#else
    uint pstate_cnt = frequency_get_num_pstates();
#endif

    assert(pstate_cnt != 0);
    for (uint i = 0; i < pstate_cnt; i++){
        sig_ready[i]=0;
    }
}


/** This function is executed at each loop end */
void states_end_period(uint iterations)
{
    if (loop_with_signature)
    {
#if REPORT_TIMESTAMP
        curr_loop.total_iterations = (ulong) time(NULL);
#else
        curr_loop.total_iterations = iterations;
#endif
        if (system_conf->report_loops) {
            if (master ) {
                clean_db_loop(&curr_loop, system_conf->max_sig_power);
                if (report_loops(&rep_id, &curr_loop,1) != EAR_SUCCESS){
                    verbose_master(0,"Error reporting loop");
                }
            }
        }
    }

    loop_with_signature = 0;
    policy_loop_end(&curr_loop.id);
}

/** This is the main function when used DynAIS. It applies
 * the EAR state diagram that drives the internal behaviour. */
void states_new_iteration(int my_id, uint period, uint iterations,
        uint level, ulong event, ulong mpi_calls_iter, uint dynais_used)
{
    ulong prev_cpufreq_khz, lavg;
    int ready;
    int result;
    int pol_ok;
    int curr_pstate,def_pstate;
    ulong policy_def_freq;

    char use_case[SHORT_GENERIC_NAME]; // 16 bytes allocated. Used for verbose signature.

    /***************************************************************************************************/
    /**** This function can potentially include data sharing between masters, depends on the policy ****/
    /***************************************************************************************************/


    if (traces_are_on() && ((sig_shared_region[my_node_id].iterations %5 ) == 0)){
      signature_from_ssig(&tracer_signature, &sig_shared_region[my_node_id].sig);
      tracer_signature.def_f = sig_shared_region[my_node_id].new_freq;
      traces_new_signature(ear_my_rank, my_node_id, &tracer_signature);
    }


    sig_shared_region[my_node_id].iterations++;
    if (state_ok(metrics_new_iteration(&loop_signature))) {
        if (state_fail(policy_new_iteration(&loop_signature))) {

            verbose_master(2, "%sNew Phase detected%s", COL_BLU, COL_CLR);

            /* Accumulate statistics of savings */
            accumulate_phases_summary(&loop_signature, &application.signature);

            policy_set_default_freq(NULL);

            restart_period_time_init(&comp_N_begin);

            EAR_STATE = FIRST_ITERATION;

            if (report_earl_events) {
                // We can report two events at once.
                ear_event_t aux_events[2];
                aux_events[0] = curr_state_event;

                fill_common_event(&aux_events[1]);
                fill_event(&aux_events[1], EARL_STATE, EAR_STATE);

                report_events(&rep_id, aux_events, 2);
#if 0
						  fill_event(&curr_state_event, EARL_OPT_ACCURACY, OPT_TRY_AGAIN);
						  report_events(&rep_id, &curr_state_event, 1);
              fill_event(&curr_state_event, EARL_STATE, FIRST_ITERATION);
						  report_events(&rep_id, &curr_state_event, 1);
#endif
					  }
            return;
        }
    }

    prev_cpufreq_khz = ear_frequency; // TODO: Why khz are prev, but pstate is curr?
    curr_pstate      = frequency_closest_pstate(ear_frequency);

    policy_get_default_freq(&node_freqs); // Slaves don't fill node_freqs

    policy_def_freq  = node_freqs.cpu_freq[0];
    def_pstate       = frequency_closest_pstate(policy_def_freq);

    /* A rescheduling is a global event generated by the EARGM */
    if (system_conf != NULL) {
        if ((master) && (resched_conf->force_rescheduling)) {
            traces_reconfiguration(ear_my_rank, my_node_id);

            resched_conf->force_rescheduling = 0;

            verbose_master(2, "--- [EAR] Re-scheduling forced by EARD ---\n%-20s: %u\n%-20s: %lu\n%-20s: %lu\n%-20s: %lf\n",
                    "Curr. state", EAR_STATE, "Max CPUfreq", system_conf->max_freq, "Def. CPUfreq",
                    system_conf->def_freq, "Policy th", system_conf->settings[0]);

            restart_period_time_init(&comp_N_begin);

            EAR_STATE = SIGNATURE_HAS_CHANGED;
            state_report_traces_state(masters_info.my_master_rank, ear_my_rank, my_node_id, EAR_STATE);

            if (report_earl_events) {
                fill_event(&curr_state_event, EARL_STATE, EAR_STATE);
                report_events(&rep_id, &curr_state_event, 1);
            }

            tries_current_loop_same_freq = 0;
            tries_current_loop           = 0;

            return;
        }
    }

    /* Based on the EAR internal state... */
    switch (EAR_STATE)
    {
        /* This is a new state to minimize overhead, we just
         * check the loop has, at least, some granularity */
        case TEST_LOOP:
            comp_N_end = metrics_time();
            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);

            if (comp_N_time > perf_accuracy_min_time * 0.1) {
                debug("EAR_STATE <- FIRST_ITERATION. %d iterations elapsed", iterations);

                comp_N_begin = comp_N_end;

                EAR_STATE = FIRST_ITERATION;
                #if SINGLE_CONNECTION
                if (master) {
                    traces_start();
                }
                #else
                traces_start();
                #endif
            }

            break;

        /* FIRST_ITERATION computes the iteration duration and how many
         * iterations needs to be considered to compute a signature */
        case FIRST_ITERATION:
            comp_N_end = metrics_time();
            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);

            if (comp_N_time == 0) comp_N_time = 1;
            compute_perf_count_period(comp_N_time, (llong) perf_accuracy_min_time,
                    &perf_count_period, &perf_count_period_10p);

            /* Once min iterations are computed for performance accuracy we start
             * computing application signature. */
            EAR_STATE = EVALUATING_LOCAL_SIGNATURE;

            state_report_traces_state(masters_info.my_master_rank, ear_my_rank, my_node_id, EAR_STATE);
            metrics_compute_signature_begin();
            begin_iter = iterations;

            curr_loop.id.event = event;
            curr_loop.id.level = level;
            curr_loop.id.size = period;

            break; // FIRST_ITERATION

        /* If the signature (basically CPI and GBS) changes,
         * we must recompute the number of iterations. */
        case SIGNATURE_HAS_CHANGED:
            comp_N_end = metrics_time();
            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);

            compute_perf_count_period(comp_N_time, (llong) perf_accuracy_min_time, &perf_count_period, &perf_count_period_10p);

            EAR_STATE = EVALUATING_LOCAL_SIGNATURE;

            break;

        /* Again, we recompute the number of iterations to be considered but in this case
         * because we have changed the frequency. TODO: This state could be removed. */
        case RECOMPUTING_N:

            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);

            assert(comp_N_time != 0); // Only compiled with SHOW_DEBUGS enabled.

            // TODO: Here we don't tolerate comp_N_begin equal to 0
            if (comp_N_time == 0) {
                error( "EAR(%s): PANIC comp_N_time must be > 0", ear_app_name);
            }

            compute_perf_count_period(comp_N_time, (llong) perf_accuracy_min_time, &perf_count_period, &perf_count_period_10p);

            EAR_STATE = SIGNATURE_STABLE;

            break;

        /* After N iterations of being in this state the signature will be computed and
         * the policy will be applied, next state will depend on the policy itself.
         * See the REAMDE file to see the different alternatives. */
        case EVALUATING_LOCAL_SIGNATURE:

            /* We check from time to time if the signature is ready */
            /* Included to accelerate the signature computation */
            verbose_master(4, "%s--- EVALUATING LOCAL SIGNATURE ---%s", COL_YLW, COL_CLR);

            if (!lib_shared_region->master_ready) {
                if (!dynais_used && (iterations % mpi_calls_iter != 0)) {
                    return;
                }

                /* Only when using dynais we consider the 'N' */
                if (dynais_used) {
                    if (iterations % perf_count_period_10p == 0) {
                        if (time_ready_signature(perf_accuracy_min_time)) {
                            perf_count_period = iterations - 1;
                            if (!perf_count_period) {
                                perf_count_period = 1;
                            }
                        }
                    }
                    if ((iterations - 1) % perf_count_period || iterations == 1) return;
                }
            }

#if SHOW_DEBUGS
            if (lib_shared_region->master_ready) {
                if (master) {
                    debug(" Master: master ready waiting for slaves evaluating");
                } else {
                    debug("Slave: going to compute signature because master is ready evaluating");
                }
            }
#endif

            N_iter = iterations - begin_iter;

            /* Signature computation */
            result = metrics_compute_signature_finish(&loop_signature, N_iter,
                    perf_accuracy_min_time, total_threads_cnt);

			/* At this point, signatures for all processes in the node are ready.
             * lib_shared_region->job_signature includes the master signature. */
            if (result == EAR_NOT_READY)
            {
                if (!lib_shared_region->master_ready) {
                    perf_count_period++;
                }
                return;
            }

            // Both loop_signature and lib_shared_region->job_signature have the same data:
            compute_per_process_and_job_metrics(&loop_signature);
            if (VERB_ON(VERB_SIGN_TRACK) && master) {
                char sig_to_vrb[128];
                signature_to_str(&loop_signature, sig_to_vrb, sizeof(sig_to_vrb));

                verbose(0, "Signature after CPU power model projection: %s", sig_to_vrb);
            }

            /* Included for dynais test */
#if SHOW_DEBUGS
            if (loop_with_signature == 0) {
                time_t curr_time;
                double time_from_mpi_init;
                time(&curr_time);
                time_from_mpi_init = difftime(curr_time, application.job.start_time);
                debug("Number of seconds since the application start_time at which signature is computed %lf",
                        time_from_mpi_init);
            }
#endif

            loop_with_signature = 1;

            if (dynais_used) {
                check_periodic_mode = 0;
            }

            /* Computing dynais overhead.
             * Change dynais_enabled to ear_tracing_status = DYNAIS_ON/DYNAIS_OFF. */
            if (dynais_used && (dynais_enabled == DYNAIS_ENABLED)) {
                check_dynais_off(mpi_calls_iter, period, level, event);
            }

            sig_ready[curr_pstate] = 1;

            /* ******************* MOVED **********************
             * Now we compute the JOB signature for reporting. */
            if (master) {
                // curr_loop.signature <- lib_shared_region->job_signature
                metrics_job_signature(&lib_shared_region->job_signature, &curr_loop.signature);

                // lib_shared_region->job_signature <- curr_loop.signature
                signature_copy(&lib_shared_region->job_signature, &curr_loop.signature);
            }

            /* This function takes into account if we are using or not the whole node. */
            signature_t norm_signature;
            adapt_signature_to_node(&norm_signature, &lib_shared_region->job_signature, ratio_PPN);
            /* At this point curr_loop and job_signature signatures are the JOB signature
             * with accumulated per process and global (node) metrics (e.g., DC power). */

            /* This function executes the energy policy.
             * norm_signature is the local process signature,
             * the policy will compute the average if needed. */
            if (VERB_ON(VERB_SIGN_TRACK) && master) {

                char sig_to_vrb[128];
                signature_to_str(&norm_signature, sig_to_vrb, sizeof(sig_to_vrb));

                verbose(0, "Signature %s will be passed to the policy.", sig_to_vrb);
            }

            policy_node_apply(&norm_signature, &policy_cpufreq_khz, &ready);

            /* Accumulate statistics of savings */
            accumulate_phases_summary(&loop_signature, &application.signature);

            /* TODO: For no masters, ready will be 0, pending. */
            EAR_POLICY_STATE = ready;

            // EAR uses this timestamp to switch to periodic mode (or not).
            timestamp_getfast(&time_last_signature);

            /* State check */
            if (EAR_POLICY_STATE == EAR_POLICY_READY) {
                /* When the policy is ready to be evaluated, we go to the next state */

                signatures_stables = 0;
                if ((policy_cpufreq_khz != prev_cpufreq_khz) && dynais_used) {

                    tries_current_loop++;

                    // comp_N_begin = metrics_time();
                    restart_period_time_init(&comp_N_begin);

                    EAR_STATE = RECOMPUTING_N;
                } else {
                    EAR_STATE = SIGNATURE_STABLE;
                }

                signature_copy(&policy_sel_signature, &lib_shared_region->job_signature);
            } else if (EAR_POLICY_STATE == EAR_POLICY_GLOBAL_EV) {
                EAR_STATE = EVALUATING_GLOBAL_SIGNATURE;
            }

            begin_iter = iterations;

            // Only the master process will report the loop.
            report_loop_signature(iterations, &curr_loop);

            policy_get_current_freq(&node_freqs);
            lavg = node_freqs_avgcpufreq(node_freqs.cpu_freq);

            if (lavg > 0) {
                loop_signature.def_f = lavg;
            }

            /* VERBOSE */
            if (master){
              traces_generic_event(ear_my_rank, my_node_id, JOB_POWER, (ullong)lib_shared_region->job_signature.DC_power*1000);
              traces_generic_event(ear_my_rank, my_node_id, NODE_POWER, (ullong)lib_shared_region->node_signature.DC_power*1000);
            }
            if (traces_are_on()){
              signature_from_ssig(&tracer_signature, &sig_shared_region[my_node_id].sig);
              tracer_signature.def_f = sig_shared_region[my_node_id].new_freq;
              traces_new_signature(ear_my_rank, my_node_id, &tracer_signature);
            }


            // The reported signature will show whether the Library is using DynAIS
            if (dynais_used) {
                sprintf(use_case, "+D");
            } else {
                sprintf(use_case, "+P.%s",((ear_guided == TIME_GUIDED)?"T":"D"));
            }

            // Verbose loop signature
            state_verbose_signature(&curr_loop, masters_info.my_master_rank, show_signatures,
                                    ear_app_name, application.node_id, iterations,
                                    prev_cpufreq_khz, policy_cpufreq_khz, use_case);

            break; // EVALUATING_LOCAL_SIGNATURE

        case EVALUATING_GLOBAL_SIGNATURE:

            policy_app_apply(&policy_cpufreq_khz, &ready);

            // Select the next state
            if (ready == EAR_POLICY_READY) {
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            }

            break; // EVALUATING_GLOBAL_SIGNATURE

        /* If the policy is ok, we are in stable state and we increase
         * the number of iterations to compute the signature. */
        case SIGNATURE_STABLE:

            if (!lib_shared_region->master_ready) {
                if (!dynais_used && (iterations % mpi_calls_iter != 0)) {
                    return;
                }

                // I have executed N iterations more with a new frequency, we must check the signature
                if (dynais_used && ((iterations - 1) % perf_count_period)) {
                    return;
                }
            }

#if SHOW_DEBUGS
            if (lib_shared_region->master_ready) {
                if (master){
                    debug("(Stable) Master ready and waiting for slaves");
                } else {
                    debug("(Stable) Slave going to compute signature because master is ready");
                }
            }
#endif
            /* We can compute the signature */
            N_iter = iterations - begin_iter;

            result = metrics_compute_signature_finish(&loop_signature, N_iter, perf_accuracy_min_time, total_threads_cnt);
            if (result == EAR_NOT_READY)
            {
                if (!lib_shared_region->master_ready) {
                    perf_count_period++;
                }
                return;
            }

			/* At this point, signatures are ready */

            /* This function uses cpu power model and accumulates global metrics (GBS, Power etc) */
            compute_per_process_and_job_metrics(&curr_loop.signature);

			/* This function accumulates per procecss metrics in curr_loop.signature,
             * so curr_loop.signature is the job signature */
            if (master) {
                metrics_job_signature(&lib_shared_region->job_signature, &curr_loop.signature);
                signature_copy(&loop_signature, &curr_loop.signature); // loop_signature <- curr_loop.signature
                signature_copy(&lib_shared_region->job_signature, &curr_loop.signature); // lib_shared_region->job_signature <- curr_loop.signature
            }
            if (master){
              traces_generic_event(ear_my_rank, my_node_id, JOB_POWER, (ullong)lib_shared_region->job_signature.DC_power*1000);
              traces_generic_event(ear_my_rank, my_node_id, NODE_POWER, (ullong)lib_shared_region->node_signature.DC_power*1000);
            }
            if (traces_are_on()){
              signature_from_ssig(&tracer_signature, &sig_shared_region[my_node_id].sig);
              tracer_signature.def_f = sig_shared_region[my_node_id].new_freq;
              traces_new_signature(ear_my_rank, my_node_id, &tracer_signature);
            }


            timestamp_getfast(&time_last_signature);

            /* At this point curr_loop, loop_signature and job_signature are the
             * JOB signature with accumulated per process and global metrics */

            report_loop_signature(iterations, &curr_loop);

            /* VERBOSE */
            if (dynais_used) {
                sprintf(use_case, "+D-Stable");
            } else {
                sprintf(use_case, "+P.%s-Stable",((ear_guided == TIME_GUIDED)?"T":"D"));
            }
            state_verbose_signature(&curr_loop, masters_info.my_master_rank, show_signatures, ear_app_name, application.node_id, iterations, prev_cpufreq_khz,0,use_case);
            /* END VERBOSE */

            begin_iter = iterations;
            if (sig_ready[curr_pstate] == 0){
                sig_ready[curr_pstate]=1;
            }

            /* We must evaluate policy decissions */
            signatures_stables++;

            if (signatures_different(&loop_signature, &policy_sel_signature, 0.2)) {

              /* Reset */
              policy_set_default_freq(&loop_signature);
              EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
              /* And then we mark as not ready */
              if (master) clean_signatures(lib_shared_region,sig_shared_region);

              if (VERB_ON(2)) {

                char sigs_metrics_buff[128]; // This buffer is used to show differences in signatures.

                snprintf(sigs_metrics_buff, sizeof(sigs_metrics_buff),
                    "(CPI/GB/s/DC) current (%.2lf/%.2lf/%.2lf) policy (%.2lf/%.2lf/%.2lf)",
                    loop_signature.CPI, loop_signature.GBS, loop_signature.DC_power,
                    policy_sel_signature.CPI, policy_sel_signature.GBS, policy_sel_signature.DC_power);

                verbose_master(0, "%sWARNING%s EARL state reset because signature changed from the last passed to the policy: %s",
                    COL_YLW, COL_CLR, sigs_metrics_buff);
              }

              break;
            }

            if (sig_ready[def_pstate] == 0) {
                verbose_master(2,"Signature at default freq not available, assuming policy ok");
                /* If default is not available, that means a dynamic configuration has been decided, we assume we are ok */
                if (sig_ready[curr_pstate] == 0) {
                    sig_ready[curr_pstate] = 1;
                }
                /* And then we mark as not ready */
                if (master) clean_signatures(lib_shared_region,sig_shared_region);
                return;
            }

			      /* Accumulate Energy savings */
			      accumulate_energy_savings(&loop_signature, &application.signature);

            /* The policy evaluates following it's own criteria. */
            policy_ok(&loop_signature, &policy_sel_signature, &pol_ok);

            if (pol_ok) {
                /* When collecting traces, we maintain the period */
                if ((master) && (traces_are_on() == 0)) {
                    perf_count_period = perf_count_period * 2;
                }
                tries_current_loop = 0;

                /* And then we mark as not ready */
                if (master)  clean_signatures(lib_shared_region,sig_shared_region);

                return;

            } else {
                /* Reset */
                policy_set_default_freq(&loop_signature);
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            }

            // We can give the library a second change in case we are at def freq
            if ((tries_current_loop < policy_max_tries_cnt && curr_pstate == def_pstate) || !dynais_used) {

                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
                /* And then we mark as not ready */
                if (master) clean_signatures(lib_shared_region,sig_shared_region);
                break;
            }

            /* If we cannot select an optimal frequency, We must report a problem and go to the default configuration. */
            if (tries_current_loop >= policy_max_tries_cnt) {
                EAR_STATE = PROJECTION_ERROR;
                policy_get_default_freq(&node_freqs);
                policy_cpufreq_khz = node_freqs.cpu_freq[0];
                if (master) {
                    log_report_max_tries(application.job.id,application.job.step_id, application.job.def_f);
                    // traces_frequency(ear_my_rank, my_id, policy_cpufreq_khz);
                    /* And then we mark as not ready */
                    clean_signatures(lib_shared_region,sig_shared_region);
                }

                break;
            }

            /* We are not going better. If signature is different, we consider as a new loop case. */
            if (!policy_had_effect(&loop_signature, &policy_sel_signature))
            {
                EAR_STATE = SIGNATURE_HAS_CHANGED;
                // comp_N_begin = metrics_time();
                restart_period_time_init(&comp_N_begin);
                policy_loop_init(&curr_loop.id);
#if DYNAIS_CUTOFF
                check_dynais_on(&loop_signature, &policy_sel_signature, &dynais_enabled);
#endif
            } else {
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            }
            /* And then we mark as not ready */
            if (master) clean_signatures(lib_shared_region,sig_shared_region);

            break; // SIGNATURE_STABLE

        case PROJECTION_ERROR:
            verbose(2, "%s--- PROJECTION ERROR ---%s", COL_YLW, COL_CLR);

            if (master && traces_are_on()) {
                /* We compute the signature just in case EAR_GUI is on TODO: wtf is EAR_GUI */
                if (((iterations - 1) % perf_count_period)) return;

                /* We can compute the signature */
                N_iter = iterations - begin_iter;
                result = metrics_compute_signature_finish(&loop_signature, N_iter,
                        perf_accuracy_min_time, total_threads_cnt);

                if (result == EAR_NOT_READY) {
                    perf_count_period++;
                    break;
                }

                /* VERBOSE */
                signature_copy(&curr_loop.signature, &loop_signature); // curr_loop.signature <- loop_signature

                //state_report_traces(masters_info.my_master_rank, ear_my_rank, my_id,
                 //       &curr_loop, prev_cpufreq_khz,EAR_STATE);

                state_verbose_signature(&curr_loop, masters_info.my_master_rank, show_signatures,
                        ear_app_name, application.node_id, iterations, prev_cpufreq_khz, 0, "D-PE");
            }

            /* We run here at default freq */
            break;
        default: break;
    }
    if (report_earl_events && (EAR_STATE != TEST_LOOP)){
			fill_event(&curr_state_event, EARL_STATE, EAR_STATE);
			report_events(&rep_id, &curr_state_event, 1);
		}


    state_report_traces_state(masters_info.my_master_rank, ear_my_rank, my_id, EAR_STATE);
    debug("End states_end_iteration");
}

static uint are_cpi_gbs_eq(double cpi_a, double cpi_b, double gbs_a, double gbs_b, double eq_thresh)
{
    uint cpi_eq = equal_with_th(cpi_a, cpi_b, eq_thresh);
    uint gbs_eq = equal_with_th(gbs_a, gbs_b, eq_thresh);

    return cpi_eq & gbs_eq;
}

#if DYNAIS_CUTOFF
static void check_dynais_on(const signature_t *A, const signature_t *B, uint *dynais_on)
{
    assert(A != NULL && B != NULL);

    if (!are_cpi_gbs_eq(A->CPI, B->CPI, A->GBS, B->GBS, EAR_ACCEPTED_TH * 2)) {
        *dynais_on = DYNAIS_ENABLED;
        debug("DynAIS %sON%s", COL_GRE, COL_CLR);
    }
}
#endif // DYNAIS_CUTOFF

static void check_dynais_off(ulong mpi_calls_iter, uint period, uint level, ulong event)
{
    debug("[%d] Dynais: Time %f ov %f limit %f", my_node_id, loop_signature.time, ((float) mpi_calls_iter / 1000000.0) * 100.0 / loop_signature.time, MAX_DYNAIS_OVERHEAD_DYN);
    if (loop_signature.time > 0) {
        ulong dynais_overhead_usec = mpi_calls_iter;

        float dynais_overhead_perc = ((float) dynais_overhead_usec / 1000000.0) * 100.0 / loop_signature.time;

        if (dynais_overhead_perc > MAX_DYNAIS_OVERHEAD_DYN) {
            // Disable dynais: TODO: API is still pending.
#if DYNAIS_CUTOFF
            dynais_enabled = DYNAIS_DISABLED;
#endif
            if (master) {
                log_report_dynais_off(application.job.id, application.job.step_id);
            }
        }

        if (dynais_enabled == DYNAIS_DISABLED) {
            debug("    --- DynAIS disabled ---\n%15s: %lf s\n%15s: %lu us\n%15s: %lu (%lf %%)\n%15s: %lu\n%15s: %lu\n",
                    "Total time", loop_signature.time,
                    "DynAIS overhead", dynais_overhead_usec,
                    "MPI calls", mpi_calls_iter, dynais_overhead_perc,
                    "Event", event,
                    "Min. time", perf_accuracy_min_time);
        }
    }

    last_first_event    = event;          // TODO: where it comes from? usage
    last_calls_in_loop  = mpi_calls_iter; // TODO: where it comes from? usage
    last_loop_size      = period;         // TODO: where it comes from? usage
    last_loop_level     = level;          // TODO: where it comes from? usage
}

static uint policy_had_effect(const signature_t *A, const signature_t *B)
{
    assert(A != NULL && B != NULL);

    return are_cpi_gbs_eq(A->CPI, B->CPI, A->GBS, B->GBS, EAR_ACCEPTED_TH);
}

static void compute_perf_count_period(llong n_iters_time, llong n_iters_time_min,
        uint *loop_perf_count_period, uint *loop_perf_count_period_accel)
{
    if (n_iters_time < (llong) n_iters_time_min) {
        *loop_perf_count_period = (n_iters_time_min / n_iters_time) + 1;
    } else {
        *loop_perf_count_period = 1;
    }

    /* Included to accelerate the signature computation */
    *loop_perf_count_period_accel = *loop_perf_count_period / 10;
    if (*loop_perf_count_period_accel == 0) {
        *loop_perf_count_period_accel = 1;
    }
}

static void restart_period_time_init(llong *time_init)
{
    *time_init = metrics_time();
}

static void report_loop_signature(uint iterations, loop_t *my_loop)
{
#if REPORT_TIMESTAMP
    my_loop->total_iterations =  (ulong)time(NULL);
#else
    my_loop->total_iterations = iterations;
#endif

    if (master ) {

        clean_db_loop(my_loop, system_conf->max_sig_power);

        application.power_sig.max_DC_power = ear_max(application.power_sig.max_DC_power, my_loop->signature.DC_power);

        if (state_fail(report_loops(&rep_id, my_loop, 1))) {
            verbose_master(2, "%sError%s Reporting loop.", COL_RED, COL_CLR);
        }
    }

    signature_reported = 1;
}

static void accumulate_phases_summary(signature_t *loops, signature_t *apps)
{
  sig_ext_t *se       = (sig_ext_t *)apps->sig_ext;
  sig_ext_t *se_loop  = (sig_ext_t *)loops->sig_ext;

  if (se == NULL)       return;
  if (se_loop == NULL)  return;

  for (uint ph = 0; ph < EARL_MAX_PHASES; ph++){
    se->earl_phase[ph].elapsed += se_loop->earl_phase[ph].elapsed; 
    if (se->earl_phase[ph].elapsed || se_loop->earl_phase[ph].elapsed){
      verbose_master(2, "Phase[%s] accumulated elapsed %llu", phase_to_str(ph), se->earl_phase[ph].elapsed);
    }
  }
}

static void accumulate_energy_savings(signature_t *loops, signature_t *apps)
{
    static char verb_savs_pens_hdr[128] =
        "    --- Savings and penalties ---\n"
        "Scope E.save  P.save  T.pen   T.elapsed\n"
        "----- ------  ------  -----   ---------"; // Header for savings and penalties verbose

	sig_ext_t *loop_sig_ext = loops->sig_ext;
	sig_ext_t *app_sig_ext;

	if (loop_sig_ext == NULL) return;
	if (apps->sig_ext == NULL) {
		apps->sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
	}
	app_sig_ext = apps->sig_ext;

	if (loop_sig_ext->saving == 0) return;

	app_sig_ext->saving   += loop_sig_ext->saving;
	app_sig_ext->psaving  += loop_sig_ext->psaving;
	app_sig_ext->telapsed += loop_sig_ext->elapsed;
	app_sig_ext->tpenalty += loop_sig_ext->tpenalty;

    verbose_master(VERB_SAV_PEN, "%s\n Loop %-7.2lf %-7.2f %-7.2f %-9.2lf\n  App %-7.2lf %-7.2f %-7.2f %-9.2lf\n", verb_savs_pens_hdr,
            loop_sig_ext->saving, loop_sig_ext->psaving, loop_sig_ext->tpenalty, loop_sig_ext->elapsed,
            app_sig_ext->saving, app_sig_ext->psaving, app_sig_ext->tpenalty, app_sig_ext->telapsed);
}
