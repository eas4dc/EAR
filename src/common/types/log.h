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

#ifndef _EAR_LOG_H
#define _EAR_LOG_H

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/job.h>
#include <common/types/event_type.h>

#define ENERGY_POLICY_NEW_FREQ	0
#define GLOBAL_ENERGY_POLICY	1
#define ENERGY_POLICY_FAILS		2
#define DYNAIS_OFF				3





/** Creates the log file and starts it with the current time. If it can't
*   create the file it reports it to stderr */
void init_log();
/** Finishes the log with the current time and closes the log file */
void end_log();

/** Given an event, it reports it ot the log file*/
void report_new_event(ear_event_t *event);

/** Given a job id and a frequency value, reports to the log file the change
*   of frequency because of the energy policy */
void log_report_new_freq(job_id id,job_id step_id,ulong newf);
/** Given a job id, reports to the log file that the DynAIS has been turned off */
void log_report_dynais_off(job_id id,job_id step_id);
/** Given a job id and a frequency value, reports to the log file the change
*   of frequency because of a policy projections failure */
void log_report_max_tries(job_id id,job_id step_id,ulong newf);
/** Given a job id and a frequency value, reports to the log file the change
*   of frequency because of Energy Budget*/
void log_report_global_policy_freq(job_id id,job_id step_id,ulong newf);



#endif
