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

#ifndef _EAR_TYPES_PERIODIC_METRIC
#define _EAR_TYPES_PERIODIC_METRIC

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>

typedef struct periodic_metric
{
		/* short */
    ulong DC_energy;
    ulong job_id;
    ulong step_id;
    time_t start_time;
    time_t end_time;
    char node_id[NODE_SIZE];
		/* Full */
    ulong avg_f;
    ulong temp;
    ulong DRAM_energy;
    ulong PCK_energy;
#if USE_GPUS
    ulong GPU_energy;
#endif
} periodic_metric_t;

/** Initializes all values of the periodic_metric to 0 , sets the nodename */
void init_periodic_metric(periodic_metric_t *pm);

/** Replicates the periodic_metric in *source to *destiny */
void copy_periodic_metric(periodic_metric_t *destiny, periodic_metric_t *source);

/** Cleaned remake of the classic print 'fd' function */
void periodic_metrict_print_channel(FILE *file, periodic_metric_t *pm);

/** Modifies jid, sid at new job */
void new_job_for_period(periodic_metric_t *pm,ulong job_id, ulong s_id);

/** Sets to null job and sid */
void end_job_for_period(periodic_metric_t *pm);

/*
 *
 * Obsolete?
 *
 */

/** Sets the time for a new period */
void init_sample_period(periodic_metric_t *pm);

/** Sets the end time for the period and the energy */
void end_sample_period(periodic_metric_t *pm,ulong energy);

#endif
