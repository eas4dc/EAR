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

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
