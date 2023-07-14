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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
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

static int report_file_created;

static ullong plug_start_time;
static uint must_report;

static FILE *stream;

static state_t append_data(report_id_t *id, sig_ext_t *data, uint count);


state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
    debug("report mpi_node_trace init %d", id->local_rank);

    if (id->master_rank >= 0 ) must_report = 1;
    if (!must_report) return EAR_SUCCESS;

    char *job_id = ear_getenv(SCHED_JOB_ID);
    if (job_id == NULL)
    {
        verbose(3, "%sWARNING%s %s could not be read.",
                COL_YLW, COL_CLR, SCHED_JOB_ID);
    } else {
        memcpy(jobid, job_id, sizeof(jobid));
    }

    char *step_id = ear_getenv(SCHED_STEP_ID);

    xsnprintf(csv_log_file, sizeof(csv_log_file), "mpitrace.%s.%s.%d.csv",
              jobid, step_id, id->master_rank);

    plug_start_time = timestamp_getconvert(TIME_SECS);

    return EAR_SUCCESS;
}


state_t report_misc(report_id_t *id, uint type, cchar *data, uint count)
{
    if (must_report && (type & MPI_NODE_METRICS)) {
        sig_ext_t *sig_shared = (sig_ext_t *) data;
        return append_data(id, sig_shared, count);
    }
    return EAR_SUCCESS;
}


static state_t append_data(report_id_t *id, sig_ext_t *data, uint count)
{

    debug("[%d] report mpi node metrics print", id->master_rank);

    if (csv_log_file == NULL) {
        return EAR_ERROR;
    }

    if (!report_file_created)
    {
        stream = fopen(csv_log_file, "a");
        if (stream == NULL) {
            return EAR_ERROR;
        }

#if DLB
        static char header[] = "time,node_id,total_useful_time,max_useful_time,"
                               "load_balance,parallel_efficiency,max_mpi,min_mpi,avg_mpi,"
                               "talp_load_balance,talp_parallel_efficiency";
#else
        static char header[] = "time,node_id,total_useful_time,max_useful_time,"
                               "load_balance,parallel_efficiency,max_mpi,min_mpi,avg_mpi";
#endif

        if (fprintf(stream, "%s", header) < 0) {
            return EAR_ERROR;
        }
        report_file_created = 1;
    }

    // Get the current time
    time_t curr_time = time(NULL);

    struct tm *loctime = localtime (&curr_time);

    char time_buff[32];
    strftime(time_buff, sizeof time_buff, "%c", loctime);

    // Average and max useful time (computation time)

    mpi_information_t *mpi_info_master = &data->mpi_stats[0];

    ullong total_useful_time = mpi_info_master->exec_time - mpi_info_master->mpi_time;
    ullong max_useful_time = total_useful_time;

    ullong max_exec_time = mpi_info_master->exec_time;

    float avg_mpi_time = (float) mpi_info_master->mpi_time;

    for (int i = 1; i < count; i++)
    {
        mpi_information_t *mpi_info = &data->mpi_stats[i];

        ullong useful_time = mpi_info->exec_time - mpi_info->mpi_time;

        max_useful_time = ear_max(max_useful_time, useful_time);
        total_useful_time += (double) useful_time;

        max_exec_time = ear_max(max_exec_time, mpi_info->exec_time);

        ullong mpi_time = mpi_info->mpi_time;
        avg_mpi_time += (double) mpi_time;
    }
    double avg_useful_time = total_useful_time / (double) count;
    avg_mpi_time /= (double) count;

    // Load balance and Parallel efficiency

    double load_balance = avg_useful_time / (double) max_useful_time;
    double parallel_eff = avg_useful_time / (double) max_exec_time;

    // MPI %
    double avg_mpi = avg_mpi_time / (double) max_exec_time;
    fprintf(stream, "\n%s,%d,%llu,%llu,%lf,%lf,%lf,%lf,%lf",
            time_buff, id->master_rank, total_useful_time,
            max_useful_time, load_balance, parallel_eff,
            data->max_mpi, data->min_mpi, avg_mpi
           );

    return EAR_SUCCESS;
}


state_t report_dispose(report_id_t *id)
{
    if (!must_report) return EAR_SUCCESS;

    debug("report mpi node metrics dispose");

    if (stream)
    {
        fclose(stream);
    }

    return EAR_SUCCESS;
}
