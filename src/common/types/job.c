/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/output/verbose.h>
#include <common/system/time.h>
#include <common/types/job.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void init_job(job_t *job, ulong def_f, char *policy, double th, ulong procs)
{
    memset(job, 0, sizeof(job_t));
    strcpy(job->policy, policy);

    job->def_f = def_f;
    job->th    = th;
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
    time_t j_time       = time(NULL);
    job->start_mpi_time = j_time;
}

void end_mpi(job_t *job)
{
    time_t j_time;
    j_time            = time(NULL);
    job->end_mpi_time = j_time;
}

void copy_job(job_t *destiny, job_t *source)
{
    memcpy(destiny, source, sizeof(job_t));
}

void print_job_fd(int fd, job_t *job)
{
    char job_buff[4096];
    struct tm *ts;
    char buf_start[80], buf_end[80];
    time_t startt, endt;

    if (strlen(job->policy) < 1)
        strcpy(job->policy, " ");
    if (strlen(job->user_id) < 1)
        strcpy(job->user_id, " ");
    if (strlen(job->app_id) < 1)
        strcpy(job->app_id, " ");

    if (strlen(job->group_id) < 1)
        strcpy(job->group_id, " ");
    if (strlen(job->energy_tag) < 1)
        strcpy(job->energy_tag, " ");
    if (strlen(job->user_acc) < 1)
        strcpy(job->user_acc, " ");

    startt = job->start_time;
    endt   = job->end_time;

    ts = localtime(&startt);
    strftime(buf_start, sizeof(buf_start), "%Y-%m-%d %H:%M:%S", ts);

    ts = localtime(&endt);
    strftime(buf_end, sizeof(buf_end), "%Y-%m-%d %H:%M:%S", ts);

#if WF_SUPPORT
    sprintf(job_buff, "%lu;%lu;%lu;%s;%s;%s;%s;%s;%lu;%lu;%s;%s;%lu;%lu;%s;%lf;%lu;%u;%lu", job->id, job->step_id,
            job->local_id, job->user_id, job->group_id, job->user_acc, job->app_id, job->energy_tag, job->start_time,
            job->end_time, buf_start, buf_end, job->start_mpi_time, job->end_mpi_time, job->policy, job->th, job->procs,
            job->type, job->def_f);
#else
    sprintf(job_buff, "%lu;%lu;%s;%s;%s;%s;%s;%lu;%lu;%s;%s;%lu;%lu;%s;%lf;%lu;%u;%lu", job->id, job->step_id,
            job->user_id, job->group_id, job->user_acc, job->app_id, job->energy_tag, job->start_time, job->end_time,
            buf_start, buf_end, job->start_mpi_time, job->end_mpi_time, job->policy, job->th, job->procs, job->type,
            job->def_f);
#endif
    write(fd, job_buff, strlen(job_buff));
}

/** Reports the content of the job into the stderr*/
void report_job(job_t *job)
{
#if WF_SUPPORT
    verbose(VTYPE,
            "Job: ID %lu step %lu appid %lu user %s group %s name %s account %s "
            "etag %s\n",
            job->id, job->step_id, job->local_id, job->user_id, job->group_id, job->app_id, job->user_acc,
            job->energy_tag);
#else
    verbose(VTYPE, "Job: ID %lu step %lu  user %s group %s name %s account %s etag %s\n", job->id, job->step_id,
            job->user_id, job->group_id, job->app_id, job->user_acc, job->energy_tag);
#endif
    verbose(VTYPE,
            "start time %ld end time %ld start mpi %ld end mpi %ld policy %s th "
            "%lf def_f %lu\n",
            job->start_time, job->end_time, job->start_mpi_time, job->end_mpi_time, job->policy, job->th, job->def_f);
}

void print_job_fd_binary(int fd, job_t *job)
{
    write(fd, job, sizeof(job_t));
}

/** Memory is already allocated for the job */
void read_job_fd_binary(int fd, job_t *job)
{
    read(fd, job, sizeof(job_t));
}

void job_serialize(serial_buffer_t *b, job_t *job)
{
    serial_dictionary_push_auto(b, job->id);
    serial_dictionary_push_auto(b, job->step_id);
#if WF_SUPPORT
    serial_dictionary_push_auto(b, job->local_id);
#endif
    serial_dictionary_push_auto(b, job->user_id);
    serial_dictionary_push_auto(b, job->group_id);
    serial_dictionary_push_auto(b, job->app_id);
    serial_dictionary_push_auto(b, job->user_acc);
    serial_dictionary_push_auto(b, job->energy_tag);
    serial_dictionary_push_auto(b, job->start_time);
    serial_dictionary_push_auto(b, job->end_time);
    serial_dictionary_push_auto(b, job->start_mpi_time);
    serial_dictionary_push_auto(b, job->end_mpi_time);
    serial_dictionary_push_auto(b, job->policy);
    serial_dictionary_push_auto(b, job->th);
    serial_dictionary_push_auto(b, job->procs);
    serial_dictionary_push_auto(b, job->type);
    serial_dictionary_push_auto(b, job->def_f);
}

void job_deserialize(serial_buffer_t *b, job_t *job)
{
    serial_dictionary_pop_auto(b, job->id);
    serial_dictionary_pop_auto(b, job->step_id);
#if WF_SUPPORT
    serial_dictionary_pop_auto(b, job->local_id);
#endif
    serial_dictionary_pop_auto(b, job->user_id);
    serial_dictionary_pop_auto(b, job->group_id);
    serial_dictionary_pop_auto(b, job->app_id);
    serial_dictionary_pop_auto(b, job->user_acc);
    serial_dictionary_pop_auto(b, job->energy_tag);
    serial_dictionary_pop_auto(b, job->start_time);
    serial_dictionary_pop_auto(b, job->end_time);
    serial_dictionary_pop_auto(b, job->start_mpi_time);
    serial_dictionary_pop_auto(b, job->end_mpi_time);
    serial_dictionary_pop_auto(b, job->policy);
    serial_dictionary_pop_auto(b, job->th);
    serial_dictionary_pop_auto(b, job->procs);
    serial_dictionary_pop_auto(b, job->type);
    serial_dictionary_pop_auto(b, job->def_f);
}
