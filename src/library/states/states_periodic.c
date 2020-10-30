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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/log.h>
#include <common/types/loop.h>
#include <common/types/application.h>
#include <common/output/verbose.h>
#include <common/math_operations.h>
#include <library/models/models.h>
#include <library/policies/policy.h>
#include <library/tracer/tracer.h>
#include <library/states/states.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/metrics/metrics.h>
#include <common/hardware/frequency.h>
#include <daemon/eard_api.h>

extern uint mpi_calls_in_period;
extern masters_info_t masters_info;
extern float ratio_PPN;

// static defines
#define FIRST_ITERATION			1
#define EVALUATING_SIGNATURE	2

static projection_t *PP;
static ulong policy_freq;
static int current_loop_id;
static ulong perf_accuracy_min_time = 1000000;
static uint EAR_STATE ;
static ulong global_f;
static loop_id_t periodic_loop;
static uint total_th;
void states_periodic_end_job(int my_id, FILE *ear_fd, char *app_name)
{
	if (my_id) return;
	debug("EAR(%s) Ends execution. \n", app_name);
	end_log();
}

void states_periodic_begin_job(int my_id, FILE *ear_fd, char *app_name)
{
	ulong   architecture_min_perf_accuracy_time;
	if (my_id) return;
	policy_freq = EAR_default_frequency;
    perf_accuracy_min_time = get_ear_performance_accuracy();
    if (masters_info.my_master_rank>=0) {
			architecture_min_perf_accuracy_time=eards_node_energy_frequency();
		}else{	
			architecture_min_perf_accuracy_time=1000000;
		}
    if (architecture_min_perf_accuracy_time>perf_accuracy_min_time) perf_accuracy_min_time=architecture_min_perf_accuracy_time;
	periodic_loop.event=1;
	periodic_loop.size=1;
	periodic_loop.level=1;
	total_th =  get_total_resources();
}

void states_periodic_begin_period(int my_id, FILE *ear_fd, unsigned long event, unsigned int size)
{
  if (loop_init(&loop,&loop_signature.job,event,size,1)!=EAR_SUCCESS){
    error("Error creating loop");
  }

	policy_loop_init(&periodic_loop);
	if (masters_info.my_master_rank>=0) traces_new_period(ear_my_rank, my_id, event);
	loop_with_signature = 0;
	EAR_STATE=FIRST_ITERATION;
	
}

void states_periodic_end_period(uint iterations)
{
	if (loop_with_signature)
	{
		loop.total_iterations = iterations;
		#if DB_FILES
		append_loop_text_file(loop_summary_path, &loop,&loop_signature.job);
		#endif
		#if USE_DB
		if (masters_info.my_master_rank>=0) eards_write_loop_signature(&loop);
		#endif
	}

	loop_with_signature = 0;
	policy_loop_end(&periodic_loop);
}


static void print_loop_signature(char *title, signature_t *loop)
{
	float avg_f = (float) loop->avg_f / 1000000.0;

	debug("(%s) Avg. freq: %.2lf (GHz), CPI/TPI: %0.3lf/%0.3lf, GBs: %0.3lf, DC power: %0.3lf, time: %0.3lf, GFLOPS: %0.3lf",
                title, avg_f, loop->CPI, loop->TPI, loop->GBS, loop->DC_power, loop->time, loop->Gflops);
}

void states_periodic_new_iteration(int my_id, uint period, uint iterations, uint level, ulong event, ulong mpi_calls_iter);
static void report_loop_signature(uint iterations,loop_t *loop)
{
   loop->total_iterations = iterations;
   #if DB_FILES
   append_loop_text_file(loop_summary_path, loop,&loop_signature.job);
	#endif
	#if USE_DB
    if (masters_info.my_master_rank>=0){ 
			eards_write_loop_signature(loop);
		}
    #endif
	
	
}

/*
*	MAIN FUNCTION: When EAR is executed in periodic mode, only first itereation and evaluating signature states are applied
*
*/
void states_periodic_new_iteration(int my_id, uint period, uint iterations, uint level, ulong event,ulong mpi_calls_iter)
{
	double CPI, TPI, GBS, POWER, TIME, ENERGY, EDP,VPI;
	ulong AVGF;
	float AVGFF,prev_ff,policy_freqf; 

	unsigned long long VI;
	unsigned long prev_f;
	int result;
	uint N_iter;
	int ready;
	state_t st;

	prev_f = ear_frequency;
	st=policy_new_iteration(&periodic_loop);
	if (resched_conf->force_rescheduling)
	{
		if (masters_info.my_master_rank>=0) traces_reconfiguration(ear_my_rank, my_id);
		debug("EAR: rescheduling forced by eard: max freq %lu new min_time_th %lf\n",
					system_conf->max_freq, system_conf->th);

		// We set the default number of iterations to the default for this loop
		resched_conf->force_rescheduling = 0;
	}

	switch (EAR_STATE)
	{
		case FIRST_ITERATION:
				EAR_STATE = EVALUATING_SIGNATURE;
				if (masters_info.my_master_rank>=0) traces_policy_state(ear_my_rank, my_id,EVALUATING_SIGNATURE);
				metrics_compute_signature_begin();
				// Loop printing algorithm
				loop.id.event = event;
				loop.id.level = level;
				loop.id.size = period;
				break;
		case EVALUATING_SIGNATURE:
				N_iter=1;
				
				result = metrics_compute_signature_finish(&loop_signature.signature, N_iter, perf_accuracy_min_time, total_th);	

				if (result != EAR_NOT_READY)
				{
					//print_loop_signature("signature computed", &loop_signature.signature);

					loop_with_signature = 1;
					current_loop_id = event;

					CPI = loop_signature.signature.CPI;
					GBS = loop_signature.signature.GBS;
					POWER = loop_signature.signature.DC_power;
					TPI = loop_signature.signature.TPI;
					TIME = loop_signature.signature.time;
					AVGF = loop_signature.signature.avg_f;

		      VI=metrics_vec_inst(&loop_signature.signature);
		      VPI=(double)VI/(double)loop_signature.signature.instructions;

					ENERGY = TIME * POWER;
					EDP = ENERGY * TIME;
		      signature_t app_signature;
      		adapt_signature_to_node(&app_signature,&loop_signature.signature,ratio_PPN);
					st=policy_apply(&app_signature,&policy_freq,&ready);
					signature_ready(&sig_shared_region[my_node_id],EVALUATING_SIGNATURE);
					loop_signature.signature.def_f=prev_f;
					if (policy_freq != prev_f){
						if (masters_info.my_master_rank>=0) log_report_new_freq(application.job.id,application.job.step_id,policy_freq);
					}
					/* For no masters, ready will be 0, pending */
					if (masters_info.my_master_rank>=0){
					traces_new_signature(ear_my_rank, my_id,&loop_signature.signature);
					traces_frequency(ear_my_rank, my_id, policy_freq);
					traces_policy_state(ear_my_rank, my_id,EVALUATING_SIGNATURE);
					}

					if (masters_info.my_master_rank>=0){
        		AVGFF=(float)AVGF/1000000.0; 
        		prev_ff=(float)prev_f/1000000.0; 
        		policy_freqf=(float)policy_freq/1000000.0; 
						verbose(1, "\n\nEAR+P(%s) at %.2f:App. Signature (CPI=%.3lf GBS=%.2lf Power=%.1lf Time=%.3lf Energy=%.2lfJ AVGF=%.2f)--> New frequency selected %.2f\n",
									ear_app_name, prev_ff, CPI, GBS, POWER, TIME, ENERGY, AVGFF,
									policy_freqf);
					}	
					// Loop printing algorithm
					signature_copy(&loop.signature, &loop_signature.signature);
					report_loop_signature(iterations,&loop);
				}
				else{ /* Time was not enough */
					if (loop_signature.signature.time==0){
						mpi_calls_in_period=mpi_calls_in_period*2;
					}
				}

			break;
		default: break;
	}
}


