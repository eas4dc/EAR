/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_TYPES_JOB
#define _EAR_TYPES_JOB

#include <time.h>
#include <stdint.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/utils/serial_buffer.h>

// WARNING! This type is serialized through functions job_serialize and
// job_deserialize. If you want to add new types, make sure to update these
// functions too.
typedef struct job
{
	job_id 	 id; //ulong
	job_id 	 step_id; //ulong
#if WF_SUPPORT
    job_id 	 local_id;
#endif
	char 	 user_id[GENERIC_NAME];
	char 	 group_id[GENERIC_NAME];
	char 	 app_id[GENERIC_NAME];
	char     user_acc[GENERIC_NAME];
	char	 energy_tag[ENERGY_TAG_SIZE];
    time_t 	 start_time;
	time_t	 end_time;
	time_t 	 start_mpi_time;
	time_t	 end_mpi_time;
	char 	 policy[POLICY_NAME];
	double   th;
	ulong 	 procs; 
	job_type type; //uchar
	ulong    def_f;
} job_t;

// Function declarations

// MANAGEMENT

/** Initializes all job variables at 0 with the exception of def_f, policy, th and procs,
*	which take the value given by parameter. */ 
void init_job(job_t *job, ulong def_f, char *policy, double th, ulong procs);

/** Sets job->start_mpi_time to the current time */
void start_mpi(job_t *job);
/** Sets job->end_mpi_time to the current time */
void end_mpi(job_t *job);

/** Sets the job start time */
void start_job(job_t *job);
/** Sets the job end time */
void end_job(job_t *job);

/** Copies the source job given by parameter into the destiny job.*/
void copy_job(job_t *destiny, job_t *source);

/** Given a job_t and a file descriptor, outputs the contents of said job to the fd. */
void print_job_fd(int fd, job_t *job);

/** Reports the content of the job into the stderr*/
void report_job(job_t *job);

void print_job_fd_binary(int fd, job_t *job);
/** Memory is already allocated for the job */
void read_job_fd_binary(int fd, job_t *job);

void job_serialize(serial_buffer_t *b, job_t *job);

void job_deserialize(serial_buffer_t *b, job_t *job);

#endif
