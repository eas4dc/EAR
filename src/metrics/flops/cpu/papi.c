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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/hardware/hardware_info.h>
//#include <common/environment.h>
#include <metrics/flops/cpu/papi.h>

#define FLOPS_SETS		2
#define FLOPS_EVS		4
#define SP_OPS			0
#define DP_OPS			1

#define FP_ARITH_INST_RETIRED_PACKED_SINGLE_N		"FP_ARITH:SCALAR_SINGLE"
#define FP_ARITH_INST_RETIRED_128B_PACKED_SINGLE_N	"FP_ARITH:128B_PACKED_SINGLE"
#define FP_ARITH_INST_RETIRED_256B_PACKED_SINGLE_N	"FP_ARITH:256B_PACKED_SINGLE"
#define FP_ARITH_INST_RETIRED_512B_PACKED_SINGLE_N	"FP_ARITH:512B_PACKED_SINGLE"
#define FP_ARITH_INST_RETIRED_SCALAR_DOUBLE_N 		"FP_ARITH:SCALAR_DOUBLE"
#define FP_ARITH_INST_RETIRED_128B_PACKED_DOUBLE_N	"FP_ARITH:128B_PACKED_DOUBLE"
#define FP_ARITH_INST_RETIRED_256B_PACKED_DOUBLE_N	"FP_ARITH:256B_PACKED_DOUBLE"
#define FP_ARITH_INST_RETIRED_512B_PACKED_DOUBLE_N	"FP_ARITH:512B_PACKED_DOUBLE"

static const char *PAPI_NAME = "FLOPS";
static int weights[FLOPS_SETS][FLOPS_EVS] = {{1,4,8,16}, {1,2,4,8}};
static long long acum_values[FLOPS_SETS][FLOPS_EVS];
static long long values[FLOPS_SETS][FLOPS_EVS];
static int event_sets[FLOPS_SETS];
static int flops_supported = 0;

int init_flops_metrics()
{
	PAPI_option_t attach_ops[FLOPS_SETS];
	int retval, cpu_model, cid;
	int sets, events;

	PAPI_INIT_TEST(PAPI_NAME);
	PAPI_INIT_MULTIPLEX_TEST(PAPI_NAME);
	PAPI_GET_COMPONENT(cid, "perf_event", PAPI_NAME);

	for (sets = 0; sets < FLOPS_SETS; sets++)
	{
		/* Event values set to 0 */
		for (events=0;events<FLOPS_EVS;events++) {
			acum_values[sets][events]=0;
			values[sets][events]=0;
		}
		/* Init event sets */
		event_sets[sets]=PAPI_NULL;
		if ((retval = PAPI_create_eventset(&event_sets[sets])) != PAPI_OK) {
			error("FP_METRICS: Creating %d eventset.%s", sets, PAPI_strerror(retval));
			return flops_supported;
		}

		if ((retval=PAPI_assign_eventset_component(event_sets[sets],cid)) != PAPI_OK){
			error("FP_METRICS: PAPI_assign_eventset_component.%s", PAPI_strerror(retval));
			return flops_supported;
		}

		attach_ops[sets].attach.eventset=event_sets[sets];
 		attach_ops[sets].attach.tid=getpid();
		if ((retval=PAPI_set_opt(PAPI_ATTACH,(PAPI_option_t*) &attach_ops[sets]))!=PAPI_OK){
			error( "FP_METRICS: PAPI_set_opt.%s", PAPI_strerror(retval));
		}
		retval = PAPI_set_multiplex(event_sets[sets]);
		if ((retval == PAPI_EINVAL) && (PAPI_get_multiplex(event_sets[sets]) > 0)){
			error("FP_METRICS: Event set to compute FLOPS already has multiplexing enabled");
		}else if (retval != PAPI_OK){ 
			error("FP_METRICS: Error , event set to compute FLOPS can not be multiplexed %s",PAPI_strerror(retval));
		}

		cpu_model = get_model();
		switch(cpu_model){
			case MODEL_HASWELL_X:
			case MODEL_BROADWELL_X:
				break;
			case MODEL_SKYLAKE_X:
				flops_supported=1;

				switch (sets)
				{
				case SP_OPS:
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_PACKED_SINGLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_PACKED_SINGLE_N,PAPI_strerror(retval));
				}
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_128B_PACKED_SINGLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_128B_PACKED_SINGLE_N,PAPI_strerror(retval));
				}
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_256B_PACKED_SINGLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_256B_PACKED_SINGLE_N,PAPI_strerror(retval));
				}
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_512B_PACKED_SINGLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_512B_PACKED_SINGLE_N,PAPI_strerror(retval));
				}
				
				break;
				case DP_OPS:
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_SCALAR_DOUBLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_SCALAR_DOUBLE_N,PAPI_strerror(retval));
				}
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_128B_PACKED_DOUBLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_128B_PACKED_DOUBLE_N,PAPI_strerror(retval));
				}
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_256B_PACKED_DOUBLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_256B_PACKED_DOUBLE_N,PAPI_strerror(retval));
				}
				if ((retval=PAPI_add_named_event(event_sets[sets],FP_ARITH_INST_RETIRED_512B_PACKED_DOUBLE_N))!=PAPI_OK){
					error("FP_METRICS: PAPI_add_named_event %s.%s",FP_ARITH_INST_RETIRED_512B_PACKED_DOUBLE_N,PAPI_strerror(retval));
				}
				debug("FP_METRICS: PAPI_DP_OPS added to set %d",sets);
				break;
				}
		}
		
    }
	if (flops_supported) {
		debug( "FP_METRICS:Computation of Flops initialized");
	} else {
		debug( "FP_METRICS: Computation of Flops not supported");
	}
	return flops_supported;
}
void reset_flops_metrics()
{
	int sets,events,retval;
	if (!flops_supported) return;
	for (sets=0;sets<FLOPS_SETS;sets++){
		if ((retval=PAPI_reset(event_sets[sets]))!=PAPI_OK){
			error("FP_METRICS: ResetFlopsMetrics.%s", PAPI_strerror(retval));
		}
		for (events=0;events<FLOPS_EVS;events++) values[sets][events]=0;
		
	}
}
void start_flops_metrics()
{
	int sets,retval;
	if (!flops_supported) return;
	for (sets=0;sets<FLOPS_SETS;sets++){
		if ((retval=PAPI_start(event_sets[sets]))!=PAPI_OK){
			error("FP_METRICS:StartFlopsMetrics.%s", PAPI_strerror(retval));
		}
	}
}
/* Stops includes accumulate metrics */
void stop_flops_metrics(long long *flops, long long *f_operations)
{
	int sets,ev,retval;

	*flops = 0;
	memset(f_operations,0,sizeof(long long)*FLOPS_SETS*FLOPS_EVS);
	if (!flops_supported) return;

	for (sets=0;sets<FLOPS_SETS;sets++)
	{
		if ((retval=PAPI_read(event_sets[sets],(long long *)&values[sets]))!=PAPI_OK)
		{
			error("FP_METRICS: StopFlopsMetrics.%s",PAPI_strerror(retval));
		} else
		{
			for (ev=0; ev < FLOPS_EVS;ev++)
			{
				f_operations[sets*FLOPS_EVS+ev] = values[sets][ev];

				acum_values[sets][ev] += values[sets][ev];

				*flops += (values[sets][ev] * weights[sets][ev]);
			}
		}
	}
}

void print_gflops(long long total_inst,unsigned long total_time,uint total_cores)
{
	int sets,ev;
	long long total=0;
	if (!flops_supported) return;
	for (sets=0;sets<FLOPS_SETS;sets++)
        {
		if (sets==SP_OPS){
			debug("SP FOPS:");
		}else{
			debug("DP FOPS");
		}
		for (ev=0;ev<FLOPS_EVS;ev++){
			total=total+(weights[sets][ev]*acum_values[sets][ev]);
		}
		if (total>0){

		for (ev=0;ev<FLOPS_EVS;ev++){
			debug("[%d]=%llu x %d (%lf %%), ", ev, acum_values[sets][ev], weights[sets][ev],
			(double)(weights[sets][ev]*acum_values[sets][ev]*100)/(double)total);
		}
		}
	}
	debug("GFlops per process = %.3lf ", (double)(total)/(double)(total_time*1000));
	debug("GFlops per node    = %.3lf ", (double)(total*total_cores)/(double)(total_time*1000));
	
}
double gflops(unsigned long total_time,uint total_cores)
{
        int sets,ev;
        long long total=0;
	double Gflops;
        if (!flops_supported) return (double)0;
        for (sets=0;sets<FLOPS_SETS;sets++){
                for (ev=0;ev<FLOPS_EVS;ev++){
                        total=total+(weights[sets][ev]*acum_values[sets][ev]);
                }
        }
	Gflops=(double)(total*total_cores)/(double)(total_time*1000);
	return Gflops;
}

int get_number_fops_events()
{
	return (FLOPS_SETS * FLOPS_EVS);
}

void get_weigth_fops_instructions(int *weigth_vector)
{
	int sets,ev;
	for (sets=0;sets<FLOPS_SETS;sets++){
		for (ev=0;ev<FLOPS_EVS;ev++){
			weigth_vector[sets*FLOPS_EVS+ev]=weights[sets][ev];
		}
	}
}

void get_total_fops(long long *metrics)
{
	int sets, ev, i;

	if (!flops_supported) return;

	for (sets=0;sets < FLOPS_SETS; sets++)
	{
		for (ev=0;ev < FLOPS_EVS; ev++)
		{
			i = sets * FLOPS_EVS + ev;
			metrics[i] = acum_values[sets][ev];
		}
	}
}
