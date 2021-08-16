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

#ifndef _EARD_LOG_H
#define _EARD_LOG_H

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/job.h>
#include <common/types/event_type.h>




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






/** Creates the log file and starts it with the current time. If it can't
 *   create the file it reports it to stderr */
void init_eard_rt_log();



/** Sets the minimum interval between two runtime errors to avoid saturating the DB */
void log_report_eard_min_interval(uint secs);
/** Reports a RT error from EARD */
void log_report_eard_rt_error(uint usedb,uint useeardbd,job_id job,job_id sid,uint eventid,ulong value);
/* Reports and error when initializing EARD */
void log_report_eard_init_error(uint usedb,uint useeardbd,uint eventid,ulong value);
/* Reports powercap events from EARD */
void log_report_eard_powercap_event(uint usedb,uint useeardbd,job_id job,job_id sid,uint eventid,ulong value);

void log_report_eard_powercap_event(uint usedb,uint useeardbd,job_id job,job_id sid,uint eventid,ulong value);


#endif
