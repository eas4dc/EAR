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

//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/colors.h>
#include <common/math_operations.h>
#include <common/types/log.h>
#include <common/types/application.h>
#include <report/report.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/common/verbose_lib.h>
#include <library/tracer/tracer.h>
#include <library/states/states.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/cpu_support.h>
#include <library/states/states_comm.h>

#include <management/cpufreq/frequency.h>
#include <daemon/local_api/eard_api.h>
#include <common/environment.h>

extern masters_info_t masters_info;
extern float ratio_PPN;

application_t *signatures;
uint *sig_ready;
// static application_t last_signature;

static long long comp_N_begin, comp_N_end, comp_N_time;
static uint begin_iter, N_iter;
static ulong policy_freq;
static uint tries_current_loop=0;
static uint tries_current_loop_same_freq=0;

static ulong perf_accuracy_min_time = 1000000;
static uint perf_count_period = 100,loop_perf_count_period,perf_count_period_10p;
uint EAR_STATE = NO_PERIOD;
uint current_earl_state = NO_PERIOD;
static uint EAR_POLICY_STATE = EAR_POLICY_NO_STATE;
static int current_loop_id;
static int MAX_POLICY_TRIES;
static state_t pst;
static uint total_th;
static uint signatures_stables = 0;
static signature_t /* cpu_sel_signature, */ policy_sel_signature;
static node_freqs_t nf;
uint waiting_for_node_signatures = 0;



#define DYNAIS_CUTOFF	1

#if EAR_OVERHEAD_CONTROL
extern uint check_periodic_mode;
#endif



/** This funcion must be policy dependent */
ulong select_near_freq(ulong avg)
{
    ulong near;
    ulong norm;
    norm=avg/100000;
    near=norm*100000;
    return near;
}


/* This function check is this application is going to be affected or not (an estimation based on CPI and GBS) by DynIAS */
static void check_dynais_on(signature_t *A, signature_t *B)
{
    if (!equal_with_th(A->CPI, B->CPI, EAR_ACCEPTED_TH*2) || !equal_with_th(A->GBS, B->GBS, EAR_ACCEPTED_TH*2)){
        dynais_enabled=DYNAIS_ENABLED;
        debug("Dynais ON ");
    }
}

/* This function checks if there is too much overhead from DynAIS and sets it to OFF */
static void check_dynais_off(ulong mpi_calls_iter,uint period, uint level, ulong event)
{
#if EAR_OVERHEAD_CONTROL
    ulong dynais_overhead_usec = 0;
    float dynais_overhead_perc;

    dynais_overhead_usec = mpi_calls_iter;
    if (loop_signature.signature.time > 0){
        dynais_overhead_perc = ((float)dynais_overhead_usec/(float)1000000)*(float)100/loop_signature.signature.time;
        if (dynais_overhead_perc>MAX_DYNAIS_OVERHEAD){
            // Disable dynais : API is still pending
#if DYNAIS_CUTOFF
            dynais_enabled = DYNAIS_DISABLED;
#endif
            if (masters_info.my_master_rank>=0) log_report_dynais_off(application.job.id,application.job.step_id);
        }
        if (dynais_enabled == DYNAIS_DISABLED){
            debug("DYNAIS_DISABLED: Total time %lf (s) dynais overhead %lu usec in %lu mpi calls(%lf percent), event=%lu min_time=%lu",
                    loop_signature.signature.time,dynais_overhead_usec,mpi_calls_iter,dynais_overhead_perc,event,perf_accuracy_min_time);
        }
    }
    last_first_event    = event;
    last_calls_in_loop  = mpi_calls_iter;
    last_loop_size      = period;
    last_loop_level     = level;
#endif

}

static int policy_had_effect(signature_t *A, signature_t *B)
{
    if (equal_with_th(A->CPI, B->CPI, EAR_ACCEPTED_TH) &&
            equal_with_th(A->GBS, B->GBS, EAR_ACCEPTED_TH))
    {
        return 1;
    }
    return 0;
}


#ifdef SHOW_DEBUGS
static void print_loop_signature(char *title, signature_t *loop)
{
    float avg_f = (float) loop->avg_f / 1000000.0;

    debug("(%s) Avg. freq: %.2lf (GHz), CPI/TPI: %0.3lf/%0.3lf, GBs: %0.3lf, DC power: %0.3lf, time: %0.3lf, GFLOPS: %0.3lf",
            title, avg_f, loop->CPI, loop->TPI, loop->GBS, loop->DC_power, loop->time, loop->Gflops);
}
#endif


void states_new_iteration(int my_id, uint period, uint iterations, uint level, ulong event, ulong mpi_calls_iter,uint dynais_used);
static void report_loop_signature(uint iterations,loop_t *my_loop,job_t *job)
{
    my_loop->total_iterations = iterations;
    my_loop->jid 							= job->id;
    my_loop->step_id 					= job->step_id;
#if 0
    append_loop_text_file(loop_summary_path, my_loop,job);
#endif
    if (masters_info.my_master_rank>=0){
        clean_db_loop(my_loop, system_conf->max_sig_power);
#if 0
        eards_write_loop_signature(my_loop);
#endif
        if (report_loops(my_loop,1) != EAR_SUCCESS){
            verbose_master(0,"Error reporting Loop");
        }

    }
}


/* End auxiliary functions */

/* This function is executed at application end */
void states_end_job(int my_id,  char *app_name)
{
    debug("EAR(%s) Ends execution. ", app_name);
    end_log();
}


/* This function is executed at application init */
void states_begin_job(int my_id,  char *app_name)
{
    ulong	architecture_min_perf_accuracy_time;
    int i;

    node_freqs_alloc(&nf);
    signatures=(application_t *) calloc(frequency_get_num_pstates(),sizeof(application_t));
    sig_ready=(uint *)calloc(frequency_get_num_pstates(),sizeof(uint));
    for (i=0;i<frequency_get_num_pstates();i++){ 
        init_application(&signatures[i]);
        sig_ready[i]=0;
    }
    //init_application(&last_signature);
#if ONLY_MASTER
    if (my_id) return;
#endif

    /* LOOP WAS CREATED HERE BEFORE */

    perf_accuracy_min_time = get_ear_performance_accuracy();
    if (masters_info.my_master_rank>=0){
        architecture_min_perf_accuracy_time=eards_node_energy_frequency();
    }else{
        architecture_min_perf_accuracy_time=1000000;
    }
    if (architecture_min_perf_accuracy_time>perf_accuracy_min_time) perf_accuracy_min_time=architecture_min_perf_accuracy_time;

    EAR_STATE = NO_PERIOD;
    policy_freq = EAR_default_frequency;
    init_log();
    pst=policy_max_tries(&MAX_POLICY_TRIES);
    total_th =  get_total_resources();
    debug("Using %u total_threads",total_th);
}


int states_my_state()
{
    return EAR_STATE;
}


/* This function is executed at each loop init */
void states_begin_period(int my_id, ulong event, ulong size,ulong level)
{
    int i;
    EAR_STATE = TEST_LOOP;
    tries_current_loop=0;
    tries_current_loop_same_freq=0;


    if (loop_init(&loop,&loop_signature.job,event,size,level)!=EAR_SUCCESS){
        error("Error creating loop");
    }

    policy_loop_init(&loop.id);
    comp_N_begin = metrics_time();
    if (masters_info.my_master_rank>=0) traces_new_period(ear_my_rank, my_id, event);
    loop_with_signature = 0;
    for (i=0;i<frequency_get_num_pstates();i++){
        init_application(&signatures[i]);
        sig_ready[i]=0;
    }
}


/* This function is executed at each loop end */
void states_end_period(uint iterations)
{
    if (loop_with_signature)
    {
        loop.total_iterations = iterations;
        //append_loop_text_file(loop_summary_path, &loop,&loop_signature.job);
        if (system_conf->report_loops){
            if (masters_info.my_master_rank>=0){
                clean_db_loop(&loop, system_conf->max_sig_power);
#if 0
                eards_write_loop_signature(&loop);
#endif
                if (report_loops(&loop,1) != EAR_SUCCESS){
                    verbose_master(0,"Error reporting loop");
                }
            }
        }
    }

    loop_with_signature = 0;
    policy_loop_end(&loop.id);
}


/* This is the main function when used DynAIS. It applies the EAR state diagram that drives the internal behaviour */
void states_new_iteration(int my_id, uint period, uint iterations, uint level, ulong event,ulong mpi_calls_iter,uint dynais_used)
{
    unsigned long prev_f, lavg;
    int ready;
    int result;
    int pok;
    int curr_pstate,def_pstate;
    signature_t *l_sig;
    ulong policy_def_freq;
    signature_t app_signature, check_signature;	
    char use_case[16];
    char *ok_clr;

    /***************************************************************************************************/
    /**** This function can potentially include data sharing between masters, depends on the policy ****/
    /***************************************************************************************************/
    sig_shared_region[my_node_id].iterations++;
    if (metrics_new_iteration(&check_signature) == EAR_SUCCESS){
    	pst = policy_new_iteration(&check_signature);
    	if (pst != EAR_SUCCESS){
        	verbose_master(2,"%sNew Phase detected%s",COL_BLU,COL_CLR);
        	policy_set_default_freq(NULL);
        	comp_N_begin = metrics_time();
        	EAR_STATE = FIRST_ITERATION;
        	return;
    	}
		}

    prev_f = ear_frequency;
    curr_pstate=frequency_closest_pstate(ear_frequency);
    pst = policy_get_default_freq(&nf);
    policy_def_freq = nf.cpu_freq[0];	
    def_pstate=frequency_closest_pstate(policy_def_freq);

    /* A rescheduling is a global event generated by the EARGM */
    if (system_conf!=NULL){
        if ((masters_info.my_master_rank>=0) && (resched_conf->force_rescheduling)){
            traces_reconfiguration(ear_my_rank, my_id);
            resched_conf->force_rescheduling=0;
            verbose_master(2,"EAR: rescheduling forced by eard: current_state %u max freq %lu def_freq %lu def_th %lf",
                    EAR_STATE,system_conf->max_freq,system_conf->def_freq,system_conf->settings[0]);
            EAR_STATE = SIGNATURE_HAS_CHANGED;
            comp_N_begin = metrics_time();
            state_report_traces_state(masters_info.my_master_rank,ear_my_rank, my_id,SIGNATURE_HAS_CHANGED);
            tries_current_loop_same_freq=0;
            tries_current_loop=0;
            verbose_master(2,"New state by reschedduling %u",EAR_STATE);
            return;
        }
    }

    /* Based on the EAR internal state... */
    switch (EAR_STATE)
    {
        /* This is a new state to minimize overhead, we just check the loop has, at least, some granularity */
        case TEST_LOOP:
            comp_N_end = metrics_time();
            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);
            if (comp_N_time>(perf_accuracy_min_time*0.1)){
                debug("Going to FIRST_ITERATION after %d iterations",iterations);
                comp_N_begin=comp_N_end;
                EAR_STATE=FIRST_ITERATION;
                if (masters_info.my_master_rank>=0) traces_start();
            }
            break;

        /* FIRST_ITERATION computed the iteration duration and how many iterations needs to be considered to compute a signature */
        case FIRST_ITERATION: /********* FIRST_ITERATION *********/
            comp_N_end = metrics_time();
            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);

            if (comp_N_time == 0) comp_N_time = 1;
            /* We include a dynamic configurarion of EAR */
            if (comp_N_time < (long long) perf_accuracy_min_time) {
                perf_count_period = (perf_accuracy_min_time / comp_N_time) + 1;
            } else {
                perf_count_period = 1;
            }
            loop_perf_count_period=perf_count_period;
            /* Included to accelerate the signature computation */
            perf_count_period_10p=perf_count_period/10;
            if (perf_count_period_10p==0) perf_count_period_10p=1;
            /**/

            /* Once min iterations is computed for performance accuracy we start computing application signature */
            EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            state_report_traces_state(masters_info.my_master_rank,ear_my_rank, my_id,EVALUATING_LOCAL_SIGNATURE);
            metrics_compute_signature_begin();
            begin_iter = iterations;

            loop.id.event = event;
            loop.id.level = level;
            loop.id.size = period;
            break;

            /* If the signature (basically CPI and GBS) changes, we must recompute the number of iterations */
        case SIGNATURE_HAS_CHANGED: /********* SIGNATURE_HAS_CHANGED *************/
            comp_N_end = metrics_time();
            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);

            if (comp_N_time < (long long) perf_accuracy_min_time) {
                perf_count_period = (perf_accuracy_min_time / comp_N_time) + 1;
            } else {
                perf_count_period = 1;
            }                                        
            /* Included to accelerate the signature computation */
            perf_count_period_10p=perf_count_period/10;
            if (perf_count_period_10p==0) perf_count_period_10p=1;
            /**/
            loop_perf_count_period=perf_count_period;
            EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            break;

            /* Again, we recompute the number of iterations to be considered but in this case because we have changed the frequency */
            /* This state could be removed */
        case RECOMPUTING_N:/************ RECOMPUTING_N *************/

            comp_N_time = metrics_usecs_diff(comp_N_end, comp_N_begin);
            if (comp_N_time == 0) {
                error( "EAR(%s):PANIC comp_N_time must be >0", ear_app_name);
            }
            /* We include a dynamic configurarion of DPD behaviour */
            if (comp_N_time < (long long) perf_accuracy_min_time) {
                perf_count_period = (perf_accuracy_min_time / comp_N_time) + 1;
            } else {
                perf_count_period = 1;
            }
            /* Included to accelerate the signature computation */
            perf_count_period_10p=perf_count_period/10;
            if (perf_count_period_10p==0) perf_count_period_10p=1;
            /**/
            loop_perf_count_period=perf_count_period;
            EAR_STATE = SIGNATURE_STABLE;
            break;

            /* After N iterations of being in this state the signature will be computed and the policy will be applied, next state will 
             * 		depend on the policy itself. See the REAMDE file to see the different alternatives */
        case EVALUATING_LOCAL_SIGNATURE: /********* EVALUATING_LOCAL_SIGNATURE **********/
            /* We check from time to time if the signature is ready */
            /* Included to accelerate the signature computation */
            if (!lib_shared_region->master_ready){
                if (!dynais_used && (((iterations % mpi_calls_iter) != 0))) return;
                /* Only when using dynais we consider the 'N' */
                if (dynais_used){
                    if ((iterations%perf_count_period_10p)==0){
                        if (time_ready_signature(perf_accuracy_min_time)){	
                            perf_count_period=iterations - 1;
                            if (perf_count_period==0) perf_count_period=1;
                        }
                    }
                    if (((iterations - 1) % perf_count_period) || (iterations == 1)) return;
                }
            }
#if SHOW_DEBUGS
            if (lib_shared_region->master_ready){
                if (masters_info.my_master_rank >= 0){
                    debug(" Master: master ready waiting for slaves evaluating");
                }else{
                    debug("Slave: going to compute signature because master is ready evaluating");
                }
            }
#endif

            N_iter = iterations - begin_iter;

            /* Signature computation */
            result = metrics_compute_signature_finish(&loop_signature.signature, N_iter, perf_accuracy_min_time, total_th);	
            if (result == EAR_NOT_READY)
            {
                if (!lib_shared_region->master_ready) perf_count_period++;
                return;
            } 

            /* Included for dynais test */
#ifdef SHOW_DEBUGS
            if (loop_with_signature==0){
                time_t curr_time;
                double time_from_mpi_init;
                time(&curr_time);    
                time_from_mpi_init=difftime(curr_time,application.job.start_time);
                debug("Number of seconds since the application start_time at which signature is computed %lf",time_from_mpi_init);
            }
#endif

            loop_with_signature = 1;
#if EAR_OVERHEAD_CONTROL
            if (dynais_used) check_periodic_mode=0;
#endif

            /* Computing dynais overhead
             ** Change dynais_enabled to ear_tracing_status=DYNAIS_ON/DYNAIS_OFF */
            if (dynais_used && (dynais_enabled==DYNAIS_ENABLED)){
                check_dynais_off(mpi_calls_iter,period,level,event);
            }
            current_loop_id = event;
            //begin_iter = iterations;
            memcpy(&signatures[curr_pstate], &loop_signature, sizeof(application_t));
            sig_ready[curr_pstate] = 1;

            /****** We mark our local signature as ready ************ Moved to metrics*/
            //signature_ready(&sig_shared_region[my_node_id],EVALUATING_LOCAL_SIGNATURE);

            /* This function executes the energy policy */
            adapt_signature_to_node(&app_signature,&loop_signature.signature,ratio_PPN);
            pst=policy_node_apply(&app_signature,&policy_freq,&ready);

            /* For no masters, ready will be 0, pending */
            EAR_POLICY_STATE = ready;

            /* State check */
            /* When the policy is ready to be evaluated, we go to the next state */
            if (EAR_POLICY_STATE == EAR_POLICY_READY){
                verbose_master(2,"Policy ready after evaluating local signature");
                /*if (policy_freq != policy_def_freq)*/
                signatures_stables = 0;
                if ((policy_freq != prev_f) && (dynais_used)){
                    tries_current_loop++;
                    comp_N_begin = metrics_time();
                    EAR_STATE = RECOMPUTING_N;
                } else {
                    signature_copy(&policy_sel_signature, &loop_signature.signature);
                    EAR_STATE = SIGNATURE_STABLE; }
            }
            if (EAR_POLICY_STATE == EAR_POLICY_GLOBAL_EV){ EAR_STATE = EVALUATING_GLOBAL_SIGNATURE; }
            begin_iter = iterations;
            //signature_copy(&loop.signature, &loop_signature.signature);
            metrics_node_signature(&loop_signature.signature,&loop.signature);
            report_loop_signature(iterations,&loop,&loop_signature.job);
            policy_get_current_freq(&nf);
            lavg = node_freqs_avgcpufreq(nf.cpu_freq);	
            if (lavg > 0 ) loop_signature.signature.def_f = lavg;

            /* VERBOSE */
            state_report_traces(masters_info.my_master_rank,ear_my_rank, my_id,&loop,policy_freq,EAR_STATE);
            if (dynais_used) sprintf(use_case,"D");
            else sprintf(use_case,"P");
            state_verbose_signature(&loop,masters_info.my_master_rank,show_signatures,ear_app_name,application.node_id,iterations,prev_f,policy_freq,use_case);

            /* END VERBOSE */
            break;
        case EVALUATING_GLOBAL_SIGNATURE:
            //if (masters_info.my_master_rank>=0) verbose(1,"EVALUATING_GLOBAL_SIGNATURE");
            /* st =  */policy_app_apply(&policy_freq,&ready);
            /* State validation */
            if (ready == EAR_POLICY_READY){
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            }
            break;

            /* If the policy is ok, we are in stable state and we increase the numbers of iterations to compute the signature */
        case SIGNATURE_STABLE:
            if (!lib_shared_region->master_ready){

                if (!dynais_used && ((iterations % mpi_calls_iter) != 0)) return;
                // I have executed N iterations more with a new frequency, we must check the signature
                if (dynais_used &&  ((iterations - 1) % perf_count_period) ) return;
            }
#if SHOW_DEBUGS
            if (lib_shared_region->master_ready){
                if (masters_info.my_master_rank >= 0){
                    debug(" Master: master ready waiting for slaves stable");
                }else{
                    debug("Slave: going to compute signature because master is ready stable");
                }
            }
#endif
            /* We can compute the signature */
            N_iter = iterations - begin_iter;
            result = metrics_compute_signature_finish(&loop_signature.signature, N_iter, perf_accuracy_min_time, total_th);
            if (result == EAR_NOT_READY)
            {
                if (!lib_shared_region->master_ready) perf_count_period++;
                return;
            }


            /* Marked in metrics */
            //signature_ready(&sig_shared_region[my_node_id],SIGNATURE_STABLE);

            /*print_loop_signature("signature refreshed", &loop_signature.signature);*/

            //signature_copy(&loop.signature, &loop_signature.signature);
            metrics_node_signature(&loop_signature.signature,&loop.signature);
            report_loop_signature(iterations,&loop,&loop_signature.job);

            /* VERBOSE */
            if (dynais_used) sprintf(use_case,"D-Stable");
            else sprintf(use_case,"P-Stable");
            state_report_traces(masters_info.my_master_rank,ear_my_rank, my_id,&loop,prev_f,EAR_STATE);
            state_verbose_signature(&loop,masters_info.my_master_rank,show_signatures,ear_app_name,application.node_id,iterations,prev_f,0,use_case);
            /* END VERBOSE */

            begin_iter = iterations;
            if (sig_ready[curr_pstate] == 0){
                memcpy(&signatures[curr_pstate], &loop_signature, sizeof(application_t));
                sig_ready[curr_pstate]=1;
            }

            /* We must evaluate policy decissions */
            signatures_stables++;
            l_sig = &policy_sel_signature;
            if (sig_ready[def_pstate] == 0){
                verbose_master(2,"Signature at default freq not available, assuming policy ok");
                /* If default is not available, that means a dynamic configuration has been decided, we assume we are ok */
                if (sig_ready[curr_pstate] == 0){
                    memcpy(&signatures[curr_pstate], &loop_signature, sizeof(application_t));
                    sig_ready[curr_pstate] = 1;
                }
                return;
            }
            pst = policy_ok(&loop_signature.signature, l_sig,&pok);
            ok_clr = COL_GRE;
            if (pok){
                /* When collecting traces, we maintain the period */
                if ((masters_info.my_master_rank>=0) && (traces_are_on()==0)){
                    perf_count_period = perf_count_period * 2;
                }
                tries_current_loop=0;
            }else{
                /* Reset */
                policy_set_default_freq(&loop_signature.signature);
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
                ok_clr = COL_RED;
            }
            verbose_master(2,"%sPolicy ok=%u%s current (%.2lf/%.2lf/%.2lf) last/pol_sel (%.2lf/%.2lf/%.2lf)/(%.2lf/%.2lf/%.2lf)",ok_clr,pok,COL_CLR,
                    loop_signature.signature.CPI,loop_signature.signature.GBS,loop_signature.signature.DC_power,l_sig->CPI,l_sig->GBS,l_sig->DC_power,
                    policy_sel_signature.CPI,policy_sel_signature.GBS,policy_sel_signature.DC_power);
            if (pok) return;
            // We can give the library a second change in case we are at def freq
            if (((tries_current_loop<MAX_POLICY_TRIES) && (curr_pstate==def_pstate)) || (!dynais_used))
            {
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
                break;
            }


            /* If we cannot select an optimal frequency, We must report a problem and go to the default configuration*/
            if (tries_current_loop>=MAX_POLICY_TRIES){
                EAR_STATE = PROJECTION_ERROR;
                pst=policy_get_default_freq(&nf);
                policy_freq = nf.cpu_freq[0];
                if (masters_info.my_master_rank>=0){ 
                    log_report_max_tries(application.job.id,application.job.step_id, application.job.def_f);
                    traces_frequency(ear_my_rank, my_id, policy_freq);
                }
                break;
            }
            /** We are not going better **/
            /* If signature is different, we consider as a new loop case */
            //if (!policy_had_effect(&loop_signature.signature, &last_signature.signature))
            if (!policy_had_effect(&loop_signature.signature,l_sig))
            {
                EAR_STATE = SIGNATURE_HAS_CHANGED;
                comp_N_begin = metrics_time();
                policy_loop_init(&loop.id);
#if DYNAIS_CUTOFF
                check_dynais_on(&loop_signature.signature, l_sig);
#endif
            } else {
                EAR_STATE = EVALUATING_LOCAL_SIGNATURE;
            }
            break;
        case PROJECTION_ERROR:
            if (masters_info.my_master_rank>=0 && traces_are_on()){
                /* We compute the signature just in case EAR_GUI is on */
                if (((iterations - 1) % perf_count_period) ) return;
                /* We can compute the signature */
                N_iter = iterations - begin_iter;
                result = metrics_compute_signature_finish(&loop_signature.signature, N_iter, perf_accuracy_min_time, total_th);
                if (result == EAR_NOT_READY) {
                    perf_count_period++;
                    break;
                }
                /* VERBOSE */
                signature_copy(&loop.signature, &loop_signature.signature);
                state_report_traces(masters_info.my_master_rank,ear_my_rank, my_id,&loop,prev_f,EAR_STATE);
                state_verbose_signature(&loop,masters_info.my_master_rank,show_signatures,ear_app_name,application.node_id,iterations,prev_f,0,"D-PE");
            }
            /* We run here at default freq */
            break;
        default: break;
    }
    current_earl_state = EAR_STATE;
    state_report_traces_state(masters_info.my_master_rank,ear_my_rank, my_id,EAR_STATE);
    debug("End states_end_iteration");
}


