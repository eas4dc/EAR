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
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <common/types/job.h>
#include <common/output/verbose.h>
#include <common/system/time.h>

void init_job(job_t *job, ulong def_f, char *policy, double th, ulong procs)
{
    memset(job, 0, sizeof(job_t));
    job->def_f = def_f;
    strcpy(job->policy, policy);
    job->th = th;
    job->procs = procs;
}
void start_job(job_t *job)
{
	time(&job->start_time);
}

void end_job(job_t *job)
{
    time(&job->end_time);
}

void start_mpi(job_t *job)
{
		time_t j_time;
    j_time=time(NULL);
		job->start_mpi_time=j_time;
}

void end_mpi(job_t *job)
{
		time_t j_time;
    j_time=time(NULL);
		job->end_mpi_time=j_time;
}

void copy_job(job_t *destiny, job_t *source)
{
    memcpy(destiny, source, sizeof(job_t));
}

void print_job_fd(int fd, job_t *job)
{
	char job_buff[4096];
	assert(job!=NULL);
	if ((job->user_id==NULL) || (job->app_id==NULL) || (job->policy==NULL)){
		verbose(VTYPE,"print_job_fd some of the args are null\n");
		return;
	}
//    sprintf(job_buff, "%s;%s;%lu;%s;%s;%lf", job->user_id, job->group_id,job->id, job->app_id, job->policy, job->th);
    if (strlen(job->group_id) < 1) strcpy(job->group_id, " ");
    if (strlen(job->energy_tag) < 1) strcpy(job->energy_tag, " ");
    if (strlen(job->user_acc) < 1) strcpy(job->user_acc, " ");
    sprintf(job_buff, "%lu;%lu;%s;%s;%s;%s;%s;%s;%lf", job->id, job->step_id, job->user_id, job->group_id, job->app_id,
                                                          job->user_acc, job->energy_tag, job->policy, job->th);
	write(fd,job_buff,strlen(job_buff));
}


/** Reports the content of the job into the stderr*/
void report_job(job_t *job)
{
	verbose(VTYPE,"Job: ID %lu step %lu user %s group %s name %s account %s etag %s\n",
	job->id,job->step_id,job->user_id,job->group_id,job->app_id,job->user_acc,job->energy_tag);
	verbose(VTYPE,"start time %ld end time %ld start mpi %ld end mpi %ld policy %s th %lf def_f %lu\n",
	job->start_time,job->end_time,job->start_mpi_time,job->end_mpi_time,job->policy,job->th,job->def_f);
	
}

void print_job_fd_binary(int fd, job_t *job)
{
	write(fd,job, sizeof(job_t));
}
/** Memory is already allocated for the job */
void read_job_fd_binary(int fd, job_t *job)
{
	read(fd,job, sizeof(job_t));
}


