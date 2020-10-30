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
#include <string.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/hardware/hardware_info.h>
#include <metrics/common/papi.h>
#include <metrics/cache/cpu/papi.h>

#define L1 0
#define L2 1
#define L3 2

/*
 * PAPI_L2_DCM  0x80000002  Yes   Yes  Level 2 data cache misses
 * PAPI_L2_ICM  0x80000003  Yes   No   Level 2 instruction cache misses
 * PAPI_L2_TCM  0x80000007  Yes   No   Level 2 cache misses
 * PAPI_L2_LDM  0x80000019  Yes   No   Level 2 load misses
 * PAPI_L2_STM  0x8000001a  Yes   No   Level 2 store misses
 * PAPI_L2_DCA  0x80000041  Yes   No   Level 2 data cache accesses
 * PAPI_L2_DCR  0x80000044  Yes   No   Level 2 data cache reads
 * PAPI_L2_DCW  0x80000047  Yes   Yes  Level 2 data cache writes
 * PAPI_L2_ICH  0x8000004a  Yes   No   Level 2 instruction cache hits
 * PAPI_L2_ICA  0x8000004d  Yes   No   Level 2 instruction cache accesses
 * PAPI_L2_ICR  0x80000050  Yes   No   Level 2 instruction cache reads
 * PAPI_L2_TCA  0x80000059  Yes   Yes  Level 2 total cache accesses
 * PAPI_L2_TCR  0x8000005c  Yes   Yes  Level 2 total cache reads
 * PAPI_L2_TCW  0x8000005f  Yes   Yes  Level 2 total cache writes
 * PAPI_L3_TCM  0x80000008  Yes   No   Level 3 cache misses
 * PAPI_L3_LDM  0x8000000e  Yes   No   Level 3 load misses
 * PAPI_L3_DCA  0x80000042  Yes   Yes  Level 3 data cache accesses
 * PAPI_L3_DCR  0x80000045  Yes   No   Level 3 data cache reads
 * PAPI_L3_DCW  0x80000048  Yes   No   Level 3 data cache writes
 * PAPI_L3_ICA  0x8000004e  Yes   No   Level 3 instruction cache accesses
 * PAPI_L3_ICR  0x80000051  Yes   No   Level 3 instruction cache reads
 * PAPI_L3_TCA  0x8000005a  Yes   No   Level 3 total cache accesses
 * PAPI_L3_TCR  0x8000005d  Yes   Yes  Level 3 total cache reads
 * PAPI_L3_TCW  0x80000060  Yes   No   Level 3 total cache writes
 */

static const char *PAPI_NAME = "METRICS_CACHE";
static long long acum_values[CACHE_SETS];
static long long values[CACHE_SETS][CACHE_EVS];
static int event_sets[CACHE_SETS];
char *CACHE_EVENTS_NAME[CACHE_SETS][CACHE_EVS]={{"PAPI_L2_TCA","PAPI_L3_TCA"},{"PAPI_L2_TCM","PAPI_L2_DCA"},{"PAPI_L3_TCM","PAPI_L3_DCA"}};

void print_cache_metrics(long long *L)
{
	int i,j;
	for (i=0;i<CACHE_SETS;i++){
		for (j=0;j<CACHE_EVS;j++) printf("%s=%lld\n",CACHE_EVENTS_NAME[i][j],L[i*CACHE_SETS+j]);
	}
}
int init_cache_metrics()
{
	PAPI_option_t cache_attach_opt[CACHE_SETS];
	int ret, cid, sets, events;

	PAPI_INIT_TEST(PAPI_NAME);
	PAPI_INIT_MULTIPLEX_TEST(PAPI_NAME);
	PAPI_GET_COMPONENT(cid, "perf_event", PAPI_NAME);

	for (sets = 0; sets < CACHE_SETS; sets++)
	{
		acum_values[sets] = 0;

		/* Event values set to 0 */
		for (events=0;events<CACHE_EVS;events++) {
			values[sets][events]=0;
		}

		/* Init event sets */
		event_sets[sets]=PAPI_NULL;
		if ((ret = PAPI_create_eventset(&event_sets[sets])) != PAPI_OK) {
			error("ERROR while creating %d eventset (%s), exiting" ,sets, PAPI_strerror(ret));
		}

		if ((ret = PAPI_assign_eventset_component(event_sets[sets],cid)) != PAPI_OK){
			error("ERROR while assigning event set component (%s), exiting", PAPI_strerror(ret));
		}

		cache_attach_opt[sets].attach.eventset = event_sets[sets];
 		cache_attach_opt[sets].attach.tid = getpid();

		ret = PAPI_set_opt(PAPI_ATTACH,(PAPI_option_t*)&cache_attach_opt[sets]);
		if (ret != PAPI_OK) {
			error( "ERROR while setting option (%s)", PAPI_strerror(ret));
		}

		ret = PAPI_set_multiplex(event_sets[sets]);

		if ((ret == PAPI_EINVAL) && (PAPI_get_multiplex(event_sets[sets]) > 0)){
			error( "cache events sets already has multiplexing enabled");
		}else if (ret != PAPI_OK){ 
			error("ERROR, cache events sets can not be multiplexed (%s)", PAPI_strerror(ret));
		}

			int i;
			for (i=0;i<CACHE_EVS;i++){
				if (strcmp(CACHE_EVENTS_NAME[sets][i],"NOT_USED")){
					if ((ret=PAPI_add_named_event(event_sets[sets], CACHE_EVENTS_NAME[sets][i]))!=PAPI_OK){
        		error("PAPI_add_named_event %s (%s)",CACHE_EVENTS_NAME[sets][i],PAPI_strerror(ret));
					}
					debug("Added event name %s",CACHE_EVENTS_NAME[sets][i]);
				}else{
					debug("Not Added event name %s",CACHE_EVENTS_NAME[sets][i]);
				}
			}
	}

	return EAR_SUCCESS;
}
void reset_cache_metrics()
{
	int sets,events,ret;
	for (sets=0;sets<CACHE_SETS;sets++)
	{
		if ((ret=PAPI_reset(event_sets[sets]))!=PAPI_OK)
		{
			error( "reset_cache_metrics (%s)", PAPI_strerror(ret));
		}
		for (events = 0; events < CACHE_EVS; events++) {
			values[sets][events] = 0;
		}
		
	}
}
void start_cache_metrics()
{
	int sets,ret;

	for (sets=0;sets<CACHE_SETS;sets++)
	{
		if ((ret=PAPI_start(event_sets[sets])) != PAPI_OK)
		{
			error("start_cache_metrics (%s)", PAPI_strerror(ret));
		}
	}
}
/* Stops includes accumulate metrics */
void stop_cache_metrics(long long *L_data)
{
	int sets, ret;
	int i,j;
  for (i=0;i<CACHE_SETS;i++){
    for (j=0;j<CACHE_EVS;j++){ 
			L_data[i*CACHE_EVS+j]=0;
			values[i][j]=0;	
		}
	}
	for (sets=0;sets<CACHE_SETS;sets++){
		if ((ret=PAPI_stop(event_sets[sets],(long long *)&values[sets]))!=PAPI_OK){
			error( "stop_cache_metrics (%s)",PAPI_strerror(ret));

		}else{
			memcpy(&L_data[sets*CACHE_EVS],(long long *)&values[sets],sizeof(long long)*CACHE_EVS);
			for (j=0;j<CACHE_EVS;j++) acum_values[sets]+=values[sets][j];
		}
	}
}

