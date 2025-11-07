/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_LOG_H
#define _EAR_LOG_H

#include <common/config.h>
#include <common/types/event_type.h>
#include <common/types/generic.h>
#include <common/types/job.h>
#include <time.h>

/** Creates the log file and starts it with the current time. If it can't
 *   create the file it reports it to stderr */
void init_log();
/** Finishes the log with the current time and closes the log file */
void end_log();

/** Given an event, it reports it ot the log file*/
void report_new_event(ear_event_t *event);

/** Given a job id and a frequency value, reports to the log file the change
 *   of frequency because of the energy policy */
void log_report_new_freq(job_id id, job_id step_id, ulong newf);
/** Given a job id, reports to the log file that the DynAIS has been turned off */
void log_report_dynais_off(job_id id, job_id step_id);
/** Given a job id and a frequency value, reports to the log file the change
 *   of frequency because of a policy projections failure */
void log_report_max_tries(job_id id, job_id step_id, ulong newf);
/** Given a job id and a frequency value, reports to the log file the change
 *   of frequency because of Energy Budget*/
void log_report_global_policy_freq(job_id id, job_id step_id, ulong newf);

#endif
