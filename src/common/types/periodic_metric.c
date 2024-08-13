/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <common/config.h>
#include <common/math_operations.h>
#include <common/types/periodic_metric.h>

void copy_periodic_metric(periodic_metric_t *destiny, periodic_metric_t *source)
{
    memcpy(destiny, source, sizeof(periodic_metric_t));
}

void init_periodic_metric(periodic_metric_t *pm)
{
    memset(pm, 0, sizeof(periodic_metric_t));
	gethostname(pm->node_id,NODE_SIZE);
	strtok(pm->node_id,".");
	pm->job_id=0;
	pm->step_id=0;	
}

void new_job_for_period(periodic_metric_t *pm,ulong job_id, ulong s_id)
{
	pm->job_id=job_id;
	pm->step_id=s_id;	
}

void end_job_for_period(periodic_metric_t *pm)
{
	pm->job_id=0;
	pm->step_id=0;	
}

void periodic_metrict_print_channel(FILE *file, periodic_metric_t *pm)
{
	fprintf(file, "periodic_metric_t: job id '%lu.%lu', node id '%s'\n",
		pm->job_id, pm->step_id, pm->node_id);
	fprintf(file, "periodic_metric_t: start '%lu', end '%lu', energy '%lu' avg_freq %lu temp %lu\n",
		pm->start_time, pm->end_time, pm->DC_energy,pm->avg_f,pm->temp);
}

/*
 *
 * Obsolete?
 *
 */

void init_sample_period(periodic_metric_t *pm)
{
	time(&pm->start_time);
}

void end_sample_period(periodic_metric_t *pm,ulong energy)
{
	time(&pm->end_time);
	pm->DC_energy=energy;
}

void periodic_metric_clean_before_db(periodic_metric_t *pm)
{
	if (pm->DC_energy > INT_MAX) pm->DC_energy = INT_MAX;
	if (pm->avg_f     > INT_MAX) pm->avg_f     = INT_MAX;
	if (pm->temp      > INT_MAX) pm->temp      = INT_MAX;
	if (pm->DRAM_energy > INT_MAX) pm->DRAM_energy = INT_MAX;
	if (pm->PCK_energy  > INT_MAX) pm->PCK_energy  = INT_MAX;
#if USE_GPUS
	if (pm->GPU_energy  > INT_MAX) pm->GPU_energy  = INT_MAX;
#endif
}
