/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_LOG_H
#define _EARD_LOG_H

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/job.h>
#include <common/types/event_type.h>

#include <report/report.h>



#if 0
/* EARD Init events */

#define PM_CREATION_ERROR       100
#define APP_API_CREATION_ERROR  101
#define DYN_CREATION_ERROR      102
#define UNCORE_INIT_ERROR       103
#define RAPL_INIT_ERROR         104
#define ENERGY_INIT_ERROR       105
#define CONNECTOR_INIT_ERROR    106
#define RCONNECTOR_INIT_ERROR   107



/* EARD runtime events */
#define DC_POWER_ERROR		300
#define TEMP_ERROR			301
#define FREQ_ERROR			302
#define RAPL_ERROR			303
#define GBS_ERROR			304
#define CPI_ERROR			305

/* EARD powercap events */
#define POWERCAP_VALUE  500
#define RESET_POWERCAP  501
#define INC_POWERCAP    502
#define RED_POWERCAP    503
#define SET_POWERCAP    504
#define SET_ASK_DEF     505
#define RELEASE_POWER   506


#define EARD_RT_ERRORS  6
#define FIRST_RT_ERROR  300

#endif

/** Creates the log file and starts it with the current time.
 * If it can't create the file it reports it to stderr. */
void init_eard_rt_log();

/** Sets the minimum interval between two runtime errors to avoid saturating the DB */
void log_report_eard_min_interval(uint secs);
/** Reports a RT error from EARD */
void log_report_eard_rt_error(report_id_t *rid, job_id job, job_id sid, uint eventid, llong value);
/* Reports and error when initializing EARD */
void log_report_eard_init_error(report_id_t *rid, uint eventid, llong value);
/* Reports powercap events from EARD */
void log_report_eard_powercap_event(report_id_t *rid, job_id job, job_id sid, uint eventid, llong value);

#endif // _EARD_LOG_H
