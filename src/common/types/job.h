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

#ifndef _EAR_TYPES_JOB
#define _EAR_TYPES_JOB

#include <time.h>
#include <stdint.h>
#include <common/config.h>
#include <common/types/generic.h>

/* Proposal for new job
typedef struct job
{
    job_id  id;
    job_id  step_id;
    char    user_id[UID_NAME];
    char    app_id[GENERIC_NAME];
    char    user_acc[GENERIC_NAME];
	char	energy_tag[ENEGY_TAG_SIZE];
    time_t  start_time;
    time_t  end_time;
    time_t  start_mpi_time;
    time_t  end_mpi_time;
    char    policy[POLICY_NAME];
    double  th;
    ulong   procs;
    job_type    type;
    ulong       def_f;
} job_t;
*/
typedef struct job
{
	job_id 	id;
	job_id 	step_id;
	char 	user_id[GENERIC_NAME];
	char 	group_id[GENERIC_NAME];
	char 	app_id[GENERIC_NAME];
	char    user_acc[GENERIC_NAME];
	char	energy_tag[ENERGY_TAG_SIZE];
    time_t 	start_time;
	time_t	end_time;
	time_t 	start_mpi_time;
	time_t	end_mpi_time;
	char 	policy[POLICY_NAME];
	double  th;
	ulong 	procs; 
	job_type	type;	
	ulong 		def_f;
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

/** Given a job_t and a file descriptor, outputs the contents of said job to the fd.*/
void print_job_fd(int fd, job_t *job);

/** Reports the content of the job into the stderr*/
void report_job(job_t *job);

void print_job_fd_binary(int fd, job_t *job);
/** Memory is already allocated for the job */
void read_job_fd_binary(int fd, job_t *job);

#endif
