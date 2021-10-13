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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>

#define htonll(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32)

#if DB_PSQL
#include <common/database/postgresql_io_functions.h>

#define APPLICATION_PSQL_QUERY   "INSERT INTO Applications (job_id, step_id, node_id, signature_id, power_signature_id) VALUES "

#define LEARNING_APPLICATION_PSQL_QUERY   "INSERT INTO Learning_applications (job_id, step_id, node_id, signature_id, power_signature_id) VALUES "

#define LOOP_PSQL_QUERY              "INSERT INTO Loops (event, size, level, job_id, step_id, node_id, total_iterations," \
                                "signature_id) VALUES "

#define CREATE_TEMP_JOBS    "CREATE TEMPORARY TABLE temp_jobs(id INT NOT NULL,step_id INT NOT NULL, user_id VARCHAR(128),app_id VARCHAR(128), " \
                            "start_time INT NOT NULL,end_time INT NOT NULL,start_mpi_time INT NOT NULL,end_mpi_time INT NOT NULL, "\
                            "policy VARCHAR(256) NOT NULL,threshold FLOAT NOT NULL,procs INT NOT NULL,job_type SMALLINT NOT NULL, def_f INT, "\
                            "user_acc VARCHAR(256), user_group VARCHAR(256), e_tag VARCHAR(256)) ON COMMIT DROP"

#define LOCK_JOBS_PSQL_QUERY     "LOCK TABLE Jobs IN SHARE ROW EXCLUSIVE MODE"

#define LOCK_LEARNING_JOBS_PSQL_QUERY     "LOCK TABLE Learning_jobs IN SHARE ROW EXCLUSIVE MODE"

#define INSERT_NEW_LEARNING_JOBS    "INSERT INTO Learning_jobs SELECT DISTINCT ON (temp_jobs.id, temp_jobs.step_id) temp_jobs.id, temp_jobs.step_id, temp_jobs.user_id, temp_jobs.app_id, "\
                                    "temp_jobs.start_time, temp_jobs.end_time, temp_jobs.start_mpi_time, temp_jobs.end_mpi_time, temp_jobs.policy, temp_jobs.threshold, "\
                                    "temp_jobs.procs, temp_jobs.job_type, temp_jobs.def_f, temp_jobs.user_acc, temp_jobs.user_group, temp_jobs.e_tag "\
                                    "FROM temp_jobs LEFT OUTER JOIN Learning_jobs ON (Learning_jobs.id = temp_jobs.id AND " \
                                    "Learning_jobs.step_id = temp_jobs.step_id) WHERE Learning_jobs.id IS NULL"

#define INSERT_NEW_JOBS     "INSERT INTO Jobs SELECT DISTINCT ON (temp_jobs.id, temp_jobs.step_id) temp_jobs.id, temp_jobs.step_id, temp_jobs.user_id, temp_jobs.app_id, "\
                            "temp_jobs.start_time, temp_jobs.end_time, temp_jobs.start_mpi_time, temp_jobs.end_mpi_time, temp_jobs.policy, temp_jobs.threshold, "\
                            "temp_jobs.procs, temp_jobs.job_type, temp_jobs.def_f, temp_jobs.user_acc, temp_jobs.user_group, temp_jobs.e_tag "\
                            "FROM temp_jobs LEFT OUTER JOIN Jobs ON (Jobs.id = temp_jobs.id AND " \
                            "Jobs.step_id = temp_jobs.step_id) WHERE Jobs.id IS NULL"

#define JOB_PSQL_QUERY               "INSERT INTO temp_jobs (id, step_id, user_id, app_id, start_time, end_time, start_mpi_time," \
                                "end_mpi_time, policy, threshold, procs, job_type, def_f, user_acc, user_group, e_tag) VALUES "

#define EAR_WARNING_PSQL_QUERY       "INSERT INTO Global_energy (energy_percent, warning_level, inc_th, p_state, GlobEnergyConsumedT1, "\
                                "GlobEnergyConsumedT2, GlobEnergyLimit, GlobEnergyPeriodT1, GlobEnergyPeriodT2, GlobEnergyPolicy) "\
                                "VALUES "

#if USE_GPUS
#define LEARNING_SIGNATURE_QUERY_SIMPLE "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                        "GBS, TPI, CPI, Gflops, time, avg_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES"

#define LEARNING_SIGNATURE_QUERY_FULL   "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                        "GBS, TPI, CPI, Gflops, time, FLOPS1, FLOPS2, FLOPS3, FLOPS4, "\
                                        "FLOPS5, FLOPS6, FLOPS7, FLOPS8,"\
                                        "instructions, cycles, avg_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES "
#else
#define LEARNING_SIGNATURE_QUERY_SIMPLE "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                        "GBS, TPI, CPI, Gflops, time, avg_f, def_f) VALUES"

#define LEARNING_SIGNATURE_QUERY_FULL   "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                        "GBS, TPI, CPI, Gflops, time, FLOPS1, FLOPS2, FLOPS3, FLOPS4, "\
                                        "FLOPS5, FLOPS6, FLOPS7, FLOPS8,"\
                                        "instructions, cycles, avg_f, def_f) VALUES "
#endif

#define POWER_SIGNATURE_PSQL_QUERY   "INSERT INTO Power_signatures (DC_power, DRAM_power, PCK_power, EDP, max_DC_power, min_DC_power, "\
                                "time, avg_f, def_f) VALUES "

#if USE_GPUS
#define GPU_SIGNATURE_PSQL_QUERY    "INSERT INTO GPU_signatures (GPU_power, GPU_freq, GPU_mem_freq, GPU_util, GPU_mem_util) VALUES "

#define PERIODIC_METRIC_QUERY_DETAIL    "INSERT INTO Periodic_metrics (start_time, end_time, DC_energy, node_id, job_id, step_id, avg_f, temp, DRAM_energy, "\
                                        "PCK_energy, GPU_energy) VALUES " 
#else
#define PERIODIC_METRIC_QUERY_DETAIL    "INSERT INTO Periodic_metrics (start_time, end_time, DC_energy, node_id, job_id, step_id, avg_f, temp, DRAM_energy, "\
                                        "PCK_energy) VALUES " 
#endif

#define PERIODIC_METRIC_QUERY_SIMPLE    "INSERT INTO Periodic_metrics (start_time, end_time, DC_energy, node_id, job_id, step_id) "\
                                        "VALUES "

#define EAR_EVENT_PSQL_QUERY         "INSERT INTO Events (timestamp, event_type, job_id, step_id, freq, node_id) VALUES "

#define PERIODIC_AGGREGATION_PSQL_QUERY "INSERT INTO Periodic_aggregations (DC_energy, start_time, end_time, eardbd_host) VALUES "

#if USE_GPUS
#define SIGNATURE_QUERY_SIMPLE  "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                        "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, avg_f, avg_imc_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES"

#define SIGNATURE_QUERY_FULL    "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, FLOPS1, FLOPS2, FLOPS3, FLOPS4, "\
                                "FLOPS5, FLOPS6, FLOPS7, FLOPS8,"\
                                "instructions, cycles, avg_f, avg_imc_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES "
#else
#define SIGNATURE_QUERY_SIMPLE  "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                        "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, avg_f, avg_imc_f, def_f) VALUES"

#define SIGNATURE_QUERY_FULL    "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, FLOPS1, FLOPS2, FLOPS3, FLOPS4, "\
                                "FLOPS5, FLOPS6, FLOPS7, FLOPS8,"\
                                "instructions, cycles, avg_f, avg_imc_f, def_f) VALUES "
#endif


static char full_signature = !DB_SIMPLE;
static char node_detail = 1;

#if DB_SIMPLE
char *LEARNING_SIGNATURE_PSQL_QUERY = LEARNING_SIGNATURE_QUERY_SIMPLE;
char *SIGNATURE_PSQL_QUERY = SIGNATURE_QUERY_SIMPLE;
#else
char *LEARNING_SIGNATURE_PSQL_QUERY = LEARNING_SIGNATURE_QUERY_FULL;
char *SIGNATURE_PSQL_QUERY = SIGNATURE_QUERY_FULL;
#endif

char *PERIODIC_METRIC_PSQL_QUERY = PERIODIC_METRIC_QUERY_DETAIL;

int per_met_args = PERIODIC_METRIC_ARGS;
int sig_args = SIGNATURE_ARGS;


void exit_connection(PGconn *conn)
{
    verbose(VMYSQL, "%s\n", PQerrorMessage(conn));
    PQfinish(conn);
    exit(0);
}


/* IMPORTANT NOTEST */
// ALL BINARY DATA (non-strings) must be swapped to big endian before inserting and/or after reading
// htonl() always reverses, regardless of the endianess of the input, same as ntohl(), it might be useful to change this calls to __builtin_bswap64 in the future (GCC ONLY)
// double_swap() must be used for floating point values, as htonl does not work properly with them
// the procedure to workaround the lack of INSERT IGNORE in postgres <9.5 is: create a temp table, insert into it, filter repetitions and insert into actual table
//

void set_signature_simple(char full_sig)
{
    full_signature = full_sig;
    if (full_signature)
    {
        LEARNING_SIGNATURE_PSQL_QUERY = LEARNING_SIGNATURE_QUERY_FULL;
        SIGNATURE_PSQL_QUERY = SIGNATURE_QUERY_FULL;    
        sig_args = 24;
    }
    else
    {
        LEARNING_SIGNATURE_PSQL_QUERY = LEARNING_SIGNATURE_QUERY_SIMPLE;
        SIGNATURE_PSQL_QUERY = SIGNATURE_QUERY_SIMPLE;    
        sig_args = 14;
    }

#if USE_GPUS
    sig_args += 2; //with gpus, signatures will contain min_gpu_sig_id and max_gpu_sig_id
#endif

}

void set_node_detail(char node_det)
{
    node_detail = node_det;
    if (node_detail)
    {
        PERIODIC_METRIC_PSQL_QUERY = PERIODIC_METRIC_QUERY_DETAIL;
#if USE_GPUS
        per_met_args = 11;
#else
        per_met_args = 10;
#endif
    }
    else
    {
        PERIODIC_METRIC_PSQL_QUERY = PERIODIC_METRIC_QUERY_SIMPLE;
        per_met_args = 6;
    }
}

double double_swap(double d)
{
    union
    {
        double d;
        unsigned char bytes[8];
    } src, dest;

    src.d = d;
    dest.bytes[0] = src.bytes[7];
    dest.bytes[1] = src.bytes[6];
    dest.bytes[2] = src.bytes[5];
    dest.bytes[3] = src.bytes[4];
    dest.bytes[4] = src.bytes[3];
    dest.bytes[5] = src.bytes[2];
    dest.bytes[6] = src.bytes[1];
    dest.bytes[7] = src.bytes[0];
    return dest.d;
}

void reverse_loop_bytes(loop_t *loops, int num_loops)
{

    int i;

    for (i = 0; i < num_loops; i++)
    {
        loops[i].jid = htonl(loops[i].jid);
        loops[i].step_id = htonl(loops[i].step_id);
        loops[i].id.event = htonl(loops[i].id.event);
        loops[i].id.size = htonl(loops[i].id.size);
        loops[i].id.level = htonl(loops[i].id.level);
        loops[i].total_iterations = htonl(loops[i].total_iterations);
    }
}

void reverse_ear_event_bytes(ear_event_t *events, int num_events)
{

    int i;

    for (i = 0; i < num_events; i++)
    {
        events[i].jid = htonl(events[i].jid);
        events[i].step_id = htonl(events[i].step_id);
        events[i].event = htonl(events[i].event);
        events[i].freq = htonl(events[i].freq);
        events[i].timestamp = htonl(events[i].timestamp);
    }
}

void reverse_gm_warning_bytes(gm_warning_t *warns, int num_warns)
{

    int i;

    for (i = 0; i < num_warns; i++)
    {
        warns[i].energy_percent = double_swap(warns[i].energy_percent);
        warns[i].inc_th = double_swap(warns[i].inc_th);
        warns[i].level = htonl(warns[i].level);
        warns[i].new_p_state = htonl(warns[i].new_p_state);
        warns[i].energy_t1 = htonl(warns[i].energy_t1);
        warns[i].energy_t2 = htonl(warns[i].energy_t2);
        warns[i].energy_limit = htonl(warns[i].energy_limit);
        warns[i].energy_p1 = htonl(warns[i].energy_p1);
        warns[i].energy_p2 = htonl(warns[i].energy_p2);
    }
}

void reverse_periodic_aggregation_bytes(periodic_aggregation_t *per_aggs, int num_aggs)
{
    int i;

    for (i = 0; i < num_aggs; i++)
    {
        per_aggs[i].DC_energy = htonl(per_aggs[i].DC_energy);
        per_aggs[i].start_time = htonl(per_aggs[i].start_time);
        per_aggs[i].end_time = htonl(per_aggs[i].end_time);
    }
}

void reverse_periodic_metric_bytes(periodic_metric_t *per_mets, int num_mets)
{
    int i;

    for (i = 0; i < num_mets; i++)
    {
        per_mets[i].DC_energy = htonl(per_mets[i].DC_energy);
        per_mets[i].job_id = htonl(per_mets[i].job_id);
        per_mets[i].step_id = htonl(per_mets[i].step_id);
        per_mets[i].start_time = htonl(per_mets[i].start_time);
        per_mets[i].end_time = htonl(per_mets[i].end_time);
        per_mets[i].avg_f = htonl(per_mets[i].avg_f);
        per_mets[i].temp = htonl(per_mets[i].temp);
        per_mets[i].PCK_energy = htonl(per_mets[i].PCK_energy);
        per_mets[i].DRAM_energy = htonl(per_mets[i].DRAM_energy);
#if USE_GPUS
        per_mets[i].GPU_energy = htonl(per_mets[i].GPU_energy);
#endif
    }
}

void reverse_power_signature_bytes(power_signature_t *sigs, int num_sigs)
{
    int i;

    for (i = 0; i < num_sigs; i++)
    {
        sigs[i].DC_power = double_swap(sigs[i].DC_power);
        sigs[i].DRAM_power = double_swap(sigs[i].DRAM_power);
        sigs[i].PCK_power = double_swap(sigs[i].PCK_power);
        sigs[i].EDP = double_swap(sigs[i].EDP);
        sigs[i].max_DC_power = double_swap(sigs[i].max_DC_power);
        sigs[i].min_DC_power = double_swap(sigs[i].min_DC_power);
        sigs[i].time = double_swap(sigs[i].time);
        sigs[i].avg_f = double_swap(sigs[i].avg_f);
        sigs[i].def_f = double_swap(sigs[i].def_f);
    }
}

void reverse_signature_bytes(signature_t *sigs, int num_sigs)
{
    int i, j;

    for (i = 0; i < num_sigs; i++)
    {
        sigs[i].DC_power = double_swap(sigs[i].DC_power);
        sigs[i].DRAM_power = double_swap(sigs[i].DRAM_power);
        sigs[i].PCK_power = double_swap(sigs[i].PCK_power);
        sigs[i].EDP = double_swap(sigs[i].EDP);
        sigs[i].GBS = double_swap(sigs[i].GBS);
        sigs[i].IO_MBS = double_swap(sigs[i].IO_MBS);
        sigs[i].TPI = double_swap(sigs[i].TPI);
        sigs[i].CPI = double_swap(sigs[i].CPI);
        sigs[i].Gflops = double_swap(sigs[i].Gflops);
        sigs[i].time = double_swap(sigs[i].time);
        sigs[i].perc_MPI = double_swap(sigs[i].perc_MPI);
        for (j = 0; j < FLOPS_EVENTS; j++)
            sigs[i].FLOPS[j] = htonll(sigs[i].FLOPS[j]);
        sigs[i].L1_misses = htonll(sigs[i].L1_misses);
        sigs[i].L2_misses = htonll(sigs[i].L2_misses);
        sigs[i].L3_misses = htonll(sigs[i].L3_misses);
        sigs[i].instructions = htonll(sigs[i].instructions);
        sigs[i].cycles = htonll(sigs[i].cycles);
        sigs[i].avg_f = htonl(sigs[i].avg_f);
        sigs[i].avg_imc_f = htonl(sigs[i].avg_imc_f);
        sigs[i].def_f = htonl(sigs[i].def_f);
    }
}

#if USE_GPUS
int reverse_gpu_signature_bytes(signature_t *sigs, int num_sigs)
{
    int i, j, num_gpu_sigs = 0;
    gpu_app_t *gpu_data;

    for (i = 0; i < num_sigs; i++)
    {

        for (j = 0; j < sigs[i].gpu_sig.num_gpus; j++)
        {
            num_gpu_sigs++;
            gpu_data = &sigs[i].gpu_sig.gpu_data[j];
            gpu_data->GPU_power    = double_swap(gpu_data->GPU_power); 
            gpu_data->GPU_freq     = htonl(gpu_data->GPU_freq); 
            gpu_data->GPU_mem_freq = htonl(gpu_data->GPU_mem_freq); 
            gpu_data->GPU_util     = htonl(gpu_data->GPU_util);
            gpu_data->GPU_mem_util = htonl(gpu_data->GPU_mem_util); 
        }
    }

    return num_gpu_sigs;
}
#endif

void reverse_job_bytes(application_t *apps, int num_jobs)
{
    int i;
    
    for (i = 0; i < num_jobs; i++)
    {
        apps[i].job.id = htonl(apps[i].job.id);
        apps[i].job.step_id = htonl(apps[i].job.step_id);
        apps[i].job.start_time = htonl(apps[i].job.start_time);
        apps[i].job.end_time = htonl(apps[i].job.end_time);
        apps[i].job.start_mpi_time = htonl(apps[i].job.start_mpi_time);
        apps[i].job.end_mpi_time = htonl(apps[i].job.end_mpi_time);
        apps[i].job.th = double_swap(apps[i].job.th);
        apps[i].job.procs = htonl(apps[i].job.procs);
        apps[i].job.type = htonl(apps[i].job.type);
        apps[i].job.def_f = htonl(apps[i].job.def_f);
    }
}


int postgresql_retrieve_jobs(PGconn *connection, char *query, job_t **jobs)
{
    int i, num_rows;
    job_t *jobs_aux;

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
       verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    //for every row, we read the data and query the job, power_signature and signature, the latter only if the job has one

    num_rows = PQntuples(res);
    if (num_rows < 1)
    {
       verbose(VMYSQL, "Query returned 0 rows\n");
        PQclear(res);
        return EAR_ERROR;
    }

    jobs_aux = calloc(num_rows, sizeof(application_t));
   
    for (i = 0; i < num_rows; i++)
    {
        jobs_aux[i].id = htonl( *((int *)PQgetvalue(res, i, 0)));
        jobs_aux[i].step_id = htonl( *((int *)PQgetvalue(res, i, 1)));
        strcpy(jobs_aux[i].user_id, PQgetvalue(res, i, 2));
        strcpy(jobs_aux[i].app_id , PQgetvalue(res, i, 3));
        jobs_aux[i].start_time = htonl( *((time_t *)PQgetvalue(res, i, 4)));
        jobs_aux[i].end_time = htonl( *((time_t *)PQgetvalue(res, i, 5)));
        jobs_aux[i].start_mpi_time = htonl( *((time_t *)PQgetvalue(res, i, 6)));
        jobs_aux[i].end_mpi_time = htonl( *((time_t *)PQgetvalue(res, i, 7)));
        strcpy(jobs_aux[i].policy, PQgetvalue(res, i, 8));
        jobs_aux[i].th = double_swap( *((double *)PQgetvalue(res, i, 9)));
        jobs_aux[i].procs = htonl( *((ulong *)PQgetvalue(res, i, 10))); //this might cause trouble, might have to cast into long
        jobs_aux[i].type = htonl( *((job_type *)PQgetvalue(res, i, 11)));
        jobs_aux[i].def_f = htonl( *((ulong *)PQgetvalue(res, i, 12)));
        strcpy(jobs_aux[i].user_acc, PQgetvalue(res, i, 13));
        strcpy(jobs_aux[i].group_id, PQgetvalue(res, i, 14));
        strcpy(jobs_aux[i].energy_tag, PQgetvalue(res, i, 15));

    }
   
    *jobs = jobs_aux;

    return num_rows;
}

int postgresql_retrieve_power_signatures(PGconn *connection, char *query, power_signature_t **pow_sigs)
{
    int i, num_rows;
    power_signature_t *power_sig_aux;

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
       verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    //for every row, we read the data and query the job, power_signature and signature, the latter only if the job has one

    num_rows = PQntuples(res);
    if (num_rows < 1)
    {
       verbose(VMYSQL, "Query returned 0 rows\n");
        PQclear(res);
        return EAR_ERROR;
    }

    power_sig_aux = calloc(num_rows, sizeof(power_signature_t));
   
    for (i = 0; i < num_rows; i++)
    {
        power_sig_aux[i].DC_power = double_swap( *((double *)PQgetvalue(res, i, 1)));
        power_sig_aux[i].DRAM_power = double_swap( *((double *)PQgetvalue(res, i, 2)));
        power_sig_aux[i].PCK_power = double_swap( *((double *)PQgetvalue(res, i, 3)));
        power_sig_aux[i].EDP = double_swap( *((double *)PQgetvalue(res, i, 4)));
        power_sig_aux[i].max_DC_power = double_swap( *((double *)PQgetvalue(res, i, 5)));
        power_sig_aux[i].min_DC_power = double_swap( *((double *)PQgetvalue(res, i, 6)));
        power_sig_aux[i].time = double_swap( *((double *)PQgetvalue(res, i, 7)));
        power_sig_aux[i].avg_f = htonl( *((ulong *)PQgetvalue(res, i, 8)));
        power_sig_aux[i].def_f = htonl( *((ulong *)PQgetvalue(res, i, 9)));

    }
   
    *pow_sigs = power_sig_aux;

    return num_rows;
}

#if USE_GPUS
int postgresql_retrieve_gpu_signatures(PGconn *connection, char *query, gpu_signature_t *gpu_sig)
{
    int i, num_rows;
    gpu_app_t base_data;

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        verbose(VMYSQL, "ERROR while reading gpu signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    //for every row, we read the data and query the job, power_signature and signature, the latter only if the job has one

    num_rows = PQntuples(res);
    if (num_rows < 1 || num_rows > MAX_GPUS_SUPPORTED)
    {
        verbose(VMYSQL, "Query returned %d rows (minimum 1, maximum %d)\n", num_rows, MAX_GPUS_SUPPORTED);
        PQclear(res);
        return EAR_ERROR;
    }

    gpu_sig->num_gpus = num_rows; 

    for (i = 0; i < num_rows; i++)
    {
        //we don't actually need to read the id in postgresql
        //id = htonl( *((ulong *)PQgetvalue(res, i, 1))); 

        base_data.GPU_power    = double_swap( *((double *)PQgetvalue(res, i, 2)));
        base_data.GPU_freq     = htonl( *((ulong *)PQgetvalue(res, i, 3)));
        base_data.GPU_mem_freq = htonl( *((ulong *)PQgetvalue(res, i, 4)));
        base_data.GPU_util     = htonl( *((ulong *)PQgetvalue(res, i, 5)));
        base_data.GPU_mem_util = htonl( *((ulong *)PQgetvalue(res, i, 6)));

        memcpy(&gpu_sig->gpu_data[i], &base_data, sizeof(gpu_app_t));
    }
   
    return num_rows;
}
#endif

int postgresql_retrieve_signatures(PGconn *connection, char *query, signature_t **sigs)
{
    int i, num_rows;
    signature_t *sig_aux;
#if USE_GPUS
    char gpu_sig_query[256];
    long int min_gpu_sig_id = -1, max_gpu_sig_id = -1;
    int num_gpus;
#endif

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
       verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    //for every row, we read the data and query the job, power_signature and signature, the latter only if the job has one

    num_rows = PQntuples(res);
    if (num_rows < 1)
    {
       verbose(VMYSQL, "Query returned 0 rows\n");
        PQclear(res);
        return EAR_ERROR;
    }

    sig_aux = calloc(num_rows, sizeof(signature_t));
   
    for (i = 0; i < num_rows; i++)
    {
        sig_aux[i].DC_power = double_swap( *((double *)PQgetvalue(res, i, 1)));
        sig_aux[i].DRAM_power = double_swap( *((double *)PQgetvalue(res, i, 2)));
        sig_aux[i].PCK_power = double_swap( *((double *)PQgetvalue(res, i, 3)));
        sig_aux[i].EDP = double_swap( *((double *)PQgetvalue(res, i, 4)));
        sig_aux[i].GBS = double_swap( *((double *)PQgetvalue(res, i, 5)));
        sig_aux[i].IO_MBS = double_swap( *((double *)PQgetvalue(res, i, 6)));
        sig_aux[i].TPI = double_swap( *((double *)PQgetvalue(res, i, 7)));
        sig_aux[i].CPI = double_swap( *((double *)PQgetvalue(res, i, 8)));
        sig_aux[i].Gflops = double_swap( *((double *)PQgetvalue(res, i, 9)));
        sig_aux[i].time = double_swap( *((double *)PQgetvalue(res, i, 10)));
        sig_aux[i].perc_MPI = double_swap( *((double *)PQgetvalue(res, i, 11)));
        if (full_signature)
        {
            sig_aux[i].FLOPS[0] = htonll( *((ulong *)PQgetvalue(res, i, 12)));
            sig_aux[i].FLOPS[1] = htonll( *((ulong *)PQgetvalue(res, i, 13)));
            sig_aux[i].FLOPS[2] = htonll( *((ulong *)PQgetvalue(res, i, 14)));
            sig_aux[i].FLOPS[3] = htonll( *((ulong *)PQgetvalue(res, i, 15)));
            sig_aux[i].FLOPS[4] = htonll( *((ulong *)PQgetvalue(res, i, 16)));
            sig_aux[i].FLOPS[5] = htonll( *((ulong *)PQgetvalue(res, i, 17)));
            sig_aux[i].FLOPS[6] = htonll( *((ulong *)PQgetvalue(res, i, 18)));
            sig_aux[i].FLOPS[7] = htonll( *((ulong *)PQgetvalue(res, i, 19)));
            sig_aux[i].instructions = htonll( *((ulong *)PQgetvalue(res, i, 20)));
            sig_aux[i].cycles = htonll( *((ulong *)PQgetvalue(res, i, 21)));
            sig_aux[i].avg_f = htonl( *((ulong *)PQgetvalue(res, i, 22)));
            sig_aux[i].avg_imc_f = htonl( *((ulong *)PQgetvalue(res, i, 23)));
            sig_aux[i].def_f = htonl( *((ulong *)PQgetvalue(res, i, 24)));
#if USE_GPUS
            min_gpu_sig_id = htonl( *((ulong *)PQgetvalue(res, i, 25)));
            max_gpu_sig_id = htonl( *((ulong *)PQgetvalue(res, i, 26)));
#endif
        }
        else
        {
            sig_aux[i].avg_f = htonl( *((ulong *)PQgetvalue(res, i, 12)));
            sig_aux[i].avg_imc_f = htonl( *((ulong *)PQgetvalue(res, i, 13)));
            sig_aux[i].def_f = htonl( *((ulong *)PQgetvalue(res, i, 14)));
#if USE_GPUS
            min_gpu_sig_id = htonl( *((ulong *)PQgetvalue(res, i, 15)));
            max_gpu_sig_id = htonl( *((ulong *)PQgetvalue(res, i, 16)));
#endif
        }
#if USE_GPUS
        if (min_gpu_sig_id >= 0 && max_gpu_sig_id >= 0)
        {
            sprintf(gpu_sig_query, "SELECT * FROM GPU_signatures WHERE id <= %ld AND id >= %ld", max_gpu_sig_id, min_gpu_sig_id);
            num_gpus = postgresql_retrieve_gpu_signatures(connection, gpu_sig_query, &sig_aux[i].gpu_sig);
            if (num_gpus < 1)
                sig_aux[i].gpu_sig.num_gpus = 0;
        }

        min_gpu_sig_id = -1;
        max_gpu_sig_id = -1;

#endif
    }

    *sigs = sig_aux;

    return num_rows;
}

int postgresql_retrieve_applications(PGconn *connection, char *query, application_t **apps, char is_learning)
{

    int i, num_rows, job_id, step_id, sig_id, pow_sig_id;
    char job_query[256], pow_sig_query[256], sig_query[256];
    job_t *job_aux;
    application_t *apps_aux;
    power_signature_t *pow_sig_aux;
    signature_t *sig_aux;


    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    //for every row, we read the data and query the job, power_signature and signature, the latter only if the job has one

    num_rows = PQntuples(res);
    if (num_rows < 1)
    {
        verbose(VMYSQL, "Query returned 0 rows\n");
        PQclear(res);
        return EAR_ERROR;
    }

    apps_aux = calloc(num_rows, sizeof(application_t));

    for (i = 0; i < PQntuples(res); i++)
    {
        job_id = htonl( *((int *)PQgetvalue(res, i, 0)));
        step_id = htonl( *((int *)PQgetvalue(res, i, 1)));
        strcpy(apps_aux[i].node_id, PQgetvalue(res, i, 2));
        sig_id = htonl( *((int *)PQgetvalue(res, i, 3)));
        pow_sig_id = htonl( *((int *)PQgetvalue(res, i, 4)));

        /* JOB RETRIEVAL */
        if (is_learning)
            sprintf(job_query, "SELECT * FROM Learning_jobs WHERE id = %d AND step_id = %d", job_id, step_id);
        else
            sprintf(job_query, "SELECT * FROM Jobs WHERE id = %d AND step_id = %d", job_id, step_id);

        if (postgresql_retrieve_jobs(connection, job_query, &job_aux) < 1)
        {
            verbose(VMYSQL, "ERROR retrieving jobs\n");
        }
        else
        {
            memcpy(&apps_aux[i].job, job_aux, sizeof(job_t));
            free(job_aux);
        }

        /* POWER SIGNATURE RETRIEVAL */
        sprintf(pow_sig_query, "SELECT * FROM Power_signatures WHERE id = %d", pow_sig_id);

        if (postgresql_retrieve_power_signatures(connection, pow_sig_query, &pow_sig_aux) < 1)
        {
            verbose(VMYSQL, "ERROR retrieving power signatures\n");
        }
        else
        {
            memcpy(&apps_aux[i].power_sig, pow_sig_aux, sizeof(power_signature_t));
            free(pow_sig_aux);
        }

        /* JOB RETRIEVAL */
        if (sig_id > 0)
        {
            if (is_learning)
                sprintf(sig_query, "SELECT * FROM Learning_signatures WHERE id = %d", sig_id);
            else
                sprintf(sig_query, "SELECT * FROM Signatures WHERE id = %d", sig_id);


            if (postgresql_retrieve_signatures(connection, sig_query, &sig_aux) < 1)
            {
                verbose(VMYSQL, "ERROR retrieving signatures\n");
            }
            else
            {
                memcpy(&apps_aux[i].signature, sig_aux, sizeof(signature_t));
                apps_aux[i].is_mpi = 1;
                free(sig_aux);
            }
        }

    }

    *apps = apps_aux;
    PQclear(res);
    return num_rows;

}

int postgresql_retrieve_loops(PGconn *connection, char *query, loop_t **loops)
{

    int i, num_rows, sig_id;
    char sig_query[256];
    loop_t *loops_aux;
    signature_t *sig_aux;


    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    //for every row, we read the data and query the job, power_signature and signature, the latter only if the job has one

    num_rows = PQntuples(res);
    if (num_rows < 1)
    {
        verbose(VMYSQL, "Query returned 0 rows\n");
        PQclear(res);
        return EAR_ERROR;
    }

    loops_aux = calloc(num_rows, sizeof(loop_t));

    for (i = 0; i < PQntuples(res); i++)
    {
        loops_aux[i].id.event           = htonl( *((int *)PQgetvalue(res, i, 0)));
        loops_aux[i].id.size            = htonl( *((int *)PQgetvalue(res, i, 1)));
        loops_aux[i].id.level           = htonl( *((int *)PQgetvalue(res, i, 2)));
        loops_aux[i].jid                = htonl( *((int *)PQgetvalue(res, i, 3)));
        loops_aux[i].step_id            = htonl( *((int *)PQgetvalue(res, i, 4)));
        loops_aux[i].total_iterations   = htonl( *((int *)PQgetvalue(res, i, 6)));
        sig_id                          = htonl( *((int *)PQgetvalue(res, i, 7)));
        strcpy(loops_aux[i].node_id, PQgetvalue(res, i, 5));

        /* SIGNATURE RETRIEVAL */
        if (sig_id > 0)
        {
            sprintf(sig_query, "SELECT * FROM Signatures WHERE id = %d", sig_id);

            if (postgresql_retrieve_signatures(connection, sig_query, &sig_aux) < 1)
            {
                verbose(VMYSQL, "ERROR retrieving signatures\n");
            }
            else
            {
                memcpy(&loops_aux[i].signature, sig_aux, sizeof(signature_t));
                free(sig_aux);
            }
        }

    }

    *loops = loops_aux;
    PQclear(res);
    return num_rows;

}

int postgresql_get_current_autoincrement_val(PGconn *connection, char *table_name)
{
    char *full_query = calloc(strlen("SELECT last_values FROM ")+strlen(table_name)+strlen("_id_seq")+1, sizeof(char));
    strcpy(full_query, "SELECT last_value FROM ");
    strcat(full_query, table_name);
    strcat(full_query, "_id_seq");
    PGresult *res = PQexecParams(connection, full_query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    free(full_query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) //-> if it returned data
    {
        verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    char *data = PQgetvalue(res, 0, 0);
    //warning This might actually be a long int and that is why the first bytes are all zeroes. 
    int *value = ((int *) data);
    PQclear(res);
    return htonl(value[1]);

}

int postgresql_batch_insert_ear_events(PGconn *connection, ear_event_t *events, int num_events)
{
    if (num_events < 0 || events == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    reverse_ear_event_bytes(events, num_events);

    /* Memory allocation */
    param_values = calloc(EAR_EVENTS_ARGS*num_events, sizeof(char *));
    param_lengths = calloc(EAR_EVENTS_ARGS*num_events, sizeof(int));
    param_formats = calloc(EAR_EVENTS_ARGS*num_events, sizeof(int));
    query = calloc((strlen(EAR_EVENT_PSQL_QUERY)+(num_events*EAR_EVENTS_ARGS * 10)), sizeof(char));

    strcpy(query, EAR_EVENT_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_events; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + EAR_EVENTS_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &events[i].timestamp;
        param_values[1  + offset] = (char *) &events[i].event;
        param_values[2  + offset] = (char *) &events[i].jid;
        param_values[3  + offset] = (char *) &events[i].step_id;
        param_values[4  + offset] = (char *) &events[i].freq;
        param_values[5  + offset] = (char *) &events[i].node_id;

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = sizeof(int);
        param_lengths[3  + offset] = sizeof(int);
        param_lengths[4  + offset] = sizeof(int);
        param_lengths[5  + offset] = sizeof(events[i].node_id);

        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < EAR_EVENTS_ARGS; j++)
            param_formats[j + offset] = 1;
        param_formats[offset + 5] = 0; //node_id

        offset += EAR_EVENTS_ARGS;
    }


    PGresult *res = PQexecParams(connection, query, EAR_EVENTS_ARGS * num_events, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting ear_events: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }
    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_insert_ear_event(PGconn *connection, ear_event_t *event)
{
    int result;
    result = postgresql_batch_insert_ear_events(connection, event, 1);
    reverse_ear_event_bytes(event, 1);
    return result;
}

int postgresql_batch_insert_gm_warnings(PGconn *connection, gm_warning_t *warns, int num_warns)
{
    if (num_warns < 0 || warns == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    reverse_gm_warning_bytes(warns, num_warns);

    /* Memory allocation */
    param_values = calloc(10*num_warns, sizeof(char *));
    param_lengths = calloc(10*num_warns, sizeof(int));
    param_formats = calloc(10*num_warns, sizeof(int));
    query = calloc((strlen(EAR_WARNING_PSQL_QUERY)+(num_warns*10 * 10)), sizeof(char));

    strcpy(query, EAR_WARNING_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_warns; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + 10 + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &warns[i].energy_percent;
        param_values[1  + offset] = (char *) &warns[i].level;
        param_values[2  + offset] = (char *) &warns[i].inc_th;
        param_values[3  + offset] = (char *) &warns[i].new_p_state;
        param_values[4  + offset] = (char *) &warns[i].energy_t1;
        param_values[5  + offset] = (char *) &warns[i].energy_t2;
        param_values[6  + offset] = (char *) &warns[i].energy_limit;
        param_values[7  + offset] = (char *) &warns[i].energy_p1;
        param_values[8  + offset] = (char *) &warns[i].energy_p2;
        param_values[9  + offset] = (char *) &warns[i].policy;

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(double);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = sizeof(double);
        param_lengths[3  + offset] = sizeof(int);
        param_lengths[4  + offset] = sizeof(int);
        param_lengths[5  + offset] = sizeof(int);
        param_lengths[6  + offset] = sizeof(int);
        param_lengths[7  + offset] = sizeof(int);
        param_lengths[8  + offset] = sizeof(int);
        param_lengths[9  + offset] = sizeof(warns[i].policy);

        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < 10; j++)
            param_formats[j + offset] = 1;
        param_formats[offset + 9] = 0; //node_id

        offset += 10;
    }


    PGresult *res = PQexecParams(connection, query, 10 * num_warns, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting gm_warnings: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }
    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_insert_gm_warning(PGconn *connection, gm_warning_t *warn)
{
    int result;
    result = postgresql_batch_insert_gm_warnings(connection, warn, 1);
    reverse_gm_warning_bytes(warn, 1);
    return result;
}

int postgresql_batch_insert_periodic_aggregations(PGconn *connection, periodic_aggregation_t *per_aggs, int num_aggs)
{
    if (num_aggs < 0 || per_aggs == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    reverse_periodic_aggregation_bytes(per_aggs, num_aggs);

    /* Memory allocation */
    param_values = calloc(PERIODIC_AGGREGATION_ARGS*num_aggs, sizeof(char *));
    param_lengths = calloc(PERIODIC_AGGREGATION_ARGS*num_aggs, sizeof(int));
    param_formats = calloc(PERIODIC_AGGREGATION_ARGS*num_aggs, sizeof(int));
    query = calloc((strlen(PERIODIC_AGGREGATION_PSQL_QUERY)+(num_aggs*PERIODIC_AGGREGATION_ARGS * 10)), sizeof(char));

    strcpy(query, PERIODIC_AGGREGATION_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_aggs; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + PERIODIC_AGGREGATION_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &per_aggs[i].DC_energy;
        param_values[1  + offset] = (char *) &per_aggs[i].start_time;
        param_values[2  + offset] = (char *) &per_aggs[i].end_time;
        param_values[3  + offset] = (char *) &per_aggs[i].eardbd_host;

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = sizeof(int);
        param_lengths[3  + offset] = sizeof(per_aggs[i].eardbd_host);

        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < PERIODIC_AGGREGATION_ARGS; j++)
            param_formats[j + offset] = 1;
        param_formats[offset + 3] = 0; //node_id

        offset += PERIODIC_AGGREGATION_ARGS;
    }


    PGresult *res = PQexecParams(connection, query, PERIODIC_AGGREGATION_ARGS * num_aggs, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting periodic_aggregations: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }
    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_insert_periodic_aggregation(PGconn *connection, periodic_aggregation_t *per_agg)
{
    int result;
    result = postgresql_batch_insert_periodic_aggregations(connection, per_agg, 1);
    reverse_periodic_aggregation_bytes(per_agg, 1);
    return result;
}

int postgresql_batch_insert_periodic_metrics(PGconn *connection, periodic_metric_t *per_mets, int num_mets)
{
    if (num_mets < 0 || per_mets == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    reverse_periodic_metric_bytes(per_mets, num_mets);

    /* Memory allocation */
    param_values = calloc(per_met_args*num_mets, sizeof(char *));
    param_lengths = calloc(per_met_args*num_mets, sizeof(int));
    param_formats = calloc(per_met_args*num_mets, sizeof(int));
    query = calloc((strlen(PERIODIC_METRIC_PSQL_QUERY)+(num_mets*per_met_args* 10)), sizeof(char));

    strcpy(query, PERIODIC_METRIC_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_mets; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + per_met_args + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &per_mets[i].start_time;
        param_values[1  + offset] = (char *) &per_mets[i].end_time;
        param_values[2  + offset] = (char *) &per_mets[i].DC_energy;
        param_values[3  + offset] = (char *) &per_mets[i].node_id;
        param_values[4  + offset] = (char *) &per_mets[i].job_id;
        param_values[5  + offset] = (char *) &per_mets[i].step_id;
        if (node_detail)
        {
            param_values[6  + offset] = (char *) &per_mets[i].avg_f;
            param_values[7  + offset] = (char *) &per_mets[i].temp;
            param_values[8  + offset] = (char *) &per_mets[i].DRAM_energy;
            param_values[9  + offset] = (char *) &per_mets[i].PCK_energy;
#if USE_GPUS
            param_values[10 + offset] = (char *) &per_mets[i].GPU_energy;
#endif
        }

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = sizeof(int);
        param_lengths[3  + offset] = sizeof(per_mets[i].node_id);
        param_lengths[4  + offset] = sizeof(int);
        param_lengths[5  + offset] = sizeof(int);
        if (node_detail)
        {
            param_lengths[6  + offset] = sizeof(int);
            param_lengths[7  + offset] = sizeof(int);
            param_lengths[8  + offset] = sizeof(int);
            param_lengths[9  + offset] = sizeof(int);
#if USE_GPUS
            param_lengths[10 + offset] = sizeof(int);
#endif
        }


        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < per_met_args; j++)
            param_formats[j + offset] = 1;
        param_formats[offset + 3] = 0; //node_id

        offset += per_met_args;
    }


    PGresult *res = PQexecParams(connection, query, per_met_args* num_mets, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting periodic_metrics: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }
    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_insert_periodic_metric(PGconn *connection, periodic_metric_t *per_met)
{
    int result;
    result = postgresql_batch_insert_periodic_metrics(connection, per_met, 1);
    reverse_periodic_metric_bytes(per_met, 1);
    return result;
}

int postgresql_batch_insert_power_signatures(PGconn *connection, power_signature_t *sigs, int num_sigs)
{
    if (num_sigs < 0 || sigs == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    reverse_power_signature_bytes(sigs, num_sigs);

    /* Memory allocation */
    param_values = calloc(POWER_SIGNATURE_ARGS*num_sigs, sizeof(char *));
    param_lengths = calloc(POWER_SIGNATURE_ARGS*num_sigs, sizeof(int));
    param_formats = calloc(POWER_SIGNATURE_ARGS*num_sigs, sizeof(int));
    query = calloc((strlen(POWER_SIGNATURE_PSQL_QUERY)+(num_sigs*POWER_SIGNATURE_ARGS * 10)), sizeof(char));

    strcpy(query, POWER_SIGNATURE_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_sigs; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + POWER_SIGNATURE_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &sigs[i].DC_power;
        param_values[1  + offset] = (char *) &sigs[i].DRAM_power;
        param_values[2  + offset] = (char *) &sigs[i].PCK_power;
        param_values[3  + offset] = (char *) &sigs[i].EDP;
        param_values[4  + offset] = (char *) &sigs[i].max_DC_power;
        param_values[5  + offset] = (char *) &sigs[i].min_DC_power;
        param_values[6  + offset] = (char *) &sigs[i].time;
        param_values[7  + offset] = (char *) &sigs[i].avg_f;
        param_values[8 + offset] = (char *) &sigs[i].def_f;

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(sigs[i].DC_power);
        param_lengths[1  + offset] = sizeof(sigs[i].DRAM_power);
        param_lengths[2  + offset] = sizeof(sigs[i].PCK_power);
        param_lengths[3  + offset] = sizeof(sigs[i].EDP);
        param_lengths[4  + offset] = sizeof(sigs[i].max_DC_power);
        param_lengths[5  + offset] = sizeof(sigs[i].min_DC_power);
        param_lengths[6  + offset] = sizeof(sigs[i].time);
        param_lengths[7  + offset] = sizeof(int);
        param_lengths[8  + offset] = sizeof(int);


        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < POWER_SIGNATURE_ARGS; j++)
            param_formats[j + offset] = 1;

        offset += POWER_SIGNATURE_ARGS;
    }


    PGresult *res = PQexecParams(connection, query, POWER_SIGNATURE_ARGS * num_sigs, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting power_signatures: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }
    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_insert_power_signature(PGconn *connection, power_signature_t *pow_sig)
{
    int result;
    result = postgresql_batch_insert_power_signatures(connection, pow_sig, 1);
    reverse_power_signature_bytes(pow_sig, 1);
    return result;
}

#if USE_GPUS
int postgresql_batch_insert_gpu_signatures(PGconn *connection, signature_t *sigs, int num_sigs)
{
    if (num_sigs < 0 || sigs == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, k, offset, num_gpu_sigs, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    num_gpu_sigs = reverse_gpu_signature_bytes(sigs, num_sigs);

    if (num_gpu_sigs < 1) return EAR_ERROR;

    /* Memory allocation */
    param_values = calloc(GPU_SIGNATURE_ARGS*num_sigs, sizeof(char *));
    param_lengths = calloc(GPU_SIGNATURE_ARGS*num_sigs, sizeof(int));
    param_formats = calloc(GPU_SIGNATURE_ARGS*num_sigs, sizeof(int));
    query = calloc((strlen(GPU_SIGNATURE_PSQL_QUERY)+(num_gpu_sigs*GPU_SIGNATURE_ARGS * 10)), sizeof(char));

    strcpy(query, GPU_SIGNATURE_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_sigs; i++)
    {
        for (j = 0; j < sigs[i].gpu_sig.num_gpus; j++)
        {
            /* Query argument preparation */
            if (i > 0)
                strcat(query, ", (");
            else
                strcat(query, " (");

            sprintf(arg_number, "$%d", offset+1);
            strcat(query, arg_number);
            for (k = offset + 2; k < offset + GPU_SIGNATURE_ARGS + 1 ; k++)
            {
                sprintf(arg_number, ", $%d", k);
                strcat(query, arg_number);
            }
            strcat(query, ")");

            /* Parameter binding */
            param_values[0  + offset] = (char *) &sigs[i].gpu_sig.gpu_data[j].GPU_power;
            param_values[1  + offset] = (char *) &sigs[i].gpu_sig.gpu_data[j].GPU_freq;
            param_values[2  + offset] = (char *) &sigs[i].gpu_sig.gpu_data[j].GPU_mem_freq;
            param_values[3  + offset] = (char *) &sigs[i].gpu_sig.gpu_data[j].GPU_util;
            param_values[4  + offset] = (char *) &sigs[i].gpu_sig.gpu_data[j].GPU_mem_util;

            /* Parameter sizes */
            param_lengths[0  + offset] = sizeof(sigs[i].gpu_sig.gpu_data[j].GPU_power);
            param_lengths[1  + offset] = sizeof(sigs[i].gpu_sig.gpu_data[j].GPU_freq);
            param_lengths[2  + offset] = sizeof(sigs[i].gpu_sig.gpu_data[j].GPU_mem_freq);
            param_lengths[3  + offset] = sizeof(sigs[i].gpu_sig.gpu_data[j].GPU_util);
            param_lengths[4  + offset] = sizeof(sigs[i].gpu_sig.gpu_data[j].GPU_mem_util);


            /* Parameter formats, 1 is binary 0 is string */
            for (k = 0; k < GPU_SIGNATURE_ARGS; k++)
                param_formats[k + offset] = 1;

            offset += GPU_SIGNATURE_ARGS;
        }
    }


    PGresult *res = PQexecParams(connection, query, GPU_SIGNATURE_ARGS * num_sigs, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting gpu_signatures: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }
    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_insert_gpu_signature(PGconn *connection, signature_t *sig)
{
    int result;
    result = postgresql_batch_insert_gpu_signatures(connection, sig, 1);
    reverse_gpu_signature_bytes(sig, 1);
    return result;
}
#endif


int postgresql_batch_insert_signatures(PGconn *connection, signature_t *sigs, char is_learning,  int num_sigs)
{
    if (num_sigs < 0 || sigs == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query;
    char arg_number[16];

    reverse_signature_bytes(sigs, num_sigs);

    /* Memory allocation */
    param_values = calloc(sig_args*num_sigs, sizeof(char *));
    param_lengths = calloc(sig_args*num_sigs, sizeof(int));
    param_formats = calloc(sig_args*num_sigs, sizeof(int));
    query = calloc((strlen(SIGNATURE_QUERY_FULL)+(num_sigs*sig_args*10)+strlen("ON CONFLICT DO NOTHING")), sizeof(char));
#if USE_GPUS

    int starter_gpu_sig_id, num_gpu_sigs = 0, *gpu_sig_ids, current_gpu_sig_id = 0;
    /* Insert gpu signatures */
    starter_gpu_sig_id = postgresql_batch_insert_gpu_signatures(connection, sigs, num_sigs);
    if (starter_gpu_sig_id != EAR_SUCCESS)
    {
        verbose(VMYSQL, "Error while inserting gpu signatures\n");
    }
    else
    {
        /* Get last id */
        if ((starter_gpu_sig_id = postgresql_get_current_autoincrement_val(connection, "signatures")) < 1)
        {
            verbose(VMYSQL, "Unknown error while retrieving signature id\n");
        }
        else
        {
            for (i = 0; i < num_sigs; i++)
                if (sigs[i].gpu_sig.num_gpus > 0) num_gpu_sigs += sigs[i].gpu_sig.num_gpus;

            gpu_sig_ids = calloc(num_gpu_sigs, sizeof(int));
            for (i = 0; i < num_gpu_sigs; i++)
                gpu_sig_ids[num_gpu_sigs - 1 - i] = htonl(starter_gpu_sig_id - i);

        }
    }
#endif

    if (is_learning)
        strcpy(query, LEARNING_SIGNATURE_PSQL_QUERY);
    else
        strcpy(query, SIGNATURE_PSQL_QUERY);

    offset = 0;
    for (i = 0; i < num_sigs; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + sig_args + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &sigs[i].DC_power;
        param_values[1  + offset] = (char *) &sigs[i].DRAM_power;
        param_values[2  + offset] = (char *) &sigs[i].PCK_power;
        param_values[3  + offset] = (char *) &sigs[i].EDP;
        param_values[4  + offset] = (char *) &sigs[i].GBS;
        param_values[5  + offset] = (char *) &sigs[i].IO_MBS;
        param_values[6  + offset] = (char *) &sigs[i].TPI;
        param_values[7  + offset] = (char *) &sigs[i].CPI;
        param_values[8  + offset] = (char *) &sigs[i].Gflops;
        param_values[9  + offset] = (char *) &sigs[i].time;
        param_values[10 + offset] = (char *) &sigs[i].perc_MPI;
        if (full_signature)
        {
            param_values[11 + offset] = (char *) &sigs[i].FLOPS[0];
            param_values[12 + offset] = (char *) &sigs[i].FLOPS[1];
            param_values[13 + offset] = (char *) &sigs[i].FLOPS[2];
            param_values[14 + offset] = (char *) &sigs[i].FLOPS[3];
            param_values[15 + offset] = (char *) &sigs[i].FLOPS[4];
            param_values[16 + offset] = (char *) &sigs[i].FLOPS[5];
            param_values[17 + offset] = (char *) &sigs[i].FLOPS[6];
            param_values[18 + offset] = (char *) &sigs[i].FLOPS[7];
            param_values[19 + offset] = (char *) &sigs[i].instructions;
            param_values[20 + offset] = (char *) &sigs[i].cycles;
            param_values[21 + offset] = (char *) &sigs[i].avg_f;
            param_values[22 + offset] = (char *) &sigs[i].avg_imc_f;
            param_values[23 + offset] = (char *) &sigs[i].def_f;
#if USE_GPUS
            if (sigs[i].gpu_sig.num_gpus > 0 && starter_gpu_sig_id >= 0)
            {
                param_values[24 + offset] = (char *) &gpu_sig_ids[current_gpu_sig_id];
                current_gpu_sig_id += sigs[i].gpu_sig.num_gpus - 1;
                param_values[25 + offset] = (char *) &gpu_sig_ids[current_gpu_sig_id];
            }
            else
            {
                param_values[24 + offset] = NULL;
                param_values[25 + offset] = NULL;
            }
#endif
        }
        else 
        {
            param_values[11 + offset] = (char *) &sigs[i].avg_f;
            param_values[12 + offset] = (char *) &sigs[i].avg_imc_f;
            param_values[13 + offset] = (char *) &sigs[i].def_f;
#if USE_GPUS
            if (sigs[i].gpu_sig.num_gpus > 0 && starter_gpu_sig_id >= 0)
            {
                param_values[14 + offset] = (char *) &gpu_sig_ids[current_gpu_sig_id];
                current_gpu_sig_id += sigs[i].gpu_sig.num_gpus - 1;
                param_values[15 + offset] = (char *) &gpu_sig_ids[current_gpu_sig_id];
            }
            else
            {
                param_values[14 + offset] = NULL;
                param_values[15 + offset] = NULL;
            }
#endif
        }

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(sigs[i].DC_power);
        param_lengths[1  + offset] = sizeof(sigs[i].DRAM_power);
        param_lengths[2  + offset] = sizeof(sigs[i].PCK_power);
        param_lengths[3  + offset] = sizeof(sigs[i].EDP);
        param_lengths[4  + offset] = sizeof(sigs[i].GBS);
        param_lengths[5  + offset] = sizeof(sigs[i].IO_MBS);
        param_lengths[6  + offset] = sizeof(sigs[i].TPI);
        param_lengths[7  + offset] = sizeof(sigs[i].CPI);
        param_lengths[8  + offset] = sizeof(sigs[i].Gflops);
        param_lengths[9  + offset] = sizeof(sigs[i].time);
        param_lengths[10 + offset] = sizeof(sigs[i].perc_MPI);
        if (full_signature)
        {
            param_lengths[11 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[12 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[13 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[14 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[15 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[16 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[17 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[18 + offset] = sizeof(sigs[i].FLOPS[0]);
            param_lengths[19 + offset] = sizeof(sigs[i].instructions);
            param_lengths[20 + offset] = sizeof(sigs[i].cycles);
            param_lengths[21 + offset] = sizeof(int);
            param_lengths[22 + offset] = sizeof(int);
            param_lengths[23 + offset] = sizeof(int);
#if USE_GPUS
            param_lengths[24 + offset] = sizeof(ulong);
            param_lengths[25 + offset] = sizeof(ulong);
#endif
        }
        else 
        {
            param_lengths[11 + offset] = sizeof(int);
            param_lengths[12 + offset] = sizeof(int);
            param_lengths[13 + offset] = sizeof(int);
#if USE_GPUS
            param_lengths[14 + offset] = sizeof(ulong);
            param_lengths[15 + offset] = sizeof(ulong);
#endif

        }


        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < sig_args; j++)
            param_formats[j + offset] = 1;

        offset += sig_args;
    }

    PGresult *res = PQexecParams(connection, query, sig_args * num_sigs, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

#if USE_GPUS
    free(gpu_sig_ids);
#endif
    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting signatures: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    PQclear(res);
    return EAR_SUCCESS;

}

int postgresql_batch_insert_jobs(PGconn *connection, application_t *apps, int num_jobs)
{

    if (num_jobs < 0 || apps == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, *param_lengths, *param_formats;
    char *query, *lock_query, *insert_query;
    char arg_number[16];

    reverse_job_bytes(apps, num_jobs);

    /* Memory allocation */
    param_values = calloc(JOB_ARGS*num_jobs, sizeof(char *));
    param_lengths = calloc(JOB_ARGS*num_jobs, sizeof(int));
    param_formats = calloc(JOB_ARGS*num_jobs, sizeof(int));
    lock_query = calloc(strlen(LOCK_LEARNING_JOBS_PSQL_QUERY)+1, sizeof(char));
    insert_query = calloc(strlen(INSERT_NEW_LEARNING_JOBS)+1, sizeof(char));
    query = calloc((strlen(JOB_PSQL_QUERY)+(num_jobs*JOB_ARGS*10)+strlen("ON CONFLICT DO NOTHING")), sizeof(char));

    strcpy(query, JOB_PSQL_QUERY);
    if (apps[0].is_learning)
    {
        strcpy(lock_query, LOCK_LEARNING_JOBS_PSQL_QUERY);
        strcpy(insert_query, INSERT_NEW_LEARNING_JOBS);
    }
    else
    {
        strcpy(lock_query, LOCK_JOBS_PSQL_QUERY);
        strcpy(insert_query, INSERT_NEW_JOBS);
    }

    offset = 0;
    for (i = 0; i < num_jobs; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + JOB_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &apps[i].job.id;
        param_values[1  + offset] = (char *) &apps[i].job.step_id;
        param_values[2  + offset] = (char *) &apps[i].job.user_id;
        param_values[3  + offset] = (char *) &apps[i].job.app_id;
        param_values[4  + offset] = (char *) &apps[i].job.start_time;
        param_values[5  + offset] = (char *) &apps[i].job.end_time;
        param_values[6  + offset] = (char *) &apps[i].job.start_mpi_time;
        param_values[7  + offset] = (char *) &apps[i].job.end_mpi_time;
        param_values[8  + offset] = (char *) &apps[i].job.policy;
        param_values[9  + offset] = (char *) &apps[i].job.th;
        param_values[10 + offset] = (char *) &apps[i].job.procs;
        param_values[11 + offset] = (char *) &apps[i].job.type;
        param_values[12 + offset] = (char *) &apps[i].job.def_f;
        param_values[13 + offset] = (char *) &apps[i].job.user_acc;
        param_values[14 + offset] = (char *) &apps[i].job.group_id;
        param_values[15 + offset] = (char *) &apps[i].job.energy_tag;

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = sizeof apps[i].job.user_id;
        param_lengths[3  + offset] = sizeof apps[i].job.app_id;
        param_lengths[4  + offset] = sizeof(int);
        param_lengths[5  + offset] = sizeof(int);
        param_lengths[6  + offset] = sizeof(int);
        param_lengths[7  + offset] = sizeof(int);
        param_lengths[8  + offset] = sizeof apps[i].job.policy;
        param_lengths[9  + offset] = sizeof apps[i].job.th;
        param_lengths[10 + offset] = sizeof(int);
        param_lengths[11 + offset] = sizeof(short);
        param_lengths[12 + offset] = sizeof(int);
        param_lengths[13 + offset] = sizeof apps[i].job.user_acc;
        param_lengths[14 + offset] = sizeof apps[i].job.group_id;
        param_lengths[15 + offset] = sizeof apps[i].job.energy_tag;

        /* Parameter formats, 1 is binary 0 is string */
        param_formats[0  + offset] = 1;
        param_formats[1  + offset] = 1;
        param_formats[2  + offset] = 0;
        param_formats[3  + offset] = 0;
        param_formats[4  + offset] = 1;
        param_formats[5  + offset] = 1;
        param_formats[6  + offset] = 1;
        param_formats[7  + offset] = 1;
        param_formats[8  + offset] = 0;
        param_formats[9  + offset] = 1;
        param_formats[10 + offset] = 1;
        param_formats[11 + offset] = 1;
        param_formats[12 + offset] = 1;
        param_formats[13 + offset] = 0;
        param_formats[14 + offset] = 0;
        param_formats[15 + offset] = 0;

        offset += JOB_ARGS;
    }

    //printf("sending query: %s\n", query);

    PGresult *res;

    /* Query execution */
    res = PQexecParams(connection, "BEGIN", 0, NULL, NULL, NULL, NULL, 1); //BEGIN transaction block
    if (PQresultStatus(res) != PGRES_COMMAND_OK) verbose(VMYSQL, "ERROR while creating transaction block: %s\n", PQresultErrorMessage(res));
    PQclear(res); //This free may not be correct, if it sigsegv's in this function all but the last PQclear should be suspects

    res = PQexecParams(connection, CREATE_TEMP_JOBS, 0, NULL, NULL, NULL, NULL, 1); //CREATE temp table to store all jobs
    if (PQresultStatus(res) != PGRES_COMMAND_OK) verbose(VMYSQL, "ERROR while creating temp table: %s\n", PQresultErrorMessage(res));
    PQclear(res);

    res = PQexecParams(connection, lock_query, 0, NULL, NULL, NULL, NULL, 1); //LOCK main table
    if (PQresultStatus(res) != PGRES_COMMAND_OK) verbose(VMYSQL, "ERROR while creating locking main table: %s\n", PQresultErrorMessage(res));
    PQclear(res);

    res = PQexecParams(connection, query, num_jobs*JOB_ARGS, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //INSERT all jobs into temp table
    if (PQresultStatus(res) != PGRES_COMMAND_OK) verbose(VMYSQL, "ERROR while inserting to temp table: %s\n", PQresultErrorMessage(res));
    PQclear(res);

    res = PQexecParams(connection, insert_query, 0, NULL, NULL, NULL, NULL, 1); //INSERT new jobs into Jobs, ignore the rest
    if (PQresultStatus(res) != PGRES_COMMAND_OK) verbose(VMYSQL, "ERROR while merging with main table: %s\n", PQresultErrorMessage(res));
    PQclear(res);

    fprintf(stderr, "running query %s\n", "COMMIT");
    res = PQexecParams(connection, "COMMIT", 0, NULL, NULL, NULL, NULL, 1); //COMMIT and end transaction block, dropping the table

    //reverse_job_bytes(jobs, num_jobs);
    free(param_values);
    free(param_lengths);
    free(param_formats);
    free(insert_query);
    free(lock_query);
    free(query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        verbose(VMYSQL, "ERROR while inserting jobs: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    PQclear(res);

    return EAR_SUCCESS;

}

int postgresql_batch_insert_loops(PGconn *connection, loop_t *loops, int num_loops)
{

    if (num_loops < 0 || loops == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, sig_id, *param_lengths, *param_formats, *sig_ids;
    signature_t *sigs;
    char *query, arg_number[16];

    reverse_loop_bytes(loops, num_loops);

    /* Memory allocation */
    sigs = calloc(num_loops, sizeof(signature_t));
    sig_ids = calloc(num_loops, sizeof(int));
    param_values = calloc(LOOP_ARGS*num_loops, sizeof(char *));
    param_lengths = calloc(LOOP_ARGS*num_loops, sizeof(int));
    param_formats = calloc(LOOP_ARGS*num_loops, sizeof(int));
    query = calloc((strlen(LOOP_PSQL_QUERY)+(num_loops*LOOP_ARGS*10)+strlen("ON CONFLICT DO NOTHING")), sizeof(char));

    strcpy(query, LOOP_PSQL_QUERY);

    /* Previous inserts */
    for (i = 0; i < num_loops; i++)
        memcpy(&sigs[i], &loops[i].signature, sizeof(signature_t)); 

    postgresql_batch_insert_signatures(connection, sigs, 0, num_loops);

    if ((sig_id = postgresql_get_current_autoincrement_val(connection, "signatures")) < 1)
        verbose(VMYSQL, "Unknown error while retrieving signature id\n");

    for (i = 0; i < num_loops; i++)
        sig_ids[num_loops - 1 - i] = htonl(sig_id - i);

    offset = 0;
    for (i = 0; i < num_loops; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + LOOP_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &loops[i].id.event;
        param_values[1  + offset] = (char *) &loops[i].id.size;
        param_values[2  + offset] = (char *) &loops[i].id.level;
        param_values[3  + offset] = (char *) &loops[i].jid;
        param_values[4  + offset] = (char *) &loops[i].step_id;
        param_values[5  + offset] = (char *) &loops[i].node_id;
        param_values[6  + offset] = (char *) &loops[i].total_iterations;
        param_values[7  + offset] = (char *) &sig_ids[i];

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = sizeof(int);
        param_lengths[3  + offset] = sizeof(int);
        param_lengths[4  + offset] = sizeof(int);
        param_lengths[5  + offset] = strlen(loops[i].node_id);
        param_lengths[6  + offset] = sizeof(int);
        param_lengths[7  + offset] = sizeof(sig_ids[i]);

        /* Parameter formats, 1 is binary 0 is string */
        for (j = 0; j < LOOP_ARGS; j++)
            param_formats[j + offset] = 1;
        param_formats[5 + offset] = 0;

        offset += LOOP_ARGS;
    }


    PGresult *res = PQexecParams(connection, query, num_loops * LOOP_ARGS, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(sigs);
    free(query);
    free(sig_ids);
    free(param_values);
    free(param_formats);
    free(param_lengths);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting loops: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    PQclear(res);
    return EAR_SUCCESS;
}

int postgresql_insert_loop(PGconn *connection, loop_t *loop)
{
    int result;
    result = postgresql_batch_insert_loops(connection, loop, 1);
    reverse_loop_bytes(loop, 1);
    reverse_signature_bytes(&loop->signature, 1);
    return result;
}

int postgresql_batch_insert_applications(PGconn *connection, application_t *apps, int num_apps)
{

    if (num_apps < 0 || apps == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, pow_sig_id, sig_id, *param_lengths, *param_formats, *sig_ids, *pow_sig_ids;
    signature_t *sigs;
    power_signature_t *pow_sigs;
    char *query, arg_number[16];
    char is_learning = apps[0].is_learning; 
    char is_mpi      = apps[0].is_mpi; 

    /* Memory allocation */
    sigs = calloc(num_apps, sizeof(signature_t));
    pow_sigs = calloc(num_apps, sizeof(power_signature_t));
    sig_ids = calloc(num_apps, sizeof(int));
    pow_sig_ids = calloc(num_apps, sizeof(int));
    param_values = calloc(APPLICATION_ARGS*num_apps, sizeof(char *));
    param_lengths = calloc(APPLICATION_ARGS*num_apps, sizeof(int));
    param_formats = calloc(APPLICATION_ARGS*num_apps, sizeof(int));
    query = calloc((strlen(LEARNING_APPLICATION_PSQL_QUERY)+(num_apps*APPLICATION_ARGS*10)+strlen("ON CONFLICT DO NOTHING")), sizeof(char));

    if (is_learning)
        strcpy(query, LEARNING_APPLICATION_PSQL_QUERY);
    else
        strcpy(query, APPLICATION_PSQL_QUERY);

    /* Previous inserts */
    for (i = 0; i < num_apps; i++)
    {
        memcpy(&pow_sigs[i], &apps[i].power_sig, sizeof(power_signature_t)); 
        if (is_mpi)
            memcpy(&sigs[i], &apps[i].signature, sizeof(signature_t)); 
    }

    if (is_mpi) {
        postgresql_batch_insert_signatures(connection, sigs, is_learning, num_apps);
    }
    postgresql_batch_insert_power_signatures(connection, pow_sigs, num_apps);
    postgresql_batch_insert_jobs(connection, apps, num_apps); //all job data will be reversed and remain that way


    if (is_mpi) 
    {
        if (is_learning)
        {
            if ((sig_id = postgresql_get_current_autoincrement_val(connection, "learning_signatures")) < 1)
                verbose(VMYSQL, "Unknown error while retrieving signature id\n");
        }
        else
        {
            if ((sig_id = postgresql_get_current_autoincrement_val(connection, "signatures")) < 1)
                verbose(VMYSQL, "Unknown error while retrieving signature id\n");
        }
    }
    if ((pow_sig_id = postgresql_get_current_autoincrement_val(connection, "power_signatures")) < 1)
        verbose(VMYSQL, "Unknown error while retrieving power signature id (%d)\n", pow_sig_id);

    for (i = 0; i < num_apps; i++)
    {
        pow_sig_ids[num_apps - 1 - i] = htonl(pow_sig_id - i);
        if (is_mpi)
            sig_ids[num_apps - 1 - i] = htonl(sig_id - i);
    }

    offset = 0;
    for (i = 0; i < num_apps; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + APPLICATION_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &apps[i].job.id;
        param_values[1  + offset] = (char *) &apps[i].job.step_id;
        param_values[2  + offset] = (char *) &apps[i].node_id;
        if (is_mpi)
            param_values[3  + offset] = (char *) &sig_ids[i];
        else
            param_values[3  + offset] = (char *) NULL;

        param_values[4  + offset] = (char *) &pow_sig_ids[i];

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = strlen(apps[i].node_id);
        param_lengths[3  + offset] = sizeof(sig_ids[i]);
        param_lengths[4  + offset] = sizeof(pow_sig_ids[i]);

        /* Parameter formats, 1 is binary 0 is string */
        param_formats[0  + offset] = 1;
        param_formats[1  + offset] = 1;
        param_formats[2  + offset] = 0;
        param_formats[3  + offset] = 1;
        param_formats[4  + offset] = 1;

        offset += APPLICATION_ARGS;
    }


    PGresult *res = PQexecParams(connection, query, num_apps * APPLICATION_ARGS, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(sigs);
    free(query);
    free(sig_ids);
    free(pow_sigs);
    free(pow_sig_ids);
    free(param_values);
    free(param_formats);
    free(param_lengths);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting applications: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }


    PQclear(res);
    return EAR_SUCCESS;
}


int postgresql_batch_insert_applications_no_mpi(PGconn *connection, application_t *apps, int num_apps)
{

    if (num_apps < 0 || apps == NULL)
        return EAR_ERROR;

    char **param_values;
    int i, j, offset, pow_sig_id, *param_lengths, *param_formats, *sig_ids, *pow_sig_ids;
    power_signature_t *pow_sigs;
    char *query, arg_number[16];
    char is_learning = apps[0].is_learning; 

    /* Memory allocation */
    pow_sigs = calloc(num_apps, sizeof(power_signature_t));
    pow_sig_ids = calloc(num_apps, sizeof(int));
    param_values = calloc(APPLICATION_ARGS*num_apps, sizeof(char *));
    param_lengths = calloc(APPLICATION_ARGS*num_apps, sizeof(int));
    param_formats = calloc(APPLICATION_ARGS*num_apps, sizeof(int));
    query = calloc((strlen(LEARNING_APPLICATION_PSQL_QUERY)+(num_apps*APPLICATION_ARGS*10)+strlen("ON CONFLICT DO NOTHING")), sizeof(char));

    if (is_learning)
        strcpy(query, LEARNING_APPLICATION_PSQL_QUERY);
    else
        strcpy(query, APPLICATION_PSQL_QUERY);

    /* Previous inserts */
    for (i = 0; i < num_apps; i++)
        memcpy(&pow_sigs[i], &apps[i].power_sig, sizeof(power_signature_t)); 

    postgresql_batch_insert_power_signatures(connection, pow_sigs, num_apps);
    postgresql_batch_insert_jobs(connection, apps, num_apps); //all job data will be reversed and remain that way


    if ((pow_sig_id = postgresql_get_current_autoincrement_val(connection, "power_signatures")) < 1)
        verbose(VMYSQL, "Unknown error while retrieving power signature id (%d)\n", pow_sig_id);

    for (i = 0; i < num_apps; i++)
        pow_sig_ids[num_apps - 1 - i] = htonl(pow_sig_id - i);

    offset = 0;
    for (i = 0; i < num_apps; i++)
    {

        /* Query argument preparation */
        if (i > 0)
            strcat(query, ", (");
        else
            strcat(query, " (");

        sprintf(arg_number, "$%d", offset+1);
        strcat(query, arg_number);
        for (j = offset + 2; j < offset + APPLICATION_ARGS + 1 ; j++)
        {
            sprintf(arg_number, ", $%d", j);
            strcat(query, arg_number);
        }
        strcat(query, ")");

        /* Parameter binding */
        param_values[0  + offset] = (char *) &apps[i].job.id;
        param_values[1  + offset] = (char *) &apps[i].job.step_id;
        param_values[2  + offset] = (char *) &apps[i].node_id;
        // param_values[3  + offset] = (char *) &sig_ids[i];
        param_values[3  + offset] = (char *) NULL;
        param_values[4  + offset] = (char *) &pow_sig_ids[i];

        /* Parameter sizes */
        param_lengths[0  + offset] = sizeof(int);
        param_lengths[1  + offset] = sizeof(int);
        param_lengths[2  + offset] = strlen(apps[i].node_id);
        param_lengths[3  + offset] = sizeof(sig_ids[i]);
        param_lengths[4  + offset] = sizeof(pow_sig_ids[i]);

        /* Parameter formats, 1 is binary 0 is string */
        param_formats[0  + offset] = 1;
        param_formats[1  + offset] = 1;
        param_formats[2  + offset] = 0;
        param_formats[3  + offset] = 1;
        param_formats[4  + offset] = 1;

        offset += APPLICATION_ARGS;
    }


    PGresult *res = PQexecParams(connection, query, num_apps * APPLICATION_ARGS, NULL, (const char * const *)param_values, param_lengths, param_formats, 1); //0 indicates text mode, 1 is binary

    free(query);
    free(pow_sigs);
    free(pow_sig_ids);
    free(param_values);
    free(param_formats);
    free(param_lengths);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) //-> command sense lectura
    {
        verbose(VMYSQL, "ERROR while inserting applications: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }


    PQclear(res);
    return EAR_SUCCESS;
}

int postgresql_insert_application(PGconn *connection, application_t *app)
{
    int result;
    result = postgresql_batch_insert_applications(connection, app, 1);
    reverse_job_bytes(app, 1);
    reverse_signature_bytes(&app->signature, 1);
    reverse_power_signature_bytes(&app->power_sig, 1);
    return result;
}
#endif
