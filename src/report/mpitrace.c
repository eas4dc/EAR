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

// #define SHOW_DEBUGS 1

#include <common/includes.h>
#include <common/config.h>

#include <library/metrics/metrics.h>
#include <library/common/library_shared_data.h>

#include <report/report.h>

static char nodename[128];
static char csv_log_file[64];
static char jobid[32];

static int report_file_created = 0;

static ullong plug_start_time = 0;

static state_t append_data(report_id_t *id, shsignature_t *data);

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
    debug("report mpitrace init %d", id->local_rank);

    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    char *job_id = getenv(SCHED_JOB_ID);
    if (job_id == NULL) {
        verbose(3, "%sWARNING%s %s could not be read.", COL_YLW, COL_CLR, SCHED_JOB_ID);
    } else {
        memcpy(jobid, job_id, sizeof(jobid));
    }

    xsnprintf(csv_log_file, sizeof(csv_log_file), "mpitrace.%s.%s.%d.csv", jobid, nodename, id->local_rank);

    plug_start_time = timestamp_getconvert(TIME_SECS);

    return EAR_SUCCESS;
}

state_t report_misc(report_id_t *id, uint type, cchar *data, uint count)
{
    if (type & MPITRACE) {
        shsignature_t *sig_shared = (shsignature_t *) data;
        return append_data(id, sig_shared);
    }
    return EAR_SUCCESS;
}

static state_t append_data(report_id_t *id, shsignature_t *data)
{

    debug("report mpitrace print %d at %s", id->local_rank, nodename);

    FILE *fd;

    if (csv_log_file == NULL) {
        return EAR_ERROR;
    }

    fd = fopen(csv_log_file, "a");
    if (fd == NULL) {
        return EAR_ERROR;
    }

    
    if (!report_file_created) {
        char header[] = "JOBID;STEPID;NODENAME;L_RANK;G_RANK;MPI_CALLS;TIME_USECS;TIME_MPI;"
            "BLOCK;T_BLOCK;SYNC;T_SYNC;COLLEC;T_COLLEC;GFLOPS;CPI;L3_MISS";
        if (fprintf(fd, "%s\n", header) < 0) {
            return EAR_ERROR;
        }
        report_file_created = 1;
    }

    mpi_information_t *mpi_info = &data->mpi_info;
    mpi_calls_types_t *mpi_types = &data->mpi_types_info;
    ssig_t            *sig       = &data->sig;

    fprintf(fd, "%s;%d;%s;%d;%d;%u;%llu;%llu;%lu;%lu;%lu;%lu;%lu;%lu;%f;%f;%llu\n",
            jobid, 0, nodename, id->local_rank, id->global_rank,
            mpi_info->total_mpi_calls, mpi_info->exec_time, mpi_info->mpi_time,
            mpi_types->mpi_block_call_cnt, mpi_types->mpi_block_call_time,
            mpi_types->mpi_sync_call_cnt, mpi_types->mpi_sync_call_time, mpi_types->mpi_collec_call_cnt,
            mpi_types->mpi_collec_call_time, sig->Gflops, sig->CPI, sig->L3_misses);

    fclose(fd);

    return EAR_SUCCESS;
}
