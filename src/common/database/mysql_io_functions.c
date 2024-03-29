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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>

#if DB_MYSQL
#include <common/database/mysql_io_functions.h>

typedef typeof((MYSQL_BIND){0}.is_null) ear_my_bool;

#define APPLICATION_MYSQL_QUERY   "INSERT INTO Applications (job_id, step_id, node_id, signature_id, power_signature_id) VALUES" \
                            "(?, ?, ?, ?, ?)"


#define LOOP_MYSQL_QUERY        "INSERT INTO Loops (event, size, level, job_id, step_id, node_id, total_iterations," \
                                "signature_id) VALUES (?, ?, ?, ?, ?, ?, ? ,?)"


#define JOB_MYSQL_QUERY         "INSERT IGNORE INTO Jobs (id, step_id, user_id, app_id, start_time, end_time, start_mpi_time," \
                                "end_mpi_time, policy, threshold, procs, job_type, def_f, user_acc, user_group, e_tag) VALUES" \
                                "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#if USE_GPUS
#define SIGNATURE_QUERY_FULL    "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, L1_misses, L2_misses, L3_misses, "\
                                "FLOPS1, FLOPS2, FLOPS3, FLOPS4, FLOPS5, FLOPS6, FLOPS7, FLOPS8, "\
                                "instructions, cycles, avg_f, avg_imc_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES "\
                                "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define SIGNATURE_QUERY_SIMPLE  "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, avg_f, avg_imc_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES "\
                                "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
#else
#define SIGNATURE_QUERY_FULL    "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, L1_misses, L2_misses, L3_misses, "\
                                "FLOPS1, FLOPS2, FLOPS3, FLOPS4, FLOPS5, FLOPS6, FLOPS7, FLOPS8, "\
                                "instructions, cycles, avg_f, avg_imc_f, def_f) VALUES "\
                                "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define SIGNATURE_QUERY_SIMPLE  "INSERT INTO Signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, avg_f, avg_imc_f, def_f) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, "\
                                "?, ?, ?, ?, ?)"
#endif

#define AVG_SIGNATURE_QUERY_FULL        "INSERT INTO Signatures_avg (DC_power, DRAM_power, PCK_power, EDP,"\
                                        "GBS, TPI, CPI, Gflops, time, FLOPS1, FLOPS2, FLOPS3, FLOPS4, "\
                                        "FLOPS5, FLOPS6, FLOPS7, FLOPS8,"\
                                        "instructions, cycles, avg_f, def_f, job_id, step_id, nodes_counter) VALUES (?, ?, ?, ?, ?, ?, ?, ?, "\
                                        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define AVG_SIGNATURE_QUERY_SIMPLE      "INSERT INTO Signatures_avg (DC_power, DRAM_power, PCK_power, EDP,"\
                                        "GBS, TPI, CPI, Gflops, time, avg_f, def_f, job_id, step_id, nodes_counter) VALUES (?, ?, ?, ?, ?, ?, ?, ?, "\
                                         "?, ?, ?, ?, ?, ?)"

#define AVG_SIG_ENDING          "ON DUPLICATE KEY UPDATE "\
                                "DC_power = DC_power + (VALUES(DC_power) - DC_power)/nodes_counter, "\
                                "DRAM_power = DRAM_power + (VALUES(DRAM_power) - DRAM_power)/nodes_counter, "\
                                "PCK_power = PCK_power + (VALUES(PCK_power) - PCK_power)/nodes_counter, "\
                                "EDP = EDP + (VALUES(EDP) - EDP)/nodes_counter, "\
                                "GBS = GBS + (VALUES(GBS) - GBS)/nodes_counter, "\
                                "TPI = TPI + (VALUES(TPI) - TPI)/nodes_counter, "\
                                "CPI = CPI + (VALUES(CPI) - CPI)/nodes_counter, "\
                                "Gflops = Gflops + (VALUES(Gflops) - Gflops)/nodes_counter, "\
                                "time = time + (VALUES(time) - time)/nodes_counter, "\
                                "avg_f = avg_f + (VALUES(avg_f) - avg_f)/nodes_counter, "\
                                "def_f = def_f + (VALUES(def_f) - def_f)/nodes_counter, "\
                                "nodes_counter = nodes_counter + 1"                                

#define POWER_SIGNATURE_MYSQL_QUERY   "INSERT INTO Power_signatures (DC_power, DRAM_power, PCK_power, EDP, max_DC_power, min_DC_power, "\
                                "time, avg_f, def_f) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"

#if USE_GPUS
#define GPU_SIGNATURE_MYSQL_QUERY   "INSERT INTO GPU_signatures(GPU_power, GPU_freq, GPU_mem_freq, GPU_util, GPU_mem_util) VALUES (?, ?, ?, ?, ?)"
#endif

#if USE_GPUS
#define PERIODIC_METRIC_QUERY_DETAIL    "INSERT INTO Periodic_metrics (start_time, end_time, DC_energy, node_id, job_id, step_id, avg_f, temp, DRAM_energy, PCK_energy, GPU_energy)"\
                                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
#else
#define PERIODIC_METRIC_QUERY_DETAIL    "INSERT INTO Periodic_metrics (start_time, end_time, DC_energy, node_id, job_id, step_id, avg_f, temp, DRAM_energy, PCK_energy)"\
                                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
#endif

#define PERIODIC_METRIC_QUERY_SIMPLE    "INSERT INTO Periodic_metrics (start_time, end_time, DC_energy, node_id, job_id, step_id)"\
                                        "VALUES (?, ?, ?, ?, ?, ?)"

#define PERIODIC_AGGREGATION_MYSQL_QUERY "INSERT INTO Periodic_aggregations (DC_energy, start_time, end_time, eardbd_host) VALUES (?, ?, ?, ?)"

#define EAR_EVENT_MYSQL_QUERY         "INSERT INTO Events (timestamp, event_type, job_id, step_id, value, node_id) VALUES (?, ?, ?, ?, ?, ?)"

#if EXP_EARGM
#define EAR_WARNING_MYSQL_QUERY "INSERT INTO Global_energy2 (energy_percent, warning_level, inc_th, p_state, GlobEnergyConsumedT1, "\
                                "GlobEnergyConsumedT2, GlobEnergyLimit, GlobEnergyPeriodT1, GlobEnergyPeriodT2, GlobEnergyPolicy) "\
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
#else
#define EAR_WARNING_MYSQL_QUERY "INSERT INTO Global_energy (energy_percent, warning_level, inc_th, p_state, GlobEnergyConsumedT1, "\
                                "GlobEnergyConsumedT2, GlobEnergyLimit, GlobEnergyPeriodT1, GlobEnergyPeriodT2, GlobEnergyPolicy) "\
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
#endif

//Learning_phase insert queries
#define LEARNING_APPLICATION_MYSQL_QUERY  "INSERT INTO Learning_applications (job_id, step_id, node_id, "\
                                    "signature_id, power_signature_id) VALUES (?, ?, ?, ?, ?)"
#if USE_GPUS
#define LEARNING_SIGNATURE_QUERY_FULL    "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, L1_misses, L2_misses, L3_misses, "\
                                "FLOPS1, FLOPS2, FLOPS3, FLOPS4, FLOPS5, FLOPS6, FLOPS7, FLOPS8, "\
                                "instructions, cycles, avg_f, avg_imc_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES "\
                                "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,?, ?, ?)"

#define LEARNING_SIGNATURE_QUERY_SIMPLE  "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power,  EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, avg_f, avg_imc_f, def_f, min_GPU_sig_id, max_GPU_sig_id) VALUES "\
                                "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#else
#define LEARNING_SIGNATURE_QUERY_FULL    "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, L1_misses, L2_misses, L3_misses, "\
                                "FLOPS1, FLOPS2, FLOPS3, FLOPS4, FLOPS5, FLOPS6, FLOPS7, FLOPS8, "\
                                "instructions, cycles, avg_f, avg_imc_f, def_f) "\
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define LEARNING_SIGNATURE_QUERY_SIMPLE    "INSERT INTO Learning_signatures (DC_power, DRAM_power, PCK_power, EDP,"\
                                           "GBS, IO_MBS, TPI, CPI, Gflops, time, perc_MPI, avg_f, avg_imc_f, def_f) "\
                                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
#endif

#define LEARNING_JOB_MYSQL_QUERY          "INSERT IGNORE INTO Learning_jobs (id, step_id, user_id, app_id, start_time, end_time, "\
                                    "start_mpi_time, end_mpi_time, policy, threshold, procs, job_type, def_f, user_acc, user_group, e_tag) VALUES" \
                                    "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define AUTO_MYSQL_QUERY "SHOW VARIABLES LIKE 'auto_increment_inc%%'"



/*char *LEARNING_SIGNATURE_MYSQL_QUERY;
char *SIGNATURE_MYSQL_QUERY;
char *AVG_SIGNATURE_MYSQL_QUERY;*/
static char full_signature = !DB_SIMPLE;
static char node_detail = 1;

#if !DB_SIMPLE
char *LEARNING_SIGNATURE_MYSQL_QUERY = LEARNING_SIGNATURE_QUERY_FULL;
char *SIGNATURE_MYSQL_QUERY = SIGNATURE_QUERY_FULL;    
char *AVG_SIGNATURE_MYSQL_QUERY = AVG_SIGNATURE_QUERY_FULL;      
#else
char *LEARNING_SIGNATURE_MYSQL_QUERY = LEARNING_SIGNATURE_QUERY_SIMPLE;
char *SIGNATURE_MYSQL_QUERY = SIGNATURE_QUERY_SIMPLE;    
char *AVG_SIGNATURE_MYSQL_QUERY = AVG_SIGNATURE_QUERY_SIMPLE;      
#endif

char *PERIODIC_METRIC_MYSQL_QUERY = PERIODIC_METRIC_QUERY_DETAIL;

long autoincrement_offset = 0;

#define xmalloc(size) _xmalloc(size, __FUNCTION__)

void *_xmalloc(size_t size, const char *caller_function) 
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        error("%s: Error allocationg memory (malloc fails when trying to alloc %lu), exiting", caller_function, size);
        exit(1);
    }
    return ptr;
}

void *xcalloc(size_t nmemb, size_t size) 
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL)
    {
        error("Error allocationg memory (calloc fails), exiting");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *optr, size_t size) 
{
    void *ptr = realloc(optr, size);
    //is this really a good idea? On failure the initial pointer is still valid but the size will not have
    //increased/decreased. On most uses the failure of realloc would also imply a segfault later, but when
    //freeing exceed memory this could be overkill
    if (ptr == NULL)
    {
        error("Error allocationg memory (realloc fails), exiting");
        exit(1);
    }
    return ptr;
}


void set_signature_simple(char full_sig)
{
    full_signature = full_sig;
    if (full_signature)
    {
        LEARNING_SIGNATURE_MYSQL_QUERY = LEARNING_SIGNATURE_QUERY_FULL;
        SIGNATURE_MYSQL_QUERY = SIGNATURE_QUERY_FULL;    
        AVG_SIGNATURE_MYSQL_QUERY = AVG_SIGNATURE_QUERY_FULL;      
    }
    else
    {
        LEARNING_SIGNATURE_MYSQL_QUERY = LEARNING_SIGNATURE_QUERY_SIMPLE;
        SIGNATURE_MYSQL_QUERY = SIGNATURE_QUERY_SIMPLE;    
        AVG_SIGNATURE_MYSQL_QUERY = AVG_SIGNATURE_QUERY_SIMPLE;     
    }

}

void set_node_detail(char node_det)
{
    node_detail = node_det;
    if (node_detail)
        PERIODIC_METRIC_MYSQL_QUERY = PERIODIC_METRIC_QUERY_DETAIL;
    else
        PERIODIC_METRIC_MYSQL_QUERY = PERIODIC_METRIC_QUERY_SIMPLE;
}

int mysql_statement_error(MYSQL_STMT *statement)
{
    verbose(VMYSQL, "MYSQL statement error (%d): %s\n", mysql_stmt_errno(statement), mysql_stmt_error(statement));
    mysql_stmt_close(statement);
    return EAR_MYSQL_STMT_ERROR;
}

int get_autoincrement(MYSQL *connection, long *acum)
{

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement)
    {
        return EAR_MYSQL_ERROR;
    }

    if (mysql_stmt_prepare(statement, AUTO_MYSQL_QUERY, strlen(AUTO_MYSQL_QUERY)))
            return mysql_statement_error(statement);

    //Result parameters
    char name[256];
    MYSQL_BIND res_bind[2];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    res_bind[1].buffer = acum;
    res_bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    res_bind[0].buffer = &name;
    res_bind[0].buffer_length = strlen(name);

    if (mysql_stmt_bind_result(statement, res_bind)) return mysql_statement_error(statement);
    if (mysql_stmt_execute(statement)) return mysql_statement_error(statement);
    if (mysql_stmt_store_result(statement)) return mysql_statement_error(statement);
    int status = mysql_stmt_fetch(statement);
    
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
    {
        return mysql_statement_error(statement);
    }

    mysql_stmt_close(statement);

    return EAR_SUCCESS;

}


int mysql_insert_application(MYSQL *connection, application_t *app)
{
    return mysql_batch_insert_applications(connection, app, 1);
}

int mysql_batch_insert_applications(MYSQL *connection, application_t *app, int num_apps)
{

    if (app == NULL)
    {
        verbose(VMYSQL, "APP is null.");
        return EAR_ERROR;
    }
    else if (num_apps < 1)
    {
        verbose(VMYSQL, "Num_apps < 1 (%d)", num_apps);
        return EAR_ERROR;
    }
    
    char is_learning = app[0].is_learning & USE_LEARNING_APPS;
    char is_mpi      = app[0].is_mpi;
    
    int i;
	#if 0
    for (i = 0; i < num_apps; i++)
        report_application_data(&app[i]);
	#endif

   
    //job only needs to be inserted once
    mysql_batch_insert_jobs(connection, app, num_apps);
    
    long long pow_sig_id = 0;
    long long sig_id = 0;
    long long *pow_sigs_ids = xcalloc(num_apps, sizeof(long long));
    long long *sigs_ids = xcalloc(num_apps, sizeof(long long));
    
    if (autoincrement_offset < 1)
    {
        verbose(VMYSQL, "autoincrement_offset not set, reading from database...");
        if (get_autoincrement(connection, &autoincrement_offset) != EAR_SUCCESS)
        {
            verbose(VMYSQL, "error reading offset, setting to default (1)");
            autoincrement_offset = 1;
        }
        verbose(VMYSQL, "autoincrement_offset set to %ld\n", autoincrement_offset);
    }

    //inserting all powersignatures (always present)
    pow_sig_id = mysql_batch_insert_power_signatures(connection, app, num_apps);
    
    if (pow_sig_id < 0)
        verbose(VMYSQL,"Unknown error when writing power_signature to database.\n");

    for (i = 0; i < num_apps; i++)
        pow_sigs_ids[i] = pow_sig_id + i*autoincrement_offset;

    //inserting signatures (if the application is mpi)
    signature_container_t cont;
    cont.type = EAR_TYPE_APPLICATION;
    cont.app = app;

    if (is_mpi)
    {
        sig_id = mysql_batch_insert_signatures(connection, cont, is_learning, num_apps);

        if (sig_id < 0)
            verbose(VMYSQL,"Unknown error when writing signature to database. (%lld)\n",sig_id);

        for (i = 0; i < num_apps; i++)
            sigs_ids[i] = sig_id + i*autoincrement_offset;
    }

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) {
        free(pow_sigs_ids);
        if (is_mpi) free(sigs_ids);
        return EAR_MYSQL_ERROR;
    }

    char *params = ", (?, ?, ?, ?, ?)";
    char *query;

    if (!is_learning)
    {
        query = xmalloc(strlen(APPLICATION_MYSQL_QUERY)+strlen(params)*(num_apps-1)+1);
        strcpy(query, APPLICATION_MYSQL_QUERY);
    }
    else
    {
        query = xmalloc(strlen(LEARNING_APPLICATION_MYSQL_QUERY)+strlen(params)*(num_apps-1)+1);
        strcpy(query, LEARNING_APPLICATION_MYSQL_QUERY);
    }

    for (i = 1; i < num_apps; i++)
        strcat(query, params);

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(pow_sigs_ids);
        if (is_mpi) free(sigs_ids);
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_apps*APPLICATION_ARGS, sizeof(MYSQL_BIND));

    //binding preparations
    for (i = 0; i < num_apps; i++)
    {
        int offset = i*APPLICATION_ARGS;
        //integer types
        bind[0+offset].buffer_type = bind[1+offset].buffer_type = bind[3+offset].buffer_type = bind[4+offset].buffer_type = MYSQL_TYPE_LONG;
        bind[0+offset].is_unsigned = bind[1+offset].is_unsigned = bind[3+offset].is_unsigned = bind[4+offset].is_unsigned = 1;
        if (!is_mpi) {
            bind[3+offset].buffer_type = MYSQL_TYPE_NULL;
            bind[3+offset].is_null = (ear_my_bool) 1;
        }

        //string types
        bind[2+offset].buffer_type = MYSQL_TYPE_VAR_STRING;
        bind[2+offset].buffer_length = strlen(app[i].node_id);

        //storage variable assignation
        bind[0+offset].buffer = (char *)&app[i].job.id;
        bind[1+offset].buffer = (char *)&app[i].job.step_id;
        bind[2+offset].buffer = (char *)&app[i].node_id;
        if (is_mpi) 
            bind[3+offset].buffer = (char *) &sigs_ids[i];
        else 
            bind[3+offset].buffer = (char *) NULL;
        bind[4+offset].buffer = (char *)&pow_sigs_ids[i]; 

    }

    int ret = EAR_SUCCESS;
    if (mysql_stmt_bind_param(statement, bind)) ret = mysql_statement_error(statement);

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_close(statement)) ret = EAR_MYSQL_ERROR;
    }

    free(bind);
    free(query);
    free(pow_sigs_ids);
    if (is_mpi) free(sigs_ids);

    return ret;
}

int mysql_batch_insert_jobs(MYSQL *connection, application_t *app, int num_apps)
{
    char is_learning = app[0].is_learning & USE_LEARNING_APPS;

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    char *query;
    char *params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    if (!is_learning)
    {
        query = xmalloc(strlen(JOB_MYSQL_QUERY)+strlen(params)*(num_apps-1)+1);
        strcpy(query, JOB_MYSQL_QUERY);
    }
    else
    {
        query = xmalloc(strlen(LEARNING_JOB_MYSQL_QUERY)+strlen(params)*(num_apps-1)+1);
        strcpy(query, LEARNING_JOB_MYSQL_QUERY);
    }

    int i;
    for (i = 1; i < num_apps; i++)
        strcat(query, params);

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }
    MYSQL_BIND *bind = xcalloc(num_apps * JOB_ARGS, sizeof(MYSQL_BIND));

    //integer types
    for (i = 0; i < num_apps; i++)
    {
        int offset = i*JOB_ARGS;

        bind[0+offset].buffer_type = bind[1+offset].buffer_type = bind[4+offset].buffer_type = bind[5+offset].buffer_type = bind[6+offset].buffer_type
            = bind[7+offset].buffer_type = bind[10+offset].buffer_type = bind[12+offset].buffer_type = MYSQL_TYPE_LONG;

        bind[0+offset].is_unsigned = bind[10+offset].is_unsigned = bind[1+offset].is_unsigned = 1;

        //string types
        bind[2+offset].buffer_type = bind[3+offset].buffer_type = bind[8+offset].buffer_type = 
            bind[13+offset].buffer_type = bind[14+offset].buffer_type = bind[15+offset].buffer_type = MYSQL_TYPE_STRING;

        bind[2+offset].buffer_length = strlen(app[i].job.user_id);
        bind[3+offset].buffer_length = strlen(app[i].job.app_id);
        bind[8+offset].buffer_length = strlen(app[i].job.policy);
        bind[13+offset].buffer_length = strlen(app[i].job.user_acc);
        bind[14+offset].buffer_length = strlen(app[i].job.group_id);
        bind[15+offset].buffer_length = strlen(app[i].job.energy_tag);

        //double types
        bind[9+offset].buffer_type = MYSQL_TYPE_DOUBLE;

        //storage variable assignation
        bind[0+offset].buffer = (char *)&app[i].job.id;
        bind[1+offset].buffer = (char *)&app[i].job.step_id;
        bind[2+offset].buffer = (char *)&app[i].job.user_id;
        bind[3+offset].buffer = (char *)&app[i].job.app_id;
        bind[4+offset].buffer = (char *)&app[i].job.start_time;
        bind[5+offset].buffer = (char *)&app[i].job.end_time;
        bind[6+offset].buffer = (char *)&app[i].job.start_mpi_time;
        bind[7+offset].buffer = (char *)&app[i].job.end_mpi_time;
        bind[8+offset].buffer = (char *)&app[i].job.policy;
        bind[9+offset].buffer = (char *)&app[i].job.th;
        bind[10+offset].buffer = (char *)&app[i].job.procs;
        bind[11+offset].buffer = (char *)&app[i].job.type;
        bind[12+offset].buffer = (char *)&app[i].job.def_f;
        bind[13+offset].buffer = (char *)&app[i].job.user_acc;
        bind[14+offset].buffer = (char *)&app[i].job.group_id;
        bind[15+offset].buffer = (char *)&app[i].job.energy_tag;
    }
    int ret = EAR_SUCCESS;
    if (mysql_stmt_bind_param(statement, bind)) ret = mysql_statement_error(statement);

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_close(statement)) ret = EAR_MYSQL_ERROR;
    }

    free(query);
    free(bind);

    return ret;
}


int mysql_batch_insert_applications_no_mpi(MYSQL *connection, application_t *app, int num_apps)
{
    char is_learning = app[0].is_learning & USE_LEARNING_APPS;
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    char *params = ", (?, ?, ?, ?, ?)";
    char *query;
    int i;

    if (!is_learning)
    {
        query = xmalloc(strlen(APPLICATION_MYSQL_QUERY)+strlen(params)*num_apps+1);
        strcpy(query, APPLICATION_MYSQL_QUERY);
    }
    else
    {
        query = xmalloc(strlen(LEARNING_APPLICATION_MYSQL_QUERY)+strlen(params)*num_apps+1);
        strcpy(query, LEARNING_APPLICATION_MYSQL_QUERY);
    }

    for (i = 1; i < num_apps; i++)
        strcat(query, params);

    if (autoincrement_offset < 1)
    {
        verbose(VMYSQL, "autoincrement_offset not set, reading from database...");
        if (get_autoincrement(connection, &autoincrement_offset) != EAR_SUCCESS)
        {
            verbose(VMYSQL, "error reading offset, setting to default (1)");
            autoincrement_offset = 1;
        }
        verbose(VMYSQL, "autoincrement_offset set to %ld\n", autoincrement_offset);
    }

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_apps*APPLICATION_ARGS, sizeof(MYSQL_BIND));

    //job only needs to be inserted once
    mysql_batch_insert_jobs(connection, app, num_apps);
    long long pow_sig_id = 0;
    long long *pow_sigs_ids = xcalloc(num_apps, sizeof(long long));

    //inserting all powersignatures (always present)
    pow_sig_id = mysql_batch_insert_power_signatures(connection, app, num_apps);

    if (pow_sig_id < 0)
        verbose(VMYSQL,"Unknown error when writing power_signature to database.\n");

    for (i = 0; i < num_apps; i++)
        pow_sigs_ids[i] = pow_sig_id + i*autoincrement_offset;


    //binding preparations
    for (i = 0; i < num_apps; i++)
    {
        int offset = i*APPLICATION_ARGS;
        //integer types
        bind[0+offset].buffer_type = bind[1+offset].buffer_type = bind[3+offset].buffer_type = bind[4+offset].buffer_type = MYSQL_TYPE_LONG;
        bind[0+offset].is_unsigned = bind[1+offset].is_unsigned = bind[3+offset].is_unsigned = bind[4+offset].is_unsigned = 1;

        bind[3+offset].buffer_type = MYSQL_TYPE_NULL;
        bind[3+offset].is_null = (ear_my_bool) 1;

        //string types
        bind[2+offset].buffer_type = MYSQL_TYPE_STRING;
        bind[2+offset].buffer_length = strlen(app[i].node_id);

        //storage variable assignation
        bind[0+offset].buffer = (char *)&app[i].job.id;
        bind[1+offset].buffer = (char *)&app[i].job.step_id;
        bind[2+offset].buffer = (char *)&app[i].node_id;
        bind[3+offset].buffer = (char *) NULL;
        bind[4+offset].buffer = (char *)&pow_sigs_ids[i]; 

    }

    int ret = EAR_SUCCESS;
    if (mysql_stmt_bind_param(statement, bind)) ret = mysql_statement_error(statement);

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_close(statement)) ret = EAR_MYSQL_ERROR;
    }

    free(bind);
    free(query);
    free(pow_sigs_ids);

    return ret;
}

int mysql_retrieve_applications(MYSQL *connection, char *query, application_t **apps, char is_learning)
{
	is_learning &= USE_LEARNING_APPS;
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    application_t *app_aux = xcalloc(1, sizeof(application_t));
    application_t *apps_aux;
    int status = 0;
    int num_jobs;

    if (mysql_stmt_prepare(statement, query, strlen(query))) return mysql_statement_error(statement);
    MYSQL_BIND bind[5];
    unsigned long job_id, step_id, pow_sig_id;
    long long sig_id = 0; 
    int num_apps;
    memset(bind, 0, sizeof(bind));

    //integer types
    bind[0].buffer_type = bind[1].buffer_type = bind[3].buffer_type = bind[4].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = bind[1].is_unsigned = bind[3].is_unsigned = bind[4].is_unsigned = 1;

    //string types
    bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[2].buffer_length = 256;

    //reciever variables assignation
    bind[0].buffer = &job_id;
    bind[1].buffer = &step_id;
    bind[2].buffer = &app_aux->node_id;
    bind[3].buffer = &sig_id;
    bind[4].buffer = &pow_sig_id;

    if (mysql_stmt_bind_result(statement, bind))
    {
        free(app_aux);
        return mysql_statement_error(statement);
    } 

    if (mysql_stmt_execute(statement))
    {
        free(app_aux);
        return mysql_statement_error(statement);
    } 

    if (mysql_stmt_store_result(statement))
    {
        free(app_aux);
        return mysql_statement_error(statement);
    } 

    num_apps = mysql_stmt_num_rows(statement);
    if (num_apps < 1)
    {
        mysql_stmt_close(statement);
        free(app_aux);
        return 0;
    }
    apps_aux = xcalloc(num_apps, sizeof(application_t));
    int i = 0, ret = EAR_SUCCESS;
    char job_query[128];
    char sig_query[128];
    char pow_sig_query[128];
    power_signature_t *pow_sig_aux;

    job_t *job_aux;
    signature_t *sig_aux;
    //fetching and storing of jobs    
    status = mysql_stmt_fetch(statement);

    char is_mpi = 1; //we take mpi as the default type
    if (sig_id < 1 || bind[3].is_null)
        is_mpi = 0;

    while (status == 0 || status == MYSQL_DATA_TRUNCATED)
    {
        if (is_learning)
            sprintf(job_query, "SELECT * FROM Learning_jobs WHERE id=%lu AND step_id=%lu", job_id, step_id);
        else
            sprintf(job_query, "SELECT * FROM Jobs WHERE id=%lu AND step_id=%lu", job_id, step_id);
        num_jobs = mysql_retrieve_jobs(connection, job_query, &job_aux);
        if (num_jobs < 1)
        {
            ret = EAR_ERROR;
            if (mysql_stmt_close(statement)) ret = EAR_MYSQL_ERROR;
            free(app_aux);
            free(apps_aux);
            debug("No job structure found in the database for job %lu.%lu.\n", job_id, step_id);
            return ret;
        }
        copy_job(&app_aux->job, job_aux);
        free(job_aux);

        if (is_mpi)
        {
            if (is_learning)
                sprintf(sig_query, "SELECT * FROM Learning_signatures WHERE id=%lld", sig_id);
            else
                sprintf(sig_query, "SELECT * FROM Signatures WHERE id=%lld", sig_id);
            int num_sigs = mysql_retrieve_signatures(connection, sig_query, &sig_aux);
            if (num_sigs > 0) {
                signature_copy(&app_aux->signature, sig_aux);
                free(sig_aux);
            }
            app_aux->is_mpi = 1;
        }
        else app_aux->is_mpi = 0;

        sprintf(pow_sig_query, "SELECT * FROM Power_signatures WHERE id=%lu", pow_sig_id);
        int num_pow_sigs = mysql_retrieve_power_signatures(connection, pow_sig_query, &pow_sig_aux);
        if (num_pow_sigs > 0)
        {
            copy_power_signature(&app_aux->power_sig, pow_sig_aux);
            free(pow_sig_aux);
        }

        sig_id = 0;
        copy_application(&apps_aux[i], app_aux);
        status = mysql_stmt_fetch(statement);
        i++;

        is_mpi = 1;
        if (sig_id < 1 || bind[3].is_null)
            is_mpi = 0;


    }
    *apps = apps_aux;

    free(app_aux);

    if (mysql_stmt_close(statement)) {
        free(apps_aux);
        return EAR_MYSQL_ERROR;
    }

    return num_apps;
}

int mysql_insert_loop(MYSQL *connection, loop_t *loop)
{
    return mysql_batch_insert_loops(connection, loop, 1);
}

int mysql_batch_insert_loops(MYSQL *connection, loop_t *loop, int num_loops)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    int i, j;

    char *params = ", (?, ?, ?, ?, ?, ?, ?, ?)";
    char *query = xmalloc(strlen(LOOP_MYSQL_QUERY)+strlen(params)*(num_loops-1)+1);
    strcpy(query, LOOP_MYSQL_QUERY);

    for (i = 1; i < num_loops; i++)
        strcat(query, params);

    if (autoincrement_offset < 1)
    {
        verbose(VMYSQL, "autoincrement_offset not set, reading from database...");
        if (get_autoincrement(connection, &autoincrement_offset) != EAR_SUCCESS)
        {
            verbose(VMYSQL, "error reading offset, setting to default (1)");
            autoincrement_offset = 1;
        }
        verbose(VMYSQL, "autoincrement_offset set to %ld\n", autoincrement_offset);
    }

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_loops*LOOP_ARGS, sizeof(MYSQL_BIND));

    signature_container_t cont;
    cont.type = EAR_TYPE_LOOP;
    cont.loop = loop;
    long long sig_id = mysql_batch_insert_signatures(connection, cont, 0, num_loops);

    if (sig_id < 0){
        verbose(VMYSQL,"Error inserting N=%d loops signatures\n",num_loops);
        free(query);
        free(bind);
        return EAR_ERROR;
    }

    long long *sigs_ids = xcalloc(num_loops, sizeof(long long));

    for (i = 0; i < num_loops; i++)
        sigs_ids[i] = sig_id + i*autoincrement_offset;

    for (i = 0; i < num_loops; i++)
    {

        int offset = i*LOOP_ARGS;
        //integer types
        for (j = 0; j < LOOP_ARGS; j++)
        {
            bind[j+offset].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[j+offset].is_unsigned = 1;
        }

        //string types
        bind[offset+5].buffer_type = MYSQL_TYPE_VAR_STRING;
        bind[offset+5].buffer_length = strlen(loop[i].node_id);

        //storage variable assignation
        bind[offset+0].buffer = (char *)&loop[i].id.event;
        bind[offset+1].buffer = (char *)&loop[i].id.size;
        bind[offset+2].buffer = (char *)&loop[i].id.level;
        bind[offset+3].buffer = (char *)&loop[i].jid;
        bind[offset+4].buffer = (char *)&loop[i].step_id;
        bind[offset+5].buffer = (char *)&loop[i].node_id;
        bind[offset+6].buffer = (char *)&loop[i].total_iterations;
        bind[offset+7].buffer = (char *)&sigs_ids[i];

    }

    int ret = EAR_SUCCESS;
    if (mysql_stmt_bind_param(statement, bind)) ret = mysql_statement_error(statement);

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_close(statement)) ret = EAR_MYSQL_ERROR;
    }

    free(sigs_ids);
    free(bind);
    free(query);

    return ret;
}

int mysql_retrieve_loops(MYSQL *connection, char *query, loop_t **loops)
{

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    loop_t *loop_aux = xcalloc(1, sizeof(loop_t));
    loop_t *loops_aux;
    int status = 0;
    int num_loops, i;

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(loop_aux);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND bind[8];
    long long sig_id;
    memset(bind, 0, sizeof(bind));


    //integer types
    for (i = 0; i < 8; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[i].is_unsigned = 1;
    }

    //string types
    bind[5].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[5].buffer_length = 256;

    //reciever variables assignation
    bind[0].buffer = &loop_aux->id.event;
    bind[1].buffer = &loop_aux->id.size;
    bind[2].buffer = &loop_aux->id.level;
    bind[3].buffer = &loop_aux->jid;
    bind[4].buffer = &loop_aux->step_id;
    bind[5].buffer = &loop_aux->node_id;
    bind[6].buffer = &loop_aux->total_iterations;
    bind[7].buffer = &sig_id;


    if (mysql_stmt_bind_result(statement, bind)) 
    {
        free(loop_aux);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement))
    {
        free(loop_aux);
        return mysql_statement_error(statement);
    } 

    if (mysql_stmt_store_result(statement))
    {
        free(loop_aux);
        return mysql_statement_error(statement);
    } 

    num_loops = mysql_stmt_num_rows(statement);

    if (num_loops < 1)
    {
        mysql_stmt_close(statement);
        free(loop_aux);
        return EAR_ERROR;
    }

    loops_aux = (loop_t*) xcalloc(num_loops, sizeof(loop_t));
    char sig_query[128];
    signature_t *sig_aux;
    int ret;
    //fetching and storing of jobs
    i = 0;
    status = mysql_stmt_fetch(statement);
    while (status == 0 || status == MYSQL_DATA_TRUNCATED)
    {
        sprintf(sig_query, "SELECT * FROM Signatures WHERE id=%lld", sig_id);
        ret = mysql_retrieve_signatures(connection, sig_query, &sig_aux);

        if (ret > 0 && sig_aux != NULL) { //to prevent it from crashing
            signature_copy(&loop_aux->signature, sig_aux);
            free(sig_aux);
        }

        copy_loop(&loops_aux[i], loop_aux);

        status = mysql_stmt_fetch(statement);
        i++;
    }

    *loops = loops_aux;

    free(loop_aux);
    if (mysql_stmt_close(statement)) {
        free(loops_aux);
        return EAR_MYSQL_ERROR;
    }

    return num_loops;
}


int mysql_retrieve_jobs(MYSQL *connection, char *query, job_t **jobs)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    job_t *job_aux = xcalloc(1, sizeof(job_t));
    job_t *jobs_aux;
    int status = 0;
    int num_jobs;

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(job_aux);    
        return mysql_statement_error(statement);
    }

    MYSQL_BIND bind[16];
    memset(bind, 0, sizeof(bind));
    //integer types
    bind[0].buffer_type = bind[4].buffer_type = bind[5].buffer_type = bind[12].buffer_type
        = bind[6].buffer_type = bind[7].buffer_type = bind[10].buffer_type = bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = bind[10].is_unsigned = 1;

    //string types
    bind[2].buffer_type = bind[3].buffer_type = bind[8].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[2].buffer_length = bind[3].buffer_length = bind[8].buffer_length = 256;

    //double types
    bind[9].buffer_type = MYSQL_TYPE_DOUBLE;

    //varchar types
    bind[13].buffer_type = bind[14].buffer_type = bind[15].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[13].buffer_length = bind[14].buffer_length = bind[15].buffer_length = 256;


    //reciever variables assignation
    bind[0].buffer = &job_aux->id;
    bind[1].buffer = &job_aux->step_id;
    bind[2].buffer = &job_aux->user_id;
    bind[3].buffer = &job_aux->app_id;
    bind[4].buffer = &job_aux->start_time;
    bind[5].buffer = &job_aux->end_time;
    bind[6].buffer = &job_aux->start_mpi_time;
    bind[7].buffer = &job_aux->end_mpi_time;
    bind[8].buffer = &job_aux->policy;
    bind[9].buffer = &job_aux->th;
    bind[10].buffer = &job_aux->procs;
    bind[11].buffer = &job_aux->type;
    bind[12].buffer = &job_aux->def_f;
    bind[13].buffer = &job_aux->user_acc;
    bind[14].buffer = &job_aux->group_id;
    bind[15].buffer = &job_aux->energy_tag;

    int ret = EAR_SUCCESS;
    if (mysql_stmt_bind_result(statement, bind)) ret = mysql_statement_error(statement);

    if (ret == EAR_SUCCESS) { 
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_store_result(statement)) ret = mysql_statement_error(statement);
    }

    if (ret != EAR_SUCCESS) {
        free(job_aux);
        return ret;
    }

    num_jobs = mysql_stmt_num_rows(statement);
    if (num_jobs < 1)
    {
        free(job_aux);
        mysql_stmt_close(statement);
        return EAR_ERROR;
    }

    jobs_aux = (job_t*) xcalloc(num_jobs, sizeof(job_t));
    int i = 0;

    //fetching and storing of jobs    
    status = mysql_stmt_fetch(statement);
    while (status == 0 || status == MYSQL_DATA_TRUNCATED)
    {
        copy_job(&jobs_aux[i], job_aux);
        status = mysql_stmt_fetch(statement);
        i++;
    }
    *jobs = jobs_aux;

    free(job_aux);
    if (mysql_stmt_close(statement)) {
        free(jobs_aux);
        return EAR_MYSQL_ERROR;
    }

    return num_jobs;

}

/*Inserts a new signature for a certain job, computes average if it already exists. */
int mysql_batch_insert_avg_signatures(MYSQL *connection, application_t *app, int num_sigs)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);

    if (!statement) return EAR_MYSQL_ERROR;
    char *params;
    if (full_signature)
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    else
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    char *query;
    int num_args = full_signature ? 24 : 14;

    query = xmalloc(strlen(AVG_SIGNATURE_MYSQL_QUERY)+num_sigs*strlen(params)+1+strlen(AVG_SIG_ENDING));
    strcpy(query, AVG_SIGNATURE_MYSQL_QUERY);


    int i, j;
    for (i = 1; i < num_sigs; i++)
        strcat(query, params);

    strcat(query, AVG_SIG_ENDING);

    if (mysql_stmt_prepare(statement, query, strlen(query))) return mysql_statement_error(statement);


    MYSQL_BIND *bind = xcalloc(num_sigs*num_args, sizeof(MYSQL_BIND));

    unsigned long nodes_counter = 1;

    for (i = 0; i < num_sigs; i++)
    {
        int offset = i*num_args;
        //double storage
        for (j = 0; j < 9; j++)
        {
            bind[offset+j].buffer_type = MYSQL_TYPE_DOUBLE;
            bind[offset+j].length = 0;
        }

        //unsigned long long storage
        for (j = 9; j < num_args; j++)
        {
            bind[offset+j].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[offset+j].length = 0;
            bind[offset+j].is_null = 0;
            bind[offset+j].is_unsigned = 1;
        }

        //storage variables assignation
        bind[0+offset].buffer = (char *)&app[i].signature.DC_power;
        bind[1+offset].buffer = (char *)&app[i].signature.DRAM_power;
        bind[2+offset].buffer = (char *)&app[i].signature.PCK_power;
        bind[3+offset].buffer = (char *)&app[i].signature.EDP;
        bind[4+offset].buffer = (char *)&app[i].signature.GBS;
        bind[5+offset].buffer = (char *)&app[i].signature.TPI;
        bind[6+offset].buffer = (char *)&app[i].signature.CPI;
        bind[7+offset].buffer = (char *)&app[i].signature.Gflops;
        bind[8+offset].buffer = (char *)&app[i].signature.time;
        if (full_signature)
        {
            bind[9+offset].buffer = (char *)&app[i].signature.FLOPS[0];
            bind[10+offset].buffer = (char *)&app[i].signature.FLOPS[1];
            bind[11+offset].buffer = (char *)&app[i].signature.FLOPS[2];
            bind[12+offset].buffer = (char *)&app[i].signature.FLOPS[3];
            bind[13+offset].buffer = (char *)&app[i].signature.FLOPS[4];
            bind[14+offset].buffer = (char *)&app[i].signature.FLOPS[5];
            bind[15+offset].buffer = (char *)&app[i].signature.FLOPS[6];
            bind[16+offset].buffer = (char *)&app[i].signature.FLOPS[7];
            bind[17+offset].buffer = (char *)&app[i].signature.instructions;
            bind[18+offset].buffer = (char *)&app[i].signature.cycles;
            bind[19+offset].buffer = (char *)&app[i].signature.avg_f;
            bind[20+offset].buffer = (char *)&app[i].signature.def_f;
            bind[21+offset].buffer = (char *)&app[i].job.id;
            bind[22+offset].buffer = (char *)&app[i].job.step_id;
            bind[23+offset].buffer = (char *)&nodes_counter;
        }
        else
        {
            bind[9+offset].buffer = (char *)&app[i].signature.avg_f;
            bind[10+offset].buffer = (char *)&app[i].signature.def_f;
            bind[11+offset].buffer = (char *)&app[i].job.id;
            bind[12+offset].buffer = (char *)&app[i].job.step_id;
            bind[13+offset].buffer = (char *)&nodes_counter;
        }

    }

    if (mysql_stmt_bind_param(statement, bind)) {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    free(bind);
    free(query);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return EAR_SUCCESS;
}

//returns id of the first inserted signature
long long mysql_batch_insert_signatures(MYSQL *connection, signature_container_t cont, char is_learning, int num_sigs)
{
	is_learning &= USE_LEARNING_APPS;
    MYSQL_STMT *statement = mysql_stmt_init(connection);

    if (!statement) return EAR_MYSQL_ERROR;
    char *query, *params;
    int i, j, num_params;

#if USE_GPUS
    if (full_signature)
    {
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        num_params = 29;
    }
    else
    {
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        num_params = 16;
    }
#else
    if (full_signature)
    {
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        num_params = 27;
    }
    else
    {
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        num_params = 14;
    }
#endif

    if (!is_learning)
    {
        query = xmalloc(strlen(SIGNATURE_MYSQL_QUERY)+num_sigs*strlen(params)+1);
        strcpy(query, SIGNATURE_MYSQL_QUERY);
    }
    else
    {
        query = xmalloc(strlen(LEARNING_SIGNATURE_MYSQL_QUERY)+num_sigs*strlen(params)+1);
        strcpy(query, LEARNING_SIGNATURE_MYSQL_QUERY);
    }

    for (i = 1; i < num_sigs; i++)
        strcat(query, params);

#if USE_GPUS
    long int *gpu_sig_ids = NULL, current_gpu_sig_id = 0, starter_gpu_sig_id;
    starter_gpu_sig_id = mysql_batch_insert_gpu_signatures(connection, cont, num_sigs);
    if (starter_gpu_sig_id >= 0)
    {
        gpu_signature_t *gpu_sig;
        for (i = 0; i < num_sigs; i++)
        {
            if (cont.type == EAR_TYPE_APPLICATION)
                gpu_sig = &cont.app[i].signature.gpu_sig;
            else if (cont.type == EAR_TYPE_LOOP)
                gpu_sig = &cont.loop[i].signature.gpu_sig;
            else {
                warning("container type unrecognized");
                continue;
            }

            current_gpu_sig_id += gpu_sig->num_gpus;
        }

        gpu_sig_ids = xcalloc(current_gpu_sig_id, sizeof(long int));

        if (autoincrement_offset < 1)
        {
            verbose(VMYSQL, "autoincrement_offset not set, reading from database...");
            if (get_autoincrement(connection, &autoincrement_offset) != EAR_SUCCESS)
            {
                verbose(VMYSQL, "error reading offset, setting to default (1)");
                autoincrement_offset = 1;
            }
            verbose(VMYSQL, "autoincrement_offset set to %ld\n", autoincrement_offset);
        }

        for (i = 0; i < current_gpu_sig_id; i++)
        {
            gpu_sig_ids[i] = starter_gpu_sig_id + i*autoincrement_offset;
        }
        current_gpu_sig_id = 0;
    }
#endif

    if (mysql_stmt_prepare(statement, query, strlen(query))) 
    {
#if USE_GPUS
        if (starter_gpu_sig_id >= 0) free(gpu_sig_ids); 
#endif
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_sigs*num_params, sizeof(MYSQL_BIND));

    for (i = 0; i < num_sigs; i++)
    {
        int offset = i*num_params;
        //double storage
        for (j = 0; j < 11; j++)
        {
            bind[offset+j].buffer_type = MYSQL_TYPE_DOUBLE;
            bind[offset+j].length = 0;
        }

        //unsigned long long storage
        for (j = 11; j < num_params; j++)
        {
            bind[offset+j].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[offset+j].length = 0;
            bind[offset+j].is_null = 0;
            bind[offset+j].is_unsigned = 1;
        }

        signature_t *signature = NULL;

        //storage variables assignation
        if (cont.type == EAR_TYPE_APPLICATION)
        {
            signature = &cont.app[i].signature;
        }
        else if (cont.type == EAR_TYPE_LOOP)
        {
            signature = &cont.loop[i].signature;
        } else {
            continue;
        }

        bind[0+offset].buffer = (char *)&signature->DC_power;
        bind[1+offset].buffer = (char *)&signature->DRAM_power;
        bind[2+offset].buffer = (char *)&signature->PCK_power;
        bind[3+offset].buffer = (char *)&signature->EDP;
        bind[4+offset].buffer = (char *)&signature->GBS;
        bind[5+offset].buffer = (char *)&signature->IO_MBS;
        bind[6+offset].buffer = (char *)&signature->TPI;
        bind[7+offset].buffer = (char *)&signature->CPI;
        bind[8+offset].buffer = (char *)&signature->Gflops;
        bind[9+offset].buffer = (char *)&signature->time;
        bind[10+offset].buffer = (char *)&signature->perc_MPI;
        if (full_signature)
        {
            bind[11+offset].buffer = (char *)&signature->L1_misses;
            bind[12+offset].buffer = (char *)&signature->L2_misses;
            bind[13+offset].buffer = (char *)&signature->L3_misses;
            bind[14+offset].buffer = (char *)&signature->FLOPS[0];
            bind[15+offset].buffer = (char *)&signature->FLOPS[1];
            bind[16+offset].buffer = (char *)&signature->FLOPS[2];
            bind[17+offset].buffer = (char *)&signature->FLOPS[3];
            bind[18+offset].buffer = (char *)&signature->FLOPS[4];
            bind[19+offset].buffer = (char *)&signature->FLOPS[5];
            bind[20+offset].buffer = (char *)&signature->FLOPS[6];
            bind[21+offset].buffer = (char *)&signature->FLOPS[7];
            bind[22+offset].buffer = (char *)&signature->instructions;
            bind[23+offset].buffer = (char *)&signature->cycles;
            bind[24+offset].buffer = (char *)&signature->avg_f;
            bind[25+offset].buffer = (char *)&signature->avg_imc_f;
            bind[26+offset].buffer = (char *)&signature->def_f;
#if USE_GPUS
            if (signature->gpu_sig.num_gpus > 0 && starter_gpu_sig_id >= 0) // if there are gpu signatures and no error occured
            {
                bind[27+offset].buffer = (char *)&gpu_sig_ids[current_gpu_sig_id];

                current_gpu_sig_id += signature->gpu_sig.num_gpus - 1; //we get max_sig_id
                bind[28+offset].buffer = (char *)&gpu_sig_ids[current_gpu_sig_id];

                current_gpu_sig_id++; //we prepare for the next signature_id
            }
            else // if no gpu_signatures we set the values to null
            {
                bind[27+offset].buffer_type = bind[28+offset].buffer_type = MYSQL_TYPE_NULL;
                bind[27+offset].is_null = bind[28+offset].is_null = (ear_my_bool) 1;
                bind[27+offset].buffer  = bind[28+offset].buffer  = NULL;
            }
#endif
        }
        else
        {
            bind[11+offset].buffer = (char *)&signature->avg_f;
            bind[12+offset].buffer = (char *)&signature->avg_imc_f;
            bind[13+offset].buffer = (char *)&signature->def_f;

#if USE_GPUS
            if (signature->gpu_sig.num_gpus > 0 && starter_gpu_sig_id >= 0)
            {
                bind[14+offset].buffer = (char *)&gpu_sig_ids[current_gpu_sig_id];

                current_gpu_sig_id += signature->gpu_sig.num_gpus - 1; //we get max_sig_id
                bind[15+offset].buffer = (char *)&gpu_sig_ids[current_gpu_sig_id];

                current_gpu_sig_id++; //we prepare for the next signature_id
            }
            else // if no gpu_signatures we set the values to null
            {
                bind[14+offset].buffer_type = bind[15+offset].buffer_type = MYSQL_TYPE_NULL;
                bind[14+offset].is_null = bind[15+offset].is_null = (ear_my_bool) 1;
                bind[14+offset].buffer  = bind[15+offset].buffer  = NULL;
            }
#endif
        }
    }

    if (mysql_stmt_bind_param(statement, bind)) {
        free(bind);
        free(query);
#if USE_GPUS
        if (starter_gpu_sig_id >= 0) free(gpu_sig_ids);
#endif
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(bind);
        free(query);
#if USE_GPUS
        if (starter_gpu_sig_id >= 0) free(gpu_sig_ids);
#endif
        return mysql_statement_error(statement);
    }

    long long id = mysql_stmt_insert_id(statement);

    long long affected_rows = mysql_stmt_affected_rows(statement);

    if (affected_rows != num_sigs) 
    {
        verbose(VMYSQL, "ERROR: inserting batch signature failed (affected rows does not match num_sigs).\n");
        id = EAR_ERROR;
    }

    free(bind);
    free(query);
#if USE_GPUS
    if (starter_gpu_sig_id >= 0)
        free(gpu_sig_ids);
#endif

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return id;
}

int mysql_retrieve_signatures(MYSQL *connection, char *query, signature_t **sigs)
{
    signature_t *sig_aux = xcalloc(1, sizeof(signature_t));
    signature_t *sigs_aux = NULL;
    int num_signatures, num_params;
    int i = 0;
    int status = 0;
#if USE_GPUS
    long int min_gpu_sig_id = -1, max_gpu_sig_id = -1;
    if (full_signature)
        num_params = 30;
    else 
        num_params = 17;
#else
    if (full_signature)
        num_params = 28;
    else 
        num_params = 15;
#endif

    MYSQL_BIND *bind = xcalloc(num_params, sizeof(MYSQL_BIND));

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) {
        free(sig_aux);
        return EAR_MYSQL_ERROR;
    }

    int id = 0;
    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(sig_aux);
        return mysql_statement_error(statement);
    }

    //id reciever
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer_length= 4;
    bind[0].is_null = 0;
    bind[0].is_unsigned = 1;

    //double recievers
    for (i = 1; i < 12; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[i].buffer_length = 8;
    }

    //unsigned long long recievers
    for (i = 12; i < num_params; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[i].buffer_length = 8;
        bind[i].is_unsigned = 1;
    }

    //reciever variables assignation
    bind[0].buffer = &id;
    bind[1].buffer = &sig_aux->DC_power;
    bind[2].buffer = &sig_aux->DRAM_power;
    bind[3].buffer = &sig_aux->PCK_power;
    bind[4].buffer = &sig_aux->EDP;
    bind[5].buffer = &sig_aux->GBS;
    bind[6].buffer = &sig_aux->IO_MBS;
    bind[7].buffer = &sig_aux->TPI;
    bind[8].buffer = &sig_aux->CPI;
    bind[9].buffer = &sig_aux->Gflops;
    bind[10].buffer = &sig_aux->time;
    bind[11].buffer = &sig_aux->perc_MPI;
    if (full_signature)
    {
        bind[12].buffer = &sig_aux->L1_misses;
        bind[13].buffer = &sig_aux->L2_misses;
        bind[14].buffer = &sig_aux->L3_misses;
        bind[15].buffer = &sig_aux->FLOPS[0];
        bind[16].buffer = &sig_aux->FLOPS[1];
        bind[17].buffer = &sig_aux->FLOPS[2];
        bind[18].buffer = &sig_aux->FLOPS[3];
        bind[19].buffer = &sig_aux->FLOPS[4];
        bind[20].buffer = &sig_aux->FLOPS[5];
        bind[21].buffer = &sig_aux->FLOPS[6];
        bind[22].buffer = &sig_aux->FLOPS[7];
        bind[23].buffer = &sig_aux->instructions;
        bind[24].buffer = &sig_aux->cycles;
        bind[25].buffer = &sig_aux->avg_f;
        bind[26].buffer = &sig_aux->avg_imc_f;
        bind[27].buffer = &sig_aux->def_f;
#if USE_GPUS
        bind[28].buffer = &min_gpu_sig_id;
        bind[29].buffer = &max_gpu_sig_id;
#endif
    }
    else
    {
        bind[12].buffer = &sig_aux->avg_f;
        bind[13].buffer = &sig_aux->avg_imc_f;
        bind[14].buffer = &sig_aux->def_f;
#if USE_GPUS
        bind[15].buffer = &min_gpu_sig_id;
        bind[16].buffer = &max_gpu_sig_id;
#endif
    }

    if (mysql_stmt_bind_result(statement, bind)) 
    {
        free(sig_aux);
		free(bind);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(sig_aux);
		free(bind);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_store_result(statement)) 
    {
        free(sig_aux);
		free(bind);
        return mysql_statement_error(statement);
    }

    num_signatures = mysql_stmt_num_rows(statement);

    if (num_signatures < 1)
    {
        mysql_stmt_close(statement);
        free(sig_aux);
		free(bind);
        return EAR_ERROR;
    }

    sigs_aux = xcalloc(num_signatures, sizeof(signature_t));

    i = 0;
    //fetching and storing of signatures
    status = mysql_stmt_fetch(statement);
    while (status == 0 || status == MYSQL_DATA_TRUNCATED)
    {
        signature_copy(&(sigs_aux[i]), sig_aux);

#if USE_GPUS
        if (min_gpu_sig_id > 0 && max_gpu_sig_id > 0)
        {
            char gpu_sig_query[256];
            sprintf(gpu_sig_query, "SELECT * FROM GPU_signatures WHERE id <= %ld AND id >= %ld", max_gpu_sig_id, min_gpu_sig_id);
            int num_gpu_sigs = mysql_retrieve_gpu_signatures(connection, gpu_sig_query, &sigs_aux[i].gpu_sig);
            if (num_gpu_sigs < 1)
                sigs_aux[i].gpu_sig.num_gpus = 0;
        }
        else
            sigs_aux[i].gpu_sig.num_gpus = 0;

        max_gpu_sig_id = -1;
        min_gpu_sig_id = -1;
#endif
        status = mysql_stmt_fetch(statement);
        i++;
    }
	free(bind);
    free(sig_aux);
    if (mysql_stmt_close(statement)) {
        free(sigs_aux);
        return EAR_MYSQL_ERROR;
    }

    *sigs = sigs_aux;

    return i;
}

int mysql_insert_power_signature(MYSQL *connection, power_signature_t *pow_sig)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    if (mysql_stmt_prepare(statement, POWER_SIGNATURE_MYSQL_QUERY, strlen(POWER_SIGNATURE_MYSQL_QUERY))) return mysql_statement_error(statement);

    MYSQL_BIND bind[9];
    memset(bind, 0, sizeof(bind));

    //double types
    int i;
    for (i = 0; i < 7; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
    }

    //integer types
    bind[7].buffer_type = bind[8].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[7].is_unsigned = bind[8].is_unsigned = 1;

    //storage variable assignation
    bind[0].buffer = (char *)&pow_sig->DC_power;
    bind[1].buffer = (char *)&pow_sig->DRAM_power;
    bind[2].buffer = (char *)&pow_sig->PCK_power;
    bind[3].buffer = (char *)&pow_sig->EDP;
    bind[4].buffer = (char *)&pow_sig->max_DC_power;
    bind[5].buffer = (char *)&pow_sig->min_DC_power;
    bind[6].buffer = (char *)&pow_sig->time;
    bind[7].buffer = (char *)&pow_sig->avg_f;
    bind[8].buffer = (char *)&pow_sig->def_f;

    if (mysql_stmt_bind_param(statement, bind)) return mysql_statement_error(statement);

    if (mysql_stmt_execute(statement)) return mysql_statement_error(statement);

    int id = mysql_stmt_insert_id(statement);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return id;
}

int mysql_batch_insert_power_signatures(MYSQL *connection, application_t *pow_sig, int num_sigs)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    char *params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    char *query = xmalloc(strlen(POWER_SIGNATURE_MYSQL_QUERY) + strlen(params)*(num_sigs-1) + 1);
    strcpy(query, POWER_SIGNATURE_MYSQL_QUERY);
    int i, j;
    for (i = 1; i < num_sigs; i++)
        strcat(query, params);

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_sigs*POWER_SIGNATURE_ARGS, sizeof(MYSQL_BIND));

    for (i = 0; i < num_sigs;  i++)
    {
        int offset = i*POWER_SIGNATURE_ARGS;
        //double types
        for (j = 0; j < 7; j++)
        {
            bind[j+offset].buffer_type = MYSQL_TYPE_DOUBLE;
        }

        //integer types
        bind[7+offset].buffer_type = bind[8+offset].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[7+offset].is_unsigned = bind[8+offset].is_unsigned = 1;

        //storage variable assignation
        bind[0+offset].buffer = (char *)&pow_sig[i].power_sig.DC_power;
        bind[1+offset].buffer = (char *)&pow_sig[i].power_sig.DRAM_power;
        bind[2+offset].buffer = (char *)&pow_sig[i].power_sig.PCK_power;
        bind[3+offset].buffer = (char *)&pow_sig[i].power_sig.EDP;
        bind[4+offset].buffer = (char *)&pow_sig[i].power_sig.max_DC_power;
        bind[5+offset].buffer = (char *)&pow_sig[i].power_sig.min_DC_power;
        bind[6+offset].buffer = (char *)&pow_sig[i].power_sig.time;
        bind[7+offset].buffer = (char *)&pow_sig[i].power_sig.avg_f;
        bind[8+offset].buffer = (char *)&pow_sig[i].power_sig.def_f;
    }

    if (mysql_stmt_bind_param(statement, bind)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    int id = mysql_stmt_insert_id(statement);

    free(bind);
    free(query);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;


    return id;
}

#if USE_GPUS
long long mysql_batch_insert_gpu_signatures(MYSQL *connection, signature_container_t cont, int num_sigs)
{
    int i, j, k;
    int offset = 0, num_gpu_sigs = 0;

    if (num_sigs < 1)
        return -1; //non-valid index

    gpu_signature_t *gpu_sig;

    // GPU_signature total counts might be different of the number of signatures
    for (i = 0; i < num_sigs; i++)
    {

        if (cont.type == EAR_TYPE_APPLICATION)
            gpu_sig = &cont.app[i].signature.gpu_sig;
        else if (cont.type == EAR_TYPE_LOOP)
            gpu_sig = &cont.loop[i].signature.gpu_sig;
        else {
            warning("Container type isn't an application nor a loop.");
            continue;
        }

        if (gpu_sig) {
            if (gpu_sig->num_gpus > MAX_GPUS_SUPPORTED)
                warning("current GPU signature has more GPUs than supported");

            num_gpu_sigs += ear_min(gpu_sig->num_gpus, MAX_GPUS_SUPPORTED);
        }
    }

    verbose(VMYSQL + 1, "total number of gpu_sigs: %d", num_gpu_sigs);

    if (num_gpu_sigs < 1)
        return -1; //non-valid index, zero gpu signatures to be inserted (non-gpu apps)


    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    char *params = ", (?, ?, ?, ?, ?)";
    char *query = xmalloc(strlen(GPU_SIGNATURE_MYSQL_QUERY) + strlen(params)*(num_gpu_sigs-1) + 1);
    strcpy(query, GPU_SIGNATURE_MYSQL_QUERY);
    for (i = 1; i < num_gpu_sigs; i++)
        strcat(query, params);

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_gpu_sigs*GPU_SIGNATURE_ARGS, sizeof(MYSQL_BIND));

    for (i = 0; i < num_sigs; i++)
    {

        if (cont.type == EAR_TYPE_APPLICATION)
            gpu_sig = &cont.app[i].signature.gpu_sig;
        else if (cont.type == EAR_TYPE_LOOP)
            gpu_sig = &cont.loop[i].signature.gpu_sig;
        else {
            warning("Container type isn't an application nor a loop.");
            continue;
        }

        for (j = 0; j < gpu_sig->num_gpus; j++)
        {
            for (k = 1; k < GPU_SIGNATURE_ARGS; k++)
            {
                bind[k+offset].buffer_type = MYSQL_TYPE_LONGLONG;
                bind[k+offset].is_unsigned = 1;
            }

            //double types
            bind[0+offset].buffer_type = MYSQL_TYPE_DOUBLE;

            //storage variable assignation
            bind[0+offset].buffer = (char *)&gpu_sig->gpu_data[j].GPU_power;
            bind[1+offset].buffer = (char *)&gpu_sig->gpu_data[j].GPU_freq;
            bind[2+offset].buffer = (char *)&gpu_sig->gpu_data[j].GPU_mem_freq;
            bind[3+offset].buffer = (char *)&gpu_sig->gpu_data[j].GPU_util;
            bind[4+offset].buffer = (char *)&gpu_sig->gpu_data[j].GPU_mem_util;

            offset += GPU_SIGNATURE_ARGS;
        }
    }

    if (offset/GPU_SIGNATURE_ARGS < num_gpu_sigs) { verbose(VMYSQL, "Assigned less signatures than allocated"); }
    else if (offset/GPU_SIGNATURE_ARGS > num_gpu_sigs) { verbose(VMYSQL, "Assigned more signatures than allocated"); }

    if (mysql_stmt_bind_param(statement, bind)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    long long id = mysql_stmt_insert_id(statement);

    long long affected_rows = mysql_stmt_affected_rows(statement);

    if (affected_rows != num_gpu_sigs) 
    {
        verbose(VMYSQL, "ERROR: inserting batch gpu signature failed (affected rows does not match num_sigs).\n");
        id = EAR_ERROR;
    }

    free(bind);
    free(query);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return id;
}

int mysql_retrieve_gpu_signatures(MYSQL *connection, char *query, gpu_signature_t *gpu_sig)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    int status = 0;
    int num_gpu_sigs;

    unsigned long id;

    if (mysql_stmt_prepare(statement, query, strlen(query))) return mysql_statement_error(statement);

    MYSQL_BIND bind[6];
    gpu_app_t base_data;
    memset(bind, 0, sizeof(bind));
    memset(&base_data, 0, sizeof(gpu_app_t));

    //double types
    int i;
    for (i = 0; i < 6; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[i].is_unsigned = 1;
    }

    //integer types
    bind[1].buffer_type = MYSQL_TYPE_DOUBLE;

    //storage variable assignation
    bind[0].buffer = &id;
    bind[1].buffer = &base_data.GPU_power;
    bind[2].buffer = &base_data.GPU_freq;
    bind[3].buffer = &base_data.GPU_mem_freq;
    bind[4].buffer = &base_data.GPU_util;
    bind[5].buffer = &base_data.GPU_mem_util;

    if (mysql_stmt_bind_result(statement, bind)) return mysql_statement_error(statement);

    if (mysql_stmt_execute(statement)) return mysql_statement_error(statement);

    if (mysql_stmt_store_result(statement)) return mysql_statement_error(statement);

    num_gpu_sigs = mysql_stmt_num_rows(statement);
    if (num_gpu_sigs < 1 || num_gpu_sigs > MAX_GPUS_SUPPORTED)
    {
        mysql_stmt_close(statement);
        return EAR_ERROR;
    }

    i = 0;
    gpu_sig->num_gpus = num_gpu_sigs;


    //fetching and storing of power_signatures
    status = mysql_stmt_fetch(statement);
    while (status == 0 || status == MYSQL_DATA_TRUNCATED)
    {
        memcpy(&gpu_sig->gpu_data[i], &base_data, sizeof(gpu_app_t));
        status = mysql_stmt_fetch(statement);
        i++;
    }

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return num_gpu_sigs;

}
#endif

int mysql_retrieve_power_signatures(MYSQL *connection, char *query, power_signature_t **pow_sigs)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;
    power_signature_t *pow_sig_aux = xcalloc(1, sizeof(power_signature_t));
    power_signature_t *pow_sigs_aux;
    int status = 0;
    int num_pow_sigs;
    unsigned long id;

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(pow_sig_aux);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND bind[10];
    memset(bind, 0, sizeof(bind));

    //double types
    int i;
    for (i = 1; i < 8; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
    }

    //integer types
    bind[0].buffer_type = bind[8].buffer_type = bind[9].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = bind[8].is_unsigned = bind[9].is_unsigned = 1;

    //storage variable assignation
    bind[0].buffer = &id;
    bind[1].buffer = &pow_sig_aux->DC_power;
    bind[2].buffer = &pow_sig_aux->DRAM_power;
    bind[3].buffer = &pow_sig_aux->PCK_power;
    bind[4].buffer = &pow_sig_aux->EDP;
    bind[5].buffer = &pow_sig_aux->max_DC_power;
    bind[6].buffer = &pow_sig_aux->min_DC_power;
    bind[7].buffer = &pow_sig_aux->time;
    bind[8].buffer = &pow_sig_aux->avg_f;
    bind[9].buffer = &pow_sig_aux->def_f;

    int ret = EAR_SUCCESS;
    if (mysql_stmt_bind_result(statement, bind)) ret = mysql_statement_error(statement);

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_store_result(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        num_pow_sigs = mysql_stmt_num_rows(statement);
        if (num_pow_sigs < 1)
        {
            mysql_stmt_close(statement);
            free(pow_sig_aux);
            return EAR_ERROR;
        }
    } else {
        free(pow_sig_aux);
        return ret;
    }

    pow_sigs_aux = (power_signature_t *) xcalloc(num_pow_sigs, sizeof(power_signature_t));
    i = 0;

    //fetching and storing of power_signatures
    status = mysql_stmt_fetch(statement);
    while (status == 0 || status == MYSQL_DATA_TRUNCATED)
    {
        copy_power_signature(&pow_sigs_aux[i], pow_sig_aux);
        status = mysql_stmt_fetch(statement);
        i++;
    }


    free(pow_sig_aux);
    if (mysql_stmt_close(statement)) {
        free(pow_sigs_aux);
        return EAR_MYSQL_ERROR;
    }

    *pow_sigs = pow_sigs_aux;

    return num_pow_sigs;

}

int mysql_insert_periodic_aggregation(MYSQL *connection, periodic_aggregation_t *per_agg)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    if (mysql_stmt_prepare(statement, PERIODIC_AGGREGATION_MYSQL_QUERY, strlen(PERIODIC_AGGREGATION_MYSQL_QUERY))) return mysql_statement_error(statement);

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    //integer types
    int i;
    for (i = 0; i < 3; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
    }

    //varchar types
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer_length = strlen(per_agg->eardbd_host);

    //storage variable assignation
    bind[0].buffer = (char *)&per_agg->DC_energy;
    bind[1].buffer = (char *)&per_agg->start_time;
    bind[2].buffer = (char *)&per_agg->end_time;
    bind[3].buffer = (char *)&per_agg->eardbd_host;

    if (mysql_stmt_bind_param(statement, bind)) return mysql_statement_error(statement);

    if (mysql_stmt_execute(statement)) return mysql_statement_error(statement);

    int id = mysql_stmt_insert_id(statement);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return id;

}

int mysql_insert_periodic_metric(MYSQL *connection, periodic_metric_t *per_met) 
{
    return mysql_batch_insert_periodic_metrics(connection, per_met, 1);
}

int mysql_batch_insert_periodic_metrics(MYSQL *connection, periodic_metric_t *per_mets, int num_mets)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    char *params;
#if USE_GPUS
    int num_args = node_detail ? 11 : 6;
#else
    int num_args = node_detail ? 10 : 6;
#endif

    if (node_detail)
#if USE_GPUS
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
#else
        params = ", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
#endif
    else
        params = ", (?, ?, ?, ?, ?, ?)";

    char *query = xmalloc(strlen(PERIODIC_METRIC_MYSQL_QUERY)+(num_mets-1)*strlen(params)+1);
    strcpy(query, PERIODIC_METRIC_MYSQL_QUERY);

    int i, j;
    for (i = 1; i < num_mets; i++)
    {
        strcat(query, params);
    }


    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement); 
    }


    MYSQL_BIND *bind = xcalloc(num_mets*num_args, sizeof(MYSQL_BIND));

    for (i = 0; i < num_mets; i++)
    {

        int offset = i*num_args;
        for (j = 0; j < num_args; j++)
        {
            bind[offset+j].buffer_type = MYSQL_TYPE_LONG;
            bind[offset+j].is_unsigned = 1;
        }
        bind[offset+3].buffer_type = MYSQL_TYPE_STRING;
        bind[offset+3].buffer_length = strlen(per_mets[i].node_id);

        bind[0+offset].buffer = (char *)&per_mets[i].start_time;
        bind[1+offset].buffer = (char *)&per_mets[i].end_time;
        bind[2+offset].buffer = (char *)&per_mets[i].DC_energy;
        bind[3+offset].buffer = (char *)&per_mets[i].node_id;
        bind[4+offset].buffer = (char *)&per_mets[i].job_id;
        bind[5+offset].buffer = (char *)&per_mets[i].step_id;
        if (node_detail)
        {
            bind[8 + offset].buffer_type = bind[9 + offset].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[8 + offset].is_unsigned = bind[9 + offset].is_unsigned = 1;

            bind[6+offset].buffer = (char *)&per_mets[i].avg_f;
            bind[7+offset].buffer = (char *)&per_mets[i].temp;   
            bind[8+offset].buffer = (char *)&per_mets[i].DRAM_energy;
            bind[9+offset].buffer = (char *)&per_mets[i].PCK_energy;
#if USE_GPUS
            bind[10 + offset].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[10 + offset].is_unsigned = 1;
            bind[10+offset].buffer = (char *)&per_mets[i].GPU_energy;
#endif
        }
    }

    if (mysql_stmt_bind_param(statement, bind)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(bind);
        free(query);
        return mysql_statement_error(statement);
    }

    mysql_stmt_close(statement);
    free(bind);
    free(query);
    return EAR_SUCCESS;
}

int mysql_batch_insert_periodic_aggregations(MYSQL *connection, periodic_aggregation_t *per_aggs, int num_aggs)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    char *params = ", (?, ?, ?, ?)";
    char *query = xmalloc(strlen(PERIODIC_AGGREGATION_MYSQL_QUERY)+(num_aggs-1)*strlen(params)+1);
    strcpy(query, PERIODIC_AGGREGATION_MYSQL_QUERY);

    int i, j;
    for (i = 1; i < num_aggs; i++)
    {
        strcat(query, params);
    }


    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_aggs*PERIODIC_AGGREGATION_ARGS, sizeof(MYSQL_BIND));

    //integer types
    for (i = 0; i < num_aggs; i++)
    {
        int offset = i*PERIODIC_AGGREGATION_ARGS;
        for (j = 0; j < PERIODIC_AGGREGATION_ARGS; j++)
        {
            bind[j+offset].buffer_type = MYSQL_TYPE_LONGLONG;
        }
        //varchar types
        bind[PERIODIC_AGGREGATION_ARGS - 1 + offset].buffer_type = MYSQL_TYPE_STRING;
        bind[PERIODIC_AGGREGATION_ARGS - 1 + offset].buffer_length = strlen(per_aggs[i].eardbd_host);

        //storage variable assignation
        bind[0+offset].buffer = (char *)&per_aggs[i].DC_energy;
        bind[1+offset].buffer = (char *)&per_aggs[i].start_time;
        bind[2+offset].buffer = (char *)&per_aggs[i].end_time;
        bind[3+offset].buffer = (char *)&per_aggs[i].eardbd_host;
    }

    if (mysql_stmt_bind_param(statement, bind))
    {
        free(query);
        free(bind);
        return mysql_statement_error(statement);
    }

    if (mysql_stmt_execute(statement)) 
    {
        free(query);
        free(bind);
        return mysql_statement_error(statement);
    }

    free(query);
    free(bind);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return EAR_SUCCESS;
}

int mysql_insert_ear_event(MYSQL *connection, ear_event_t *ear_ev)
{
    return mysql_batch_insert_ear_events(connection, ear_ev, 1);
}

int mysql_batch_insert_ear_events(MYSQL *connection, ear_event_t *ear_ev, int num_evs)
{
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    char *params = ", (?, ?, ?, ?, ?, ?)";
    char *query = xmalloc(strlen(EAR_EVENT_MYSQL_QUERY)+(num_evs)*strlen(params)+1);
    strcpy(query, EAR_EVENT_MYSQL_QUERY);

    int i, j, offset;
    for (i = 1; i < num_evs; i++) {
        strcat(query, params);
    }

    //Prevent freq from going over max value
    for (i = 0; i < num_evs; i++) {
        ear_ev[i].value = ear_ev[i].value > INT_MAX ? INT_MAX : ear_ev[i].value;
    }

    if (mysql_stmt_prepare(statement, query, strlen(query))) {
        free(query);
        return mysql_statement_error(statement);
    }

    MYSQL_BIND *bind = xcalloc(num_evs*EAR_EVENTS_ARGS, sizeof(MYSQL_BIND));

    for (i = 0; i < num_evs; i++)
    {
        offset = i*EAR_EVENTS_ARGS;

        for (j = 0; j < EAR_EVENTS_ARGS; j++)
        {
            bind[offset+j].buffer_type = MYSQL_TYPE_LONGLONG;
        }
        bind[offset+1].buffer_type = MYSQL_TYPE_LONG;
        bind[offset+5].buffer_type = MYSQL_TYPE_STRING;
        bind[offset+5].buffer_length = strlen(ear_ev[i].node_id);

        //storage variable assignation
        bind[0+offset].buffer = (char *)&ear_ev[i].timestamp;
        bind[1+offset].buffer = (char *)&ear_ev[i].event;
        bind[2+offset].buffer = (char *)&ear_ev[i].jid;
        bind[3+offset].buffer = (char *)&ear_ev[i].step_id;
        bind[4+offset].buffer = (char *)&ear_ev[i].value;
        bind[5+offset].buffer = (char *)&ear_ev[i].node_id;
    }

    int ret = EAR_SUCCESS;

    if (mysql_stmt_bind_param(statement, bind)) {
        ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        if (mysql_stmt_execute(statement)) ret = mysql_statement_error(statement);
    }

    if (ret == EAR_SUCCESS) {
        mysql_stmt_close(statement);
    }
    free(bind);
    free(query);

    return ret;

}

int mysql_insert_gm_warning(MYSQL *connection, gm_warning_t *warning)
{
    int i;
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) return EAR_MYSQL_ERROR;

    if (mysql_stmt_prepare(statement, EAR_WARNING_MYSQL_QUERY, strlen(EAR_WARNING_MYSQL_QUERY))) return mysql_statement_error(statement);

    MYSQL_BIND bind[10];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[1].buffer_type = bind[3].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].is_unsigned = 1;

    for (i = 4; i < 9; i++)
    {
        bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[i].is_unsigned = 1;
    } 
    bind[9].buffer_type = MYSQL_TYPE_STRING;
    bind[9].buffer_length = strlen(warning->policy);

    bind[0].buffer = (char *)&warning->energy_percent;
    bind[1].buffer = (char *)&warning->level;
    bind[2].buffer = (char *)&warning->inc_th;
    bind[3].buffer = (char *)&warning->new_p_state;
    bind[4].buffer = (char *)&warning->energy_t1;
    bind[5].buffer = (char *)&warning->energy_t2;
    bind[6].buffer = (char *)&warning->energy_limit;
    bind[7].buffer = (char *)&warning->energy_p1;
    bind[8].buffer = (char *)&warning->energy_p2;
    bind[9].buffer = (char *)&warning->policy;


    if (mysql_stmt_bind_param(statement, bind)) return mysql_statement_error(statement);

    if (mysql_stmt_execute(statement)) return mysql_statement_error(statement);

    if (mysql_stmt_close(statement)) return EAR_MYSQL_ERROR;

    return EAR_SUCCESS;

}


int mysql_run_query_string_results(MYSQL *connection, char *query, char ****results, int *num_columns, int *num_r)
{
    int i, j;
    int num_rows, num_cols;
    char ***tmp_res;
    MYSQL_RES *result;
    MYSQL_ROW row;

    if (mysql_query(connection, query)) {
        error("Error executing query");
        return EAR_ERROR;
    }

    result = mysql_store_result(connection);
	if (result == NULL) {
        error("Error storing result. Check the query types.");
        return EAR_ERROR;
    }

    num_rows = mysql_num_rows(result);
    num_cols = mysql_num_fields(result);
    if (num_rows < 1 || num_cols < 1) {
        error("No rows or columns were returned");
        return EAR_ERROR;
    }

    tmp_res = calloc(num_rows, sizeof(char **));
    for (i = 0; i < num_rows; i++)
    {
        row = mysql_fetch_row(result);
        if (row == NULL) { //if no more rows exist we free the remaining memory and return early
            num_rows = i;
            tmp_res = realloc(tmp_res, num_rows*sizeof(char **));
            break;
        }

        tmp_res[i] = calloc(num_cols, sizeof(char *));

        for (j = 0; j < num_cols; j++)
        {
            if (row[j] == NULL)
            {
                tmp_res[i][j] = NULL;
                continue;
            }
            tmp_res[i][j] = calloc(strlen(row[j]) + 1, sizeof(char));
            strcpy(tmp_res[i][j], row[j]);
        }

    }


    mysql_free_result(result);

    *results = tmp_res;
    *num_columns = num_cols;
	*num_r = num_rows;
    return EAR_SUCCESS;

}


#endif
