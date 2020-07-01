/**************************************************************
*	Energy Aware Runtime (EA)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <metrics/stalls/cpu/papi.h>

#define STALL_SETS 		1
#define STALL_EVS 		2

static const char *PAPI_NAME = "METRICS_STALLs";
static long long values[STALL_SETS][STALL_EVS];
static long long acum_values[STALL_EVS];
static int event_sets[STALL_SETS];

int init_stall_metrics()
{
	PAPI_option_t attach_opt[STALL_SETS];
	int events, sets, cid, ret;

	//
	PAPI_INIT_TEST(PAPI_NAME);
	PAPI_INIT_MULTIPLEX_TEST(PAPI_NAME);
	PAPI_GET_COMPONENT(cid, "perf_event", PAPI_NAME);

	//
	for (sets = 0; sets < STALL_SETS; sets++)
	{
		acum_values[sets]=0;
		/* Event values set to 0 */
		for (events=0;events<STALL_EVS;events++) {
			values[sets][events]=0;
		}
		/* Init event sets */
	    	event_sets[sets]=PAPI_NULL;
	        if ((ret=PAPI_create_eventset(&event_sets[sets]))!=PAPI_OK){
			verbose(0, "Creating %d eventset.Exiting:%s", sets,PAPI_strerror(ret));
			return EAR_ERROR;
		}

		if ((ret=PAPI_assign_eventset_component(event_sets[sets],cid))!=PAPI_OK){		
			verbose(0, "PAPI_assign_eventset_component.Exiting:%s", PAPI_strerror(ret));
			return EAR_ERROR;
		}
		attach_opt[sets].attach.eventset=event_sets[sets];
 		attach_opt[sets].attach.tid=getpid();

		if ((ret=PAPI_set_opt(PAPI_ATTACH,(PAPI_option_t *) &attach_opt[sets]))!=PAPI_OK){
			verbose(0,  "PAPI_set_opt.%s", PAPI_strerror(ret));
		}

		ret = PAPI_set_multiplex(event_sets[sets]);

		if ((ret == PAPI_EINVAL) && (PAPI_get_multiplex(event_sets[sets]) > 0)){
			verbose(0,  "Event set to compute STALL already has multiplexing enabled");
		}else if (ret != PAPI_OK){ 
			verbose(0,  "Error , event set to compute STALL can not be multiplexed %s",PAPI_strerror(ret));
		}

		ret = PAPI_add_named_event(event_sets[sets], "CYCLE_ACTIVITY:STALLS_TOTAL");

		if (ret != PAPI_OK){
			verbose(0,  "PAPI_add_named_event %s.%s","CYCLE_ACTIVITY:STALLS_TOTAL",PAPI_strerror(ret));
		}
	}
	return EAR_SUCCESS;
}
void reset_stall_metrics()
{
	int sets,events,ret;

	for (sets=0;sets<STALL_SETS;sets++)
	{
		if ((ret=PAPI_reset(event_sets[sets]))!=PAPI_OK)
		{
			verbose(0,  "reset_stall_metrics set %d.%s", event_sets[sets],
					PAPI_strerror(ret));
		}
		for (events=0;events<STALL_EVS;events++) {
			values[sets][events]=0;
		}
	}
}
void start_stall_metrics()
{
	int sets,ret;
	for (sets=0;sets<STALL_SETS;sets++)
	{
		if ((ret=PAPI_start(event_sets[sets]))!=PAPI_OK)
		{
			verbose(0, "start_stall_metrics set %d .%s", event_sets[sets],
				PAPI_strerror(ret));
		}
	}
}
/* Stops includes accumulate metrics */
void stop_stall_metrics(long long *stall_cycles)
{
	int sets, ret;
	*stall_cycles=0;
	// There is a single event set for stall instruction
	for (sets=0;sets<STALL_SETS;sets++)
	{
		ret = PAPI_stop(event_sets[sets], (long long *) &values[sets]);
		if (ret != PAPI_OK){
			verbose(0, "stall_metrics: stop_stall_metrics set %d.%s",
				event_sets[sets], PAPI_strerror(ret));
		} else {
			*stall_cycles=values[sets][0];
			acum_values[0]=acum_values[0]+values[sets][0];
		}
	}
}

void get_stall_metrics(long long *total_stall_cycles)
{
	*total_stall_cycles=acum_values[0];
}

