/**************************************************************
*	Energy Aware Runtime (EAR)
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
