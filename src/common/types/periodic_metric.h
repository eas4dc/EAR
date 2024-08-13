/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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

/* Cleans pm data to avoid out of range values */
void periodic_metric_clean_before_db(periodic_metric_t *pm);

#endif
