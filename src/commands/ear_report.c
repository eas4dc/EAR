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

#define _XOPEN_SOURCE 700 //to get rid of the warning
#define AGGREGATED 1

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <common/states.h>
#include <common/config.h>
#include <common/system/user.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/types/version.h>
#include <common/types/log_eard.h>
#include <common/types/configuration/cluster_conf.h>

#if DB_MYSQL
#include <mysql/mysql.h>
#endif

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* MYSQL QUERIES */
#if DB_MYSQL
#define MET_TIME    "SELECT MIN(start_time), MAX(end_time) FROM Periodic_metrics WHERE start_time" \
                    ">= ? AND end_time <= ?"

#define AGGR_TIME   "SELECT MIN(start_time), MAX(end_time) FROM Periodic_aggregations WHERE start_time" \
                    ">= ? AND end_time <= ?"

#define AGGR_QUERY  "SELECT SUM(dc_energy)/? FROM Periodic_aggregations WHERE start_time" \
                    ">= ? AND end_time <= ?"

#define USER_QUERY  "SELECT SUM(DC_power*time)/? FROM Power_signatures WHERE id IN " \
                    "(SELECT Applications.power_signature_id FROM Applications JOIN Jobs " \
                    "ON job_id = id AND Applications.step_id = Jobs.step_id WHERE "\
                    "Jobs.user_id = '%s' AND start_time >= ? AND end_time <= ? AND DC_power < %d)"

#define ETAG_QUERY  "SELECT SUM(DC_power*time)/? FROM Power_signatures WHERE id IN " \
                    "(SELECT Applications.power_signature_id FROM Applications JOIN Jobs " \
                    "ON job_id = id AND Applications.step_id = Jobs.step_id WHERE "\
                    "Jobs.e_tag = '%s' AND start_time >= ? AND end_time <= ? AND DC_power < %d)"

#define SUM_QUERY   "SELECT SUM(dc_energy)/? FROM Periodic_metrics WHERE start_time" \
                    ">= ? AND end_time <= ?"

/* POSTGRESQL QUERIES */
#elif DB_PSQL
#define MET_TIME    "SELECT MIN(start_time), MAX(end_time) FROM Periodic_metrics WHERE start_time" \
                    ">= %d AND end_time <= %d"

#define AGGR_TIME   "SELECT MIN(start_time), MAX(end_time) FROM Periodic_aggregations WHERE start_time" \
                    ">= %d AND end_time <= %d"

#define AGGR_QUERY  "SELECT SUM(dc_energy)/%llu FROM Periodic_aggregations WHERE start_time" \
                    ">= %d AND end_time <= %d"

#define USER_QUERY  "SELECT SUM(DC_power*time)/%llu FROM Power_signatures WHERE id IN " \
                    "(SELECT Applications.power_signature_id FROM Applications JOIN Jobs " \
                    "ON job_id = id AND Applications.step_id = Jobs.step_id WHERE "\
                    "Jobs.user_id = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d)"

#define ETAG_QUERY  "SELECT SUM(DC_power*time)/%llu FROM Power_signatures WHERE id IN " \
                    "(SELECT Applications.power_signature_id FROM Applications JOIN Jobs " \
                    "ON job_id = id AND Applications.step_id = Jobs.step_id WHERE "\
                    "Jobs.e_tag = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d)"

#define SUM_QUERY   "SELECT SUM(dc_energy)/%llu FROM Periodic_metrics WHERE start_time" \
                    ">= %d AND end_time <= %d"

#endif

/* COMMON QUERIES */
#define ALL_USERS   "SELECT TRUNCATE(SUM(DC_power*time), 0) as energy, Jobs.user_id FROM " \
                    "Power_signatures INNER JOIN Applications On id=Applications.power_signature_id " \
                    "INNER JOIN Jobs ON job_id = Jobs.id AND Applications.step_id = Jobs.step_id " \
                    "WHERE start_time >= %d AND end_time <= %d AND DC_power < %d GROUP BY Jobs.user_id ORDER BY energy"

#if USE_GPUS
#define ALL_NODES   "select SUM(DC_energy), MIN(start_time), MAX(end_time), SUM(GPU_energy), node_id FROM Periodic_metrics WHERE start_time >= %d " \
                    " AND end_time <= %d GROUP BY node_id "
#else
#define ALL_NODES   "select SUM(DC_energy), MIN(start_time), MAX(end_time), node_id FROM Periodic_metrics WHERE start_time >= %d " \
                    " AND end_time <= %d GROUP BY node_id "
#endif

#define ALL_ISLANDS "SELECT SUM(DC_energy), eardbd_host FROM Periodic_aggregations WHERE start_time >= %d "\
                    " AND end_time <= %d GROUP BY eardbd_host"

#define ALL_TAGS    "SELECT TRUNCATE(SUM(DC_power*time), 0) as energy, Jobs.e_tag FROM " \
                    "Power_signatures INNER JOIN Applications ON id=Applications.power_signature_id " \
                    "INNER JOIN Jobs ON job_id = Jobs.id AND Applications.step_id = Jobs.step_id " \
                    "WHERE start_time >= %d AND end_time <= %d AND DC_power < %d GROUP BY Jobs.e_tag ORDER BY energy"

#if EXP_EARGM
#define GLOB_ENERGY "SELECT ROUND(energy_percent, 2),warning_level,time,inc_th,p_state,GlobEnergyConsumedT1, " \
                    "GlobEnergyConsumedT2,GlobEnergyLimit,GlobEnergyPeriodT1,GlobEnergyPeriodT2,GlobEnergyPolicy " \
                    "FROM Global_energy2 WHERE UNIX_TIMESTAMP(time) >= (UNIX_TIMESTAMP(NOW())-%d) ORDER BY time desc "
#else
#define GLOB_ENERGY "SELECT ROUND(energy_percent, 2),warning_level,time,inc_th,p_state,GlobEnergyConsumedT1, " \
                    "GlobEnergyConsumedT2,GlobEnergyLimit,GlobEnergyPeriodT1,GlobEnergyPeriodT2,GlobEnergyPolicy " \
                    "FROM Global_energy WHERE UNIX_TIMESTAMP(time) >= (UNIX_TIMESTAMP(NOW())-%d) ORDER BY time desc "
#endif


char *node_name = NULL;
char *user_name = NULL;
char *etag = NULL;
char *eardbd_host = NULL;
int verbose = 0;
int query_filters = 0;
unsigned long long avg_pow = 0;
time_t global_start_time = 0;
time_t global_end_time = 0;
cluster_conf_t my_conf;

void usage(char *app)
{
    printf( "%s is a tool that reports energy consumption data\n", app);
	printf( "Usage: %s [options]\n", app);
    printf( "Options are as follows:\n"\
        "\t-s start_time     \t indicates the start of the period from which the energy consumed will be computed. Format: YYYY-MM-DD. Default: end_time minus insertion time*2.\n"
        "\t-e end_time       \t indicates the end of the period from which the energy consumed will be computed. Format: YYYY-MM-DD. Default: current time.\n"
        "\t-n node_name |all \t indicates from which node the energy will be computed. Default: none (all nodes computed) \n\t\t\t\t\t 'all' option shows all users individually, not aggregated.\n"
        "\t-u user_name |all \t requests the energy consumed by a user in the selected period of time. Default: none (all users computed). \n\t\t\t\t\t 'all' option shows all users individually, not aggregated.\n"
        "\t-t energy_tag|all \t requests the energy consumed by energy tag in the selected period of time. Default: none (all tags computed). \n\t\t\t\t\t 'all' option shows all tags individually, not aggregated.\n"
        "\t-i eardbd_name|all\t indicates from which eardbd (island) the energy will be computed. Default: none (all islands computed) \n\t\t\t\t\t 'all' option shows all eardbds individually, not aggregated.\n"
        "\t-g                \t shows the contents of EAR's database Global_energy table. The default option will show the records for the two previous T2 periods of EARGM.\n\t\t\t\t\t This option can only be modified with -s, not -e\n"
        "\t-x                \t shows the daemon events from -s to -e. If no time frame is specified, it shows the last 20 events. \n"
        "\t-v                \t shows current EAR version. \n"
        "\t-h                \t shows this message.\n");
	exit(0);
}

void add_string_filter(char *query, char *addition, char *value)
{
    if (query_filters < 1)
        strcat(query, " WHERE ");
    else
        strcat(query, " AND ");

    strcat(query, addition);
    strcat(query, "=");
    strcat(query, "'");
    strcat(query, value);
    strcat(query, "'");
//    sprintf(query, query, value);
    query_filters ++;
}

void add_int_filter(char *query, char *addition, int value)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");
    
    strcat(query_tmp, addition);
    strcat(query_tmp, "=");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters ++;
}

void add_int_comp_filter(char *query, char *addition, int value, char greater_than)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");
    
    strcat(query_tmp, addition);
    if (greater_than)
        strcat(query_tmp, ">");
    else
        strcat(query_tmp, "<");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters ++;
}

void add_int_list_filter(char *query, char *addition, char *value)
{
    if (query_filters < 1)
        strcat(query, " WHERE ");
    else
        strcat(query, " AND ");

    strcat(query, addition);
    strcat(query, " IN ");
    strcat(query, "(");
    strcat(query, value);
    strcat(query, ")");
//    sprintf(query, query, value);
    query_filters ++;
}


#if DB_MYSQL
long long ereport_stmt_error(MYSQL_STMT *statement)
{
    printf( "Error preparing statement (%d): %s\n",
            mysql_stmt_errno(statement), mysql_stmt_error(statement));
    mysql_stmt_close(statement);
    return -1;
}
#endif

#if DB_MYSQL
long long get_sum(MYSQL *connection, int start_time, int end_time, unsigned long long divisor)
{

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement)
    {
        printf( "Error creating statement (%d): %s\n", mysql_errno(connection),
                mysql_error(connection)); //error
        return -1;
    }

    uint max_power = MAX_ERROR_POWER;
    int def_id = get_default_tag_id(&my_conf);
    char is_node = 0;
    if (node_name != NULL) 
    {
        my_node_conf_t *n_c = get_my_node_conf(&my_conf, node_name);
        if (n_c != NULL) {
            max_power = n_c->max_error_power;
            is_node = 1;
        }
        
    } 
    if (!is_node)
    {
        if (def_id > -1)
        {
            max_power = my_conf.tags[def_id].error_power;
        }   
        int i;
        for (i = 0; i < my_conf.num_tags; i++)
            max_power = ear_max(my_conf.tags[i].error_power, max_power);
    }
    
    char query[512];

    if (node_name != NULL)
    {
        strcpy(query, SUM_QUERY);
        strcat(query, " AND node_id='");
        strcat(query, node_name);
        strcat(query, "'");
    }
    else if (user_name != NULL)
    {
        sprintf(query, USER_QUERY, user_name, max_power);
    }
    else if (etag != NULL)
    {
        sprintf(query, ETAG_QUERY, etag, max_power);
    }
    else if (eardbd_host != NULL)
    {
        strcpy(query, AGGR_QUERY);
        strcat(query, " AND eardbd_host='");
        strcat(query, eardbd_host);
        strcat(query, "'");
    }
#if AGGREGATED
    else strcpy(query, AGGR_QUERY);
#else
    else strcpy(query, SUM_QUERY);
#endif

    if (verbose) {
        printf( "QUERY: %s\n", query);
    }
    if (mysql_stmt_prepare(statement, query, strlen(query)))
                                                return ereport_stmt_error(statement);


    //Query parameters binding
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = 1;

    bind[1].buffer_type = bind[2].buffer_type = MYSQL_TYPE_LONG;

    bind[0].buffer = (char *)&divisor;
    bind[1].buffer = (char *)&start_time;
    bind[2].buffer = (char *)&end_time;


    //Result parameters
    long long result = 0;
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    res_bind[0].is_unsigned = 1;
    res_bind[0].buffer = &result;

    if (mysql_stmt_bind_param(statement, bind)) return ereport_stmt_error(statement);
    if (mysql_stmt_bind_result(statement, res_bind)) return ereport_stmt_error(statement);
    if (mysql_stmt_execute(statement)) return ereport_stmt_error(statement);
    if (mysql_stmt_store_result(statement)) return ereport_stmt_error(statement);

    int status = mysql_stmt_fetch(statement);
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
        result = -2;

    if (mysql_stmt_free_result(statement)) {
        printf( "ERROR when freing result.\n"); //error
    }

    if (mysql_stmt_close(statement)) {
        printf( "ERROR when freeing statement\n"); //error
    }

    return result;
}
#elif DB_PSQL
long long get_sum(PGconn *connection, int start_time, int end_time, unsigned long long divisor)
{

    long long result = 0;
    char query[512];

    uint max_power = MAX_ERROR_POWER;
    int def_id = get_default_tag_id(&my_conf);
    char is_node = 0;
    if (node_name != NULL) 
    {
        my_node_conf_t *n_c = get_my_node_conf(&my_conf, node_name);
        if (n_c != NULL) {
            max_power = n_c->max_error_power;
            is_node = 1;
        }
        
    } 
    if (!is_node)
    {
        if (def_id > -1)
        {
            max_power = my_conf.tags[def_id].error_power;
        }   
        int i;
        for (i = 0; i < my_conf.num_tags; i++)
            max_power = ear_max(my_conf.tags[i].error_power, max_power);
    }
    
    if (node_name != NULL)
    {
        sprintf(query, SUM_QUERY, divisor, start_time, end_time);
        strcat(query, " AND node_id='");
        strcat(query, node_name);
        strcat(query, "'");
    }
    else if (user_name != NULL)
    {
        sprintf(query, USER_QUERY, divisor, user_name, start_time, end_time, max_power);
    }
    else if (etag != NULL)
    {
        sprintf(query, ETAG_QUERY, divisor, etag, start_time, end_time, max_power);
    }
    else if (eardbd_host != NULL)
    {
        sprintf(query, AGGR_QUERY, divisor, start_time, end_time);
        strcat(query, " AND eardbd_host='");
        strcat(query, eardbd_host);
        strcat(query, "'");
    }
#if AGGREGATED
    else sprintf(query, AGGR_QUERY, divisor, start_time, end_time);
#else
    else sprintf(query, SUM_QUERY, divisor, start_time, end_time);
#endif

    if (verbose) {
        printf( "QUERY: %s\n", query);
    }

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQgetisnull(res, 0, 0))
    {
        PQclear(res);
        return 0;
    }
        
    result = (htonl(*(long long*)(PQgetvalue(res, 0, 0))));

    PQclear(res);

    return result;
}
#endif

#if DB_MYSQL
void compute_pow(MYSQL *connection, int start_time, int end_time, unsigned long long result)
{
    char query[512];
    if (user_name == NULL && etag == NULL)
    {
        MYSQL_STMT *statement = mysql_stmt_init(connection);
        if (!statement)
        {
            printf( "Error creating statement (%d): %s\n", mysql_errno(connection),
                    mysql_error(connection));
            return;
        }

        if (node_name != NULL && strcmp(node_name, "all"))
        {
            strcpy(query, MET_TIME);
            strcat(query, " AND node_id='");
            strcat(query, node_name);
            strcat(query, "'");
        }
        else if (eardbd_host != NULL)
        {
            if (strcmp(eardbd_host, "all"))
            {
                strcpy(query, AGGR_TIME);
                strcat(query, " AND eardbd_host='");
                strcat(query, eardbd_host);
                strcat(query, "'");
            }
            else strcpy(query, AGGR_TIME);
        }
        else
#if AGGREGATED
            strcpy(query, AGGR_TIME);
#else
            strcpy(query, MET_TIME);
#endif

        if (verbose) {
            printf( "QUERY: %s\n", query);
        }

        if (mysql_stmt_prepare(statement, query, strlen(query)))
        {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        //time_t start, end; 
        MYSQL_BIND sec_bind[2];
        memset(sec_bind, 0, sizeof(sec_bind));
        sec_bind[0].buffer_type = sec_bind[1].buffer_type = MYSQL_TYPE_LONG;
        sec_bind[0].buffer = (char *)&start_time;
        sec_bind[1].buffer = (char *)&end_time;

        MYSQL_BIND sec_res[2];
        memset(sec_res, 0, sizeof(sec_res));
        sec_res[0].buffer_type = sec_res[1].buffer_type = MYSQL_TYPE_LONGLONG;
        sec_res[0].buffer = &global_start_time;
        sec_res[1].buffer = &global_end_time;
        
        if (mysql_stmt_bind_param(statement, sec_bind))
        {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        if (mysql_stmt_bind_result(statement, sec_res))
        {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        if (mysql_stmt_execute(statement))
        {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        if (mysql_stmt_store_result(statement))
        {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        int status = mysql_stmt_fetch(statement);
        if (status != 0 && status != MYSQL_DATA_TRUNCATED)
            avg_pow = -2;

        char sbuff[64], ebuff[64];
        if (verbose)
        {
            printf( "original  \t start_time: %d\t end_time: %d\n\n", start_time, end_time);
            printf( "from query\t start_time: %ld\t end_time: %ld\n\n", global_start_time, global_end_time);
            printf( "from query\t start_time: %s\t end_time: %s\n\n",
                    ctime_r(&global_start_time, sbuff), ctime_r(&global_end_time, ebuff));
            printf( "result: %llu\n", result);
        }
        if (global_start_time != global_end_time && result > 0)
            avg_pow = result / (global_end_time-global_start_time);
        else if (start_time > 0)
        {
            global_start_time = start_time;
            global_end_time = end_time;
        }
    
        if (verbose)
        {
            printf( "avg_pow after computation: %llu\n", avg_pow);
            printf( "end-start: %ld\n", global_end_time-global_start_time);
        }

        mysql_stmt_close(statement);
    }

}
#elif DB_PSQL
void compute_pow(PGconn *connection, int start_time, int end_time, unsigned long long result)
{
    char query[512], final_query[512];
    if (user_name == NULL && etag == NULL)
    {
        if (node_name != NULL && strcmp(node_name, "all"))
        {
            strcpy(query, MET_TIME);
            strcat(query, " AND node_id='");
            strcat(query, node_name);
            strcat(query, "'");
        }
        else if (eardbd_host != NULL)
        {
            if (strcmp(eardbd_host, "all"))
            {
                strcpy(query, AGGR_TIME);
                strcat(query, " AND eardbd_host='");
                strcat(query, eardbd_host);
                strcat(query, "'");
            }
            else strcpy(query, AGGR_TIME);
        }
        else
            strcpy(query, MET_TIME);

        sprintf(final_query, query, start_time, end_time);
        if (verbose) {
            printf( "QUERY: %s\n", final_query);
        }

        //time_t start, end; 

        PGresult *res = PQexecParams(connection, final_query, 0, NULL, NULL, NULL, NULL, 1);

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            avg_pow = -2;
            return;
        }
        
        global_start_time = (htonl(*(int *)(PQgetvalue(res, 0, 0))));
        global_end_time = (htonl(*(int *)(PQgetvalue(res, 0, 1))));


        char sbuff[64], ebuff[64];
        if (verbose)
        {
            printf( "original  \t start_time: %d\t end_time: %d\n\n", start_time, end_time);
            printf( "from query\t start_time: %ld\t end_time: %ld\n\n", global_start_time, global_end_time);
            printf( "from query\t start_time: %s\t end_time: %s\n\n",
                    ctime_r(&global_start_time, sbuff), ctime_r(&global_end_time, ebuff));
            printf( "result: %llu\n", result);
        }
        if (global_start_time != global_end_time && result > 0)
            avg_pow = result / (global_end_time-global_start_time);
    
        if (verbose)
        {
            printf( "avg_pow after computation: %llu\n", avg_pow);
            printf( "end-start: %ld\n", global_end_time-global_start_time);
        }

        PQclear(res);
    }

}
#endif

void event_type_to_str(int type, char *buff, size_t size)
{
    switch(type)
    {
        case DC_POWER_ERROR:
            strncpy(buff, "DC_POW_ERROR", size);
            break;
        case TEMP_ERROR:
            strncpy(buff, "TEMP_ERROR", size);
            break;
        case FREQ_ERROR:
            strncpy(buff, "FREQ_ERROR", size);
            break;
        case RAPL_ERROR:
            strncpy(buff, "RAPL_ERROR", size);
            break;
        case GBS_ERROR:
            strncpy(buff, "GBS_ERROR", size);
            break;
        case CPI_ERROR:
            strncpy(buff, "CPI_ERROR", size);
            break;
        case RESET_POWERCAP:
            strncpy(buff, "RESET_POWERCAP", size);
            break;
        case INC_POWERCAP:
            strncpy(buff, "INC_POWERCAP", size);
            break;
        case RED_POWERCAP:
            strncpy(buff, "RED_POWERCAP", size);
            break;
        case SET_POWERCAP:
            strncpy(buff, "SET_POWERCAP", size);
            break;
        case SET_ASK_DEF:
            strncpy(buff, "SET_ASK_DEF", size);
            break;
        case RELEASE_POWER:
            strncpy(buff, "RELEASE_POWER", size);
            break;
        case POWERCAP_VALUE:
            strncpy(buff, "POWERCAP_VALUE", size);
            break;
        default:
            snprintf(buff, size, "UNKNOWN(%d) ", type);
            break;
    }
}
/* EARD runtime events */
void print_event_type(int type)
{
    char buff[15];
    event_type_to_str(type, buff, 15);
    printf("%15s ", buff);
}

#if DB_MYSQL
void mysql_print_events(MYSQL_RES *result)
{

    int i;
    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    int has_records = 0;
    while ((row = mysql_fetch_row(result))!= NULL) 
    { 
        if (!has_records)
        {
            printf("%12s %20s %15s %8s %8s %20s %12s\n",
               "Event ID", "Timestamp", "Event type", "Job id", "Step id", "Value", "node_id");
            has_records = 1;
        }
        for(i = 0; i < num_fields; i++) {
            if (i == 2)
               print_event_type(atoi(row[i]));  
            else if (i == 1)
               printf("%20s ", row[i] ? row[i] : "NULL");
            else if (i == 4 || i == 3)
               printf("%8s ", row[i] ? row[i] : "NULL");
            else if (i == 5)
               printf("%20s ", row[i] ? row[i] : "NULL");
            else
               printf("%12s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    if (!has_records)
    {
        printf("There are no events with the specified properties.\n\n");
    }

}
#elif DB_PSQL
void postgresql_print_events(PGresult *res)
{
    int i, j, num_fields, has_records = 0;
    num_fields = PQnfields(res);

    for (i = 0; i < PQntuples(res); i++)
    {
        if (!has_records)
        {
            printf("%12s %22s %15s %8s %8s %20s %12s\n",
               "Event ID", "Timestamp", "Event type", "Job id", "Step id", "Value", "node_id");
            has_records = 1;
        }
        for (j = 0; j < num_fields; j++) {
            if (j == 2)
                print_event_type(atoi(PQgetvalue(res, i, j)));
            else if (i == 1)
               printf("%22s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else if (i == 4 || i == 3)
               printf("%8s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else if (i == 5)
               printf("%20s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else
               printf("%12s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
        }
        printf("\n");
    }
}
#endif

#if DB_MYSQL
#define EVENTS_QUERY "SELECT id, FROM_UNIXTIME(timestamp), event_type, job_id, step_id, freq, node_id FROM Events"
#elif DB_PSQL
#define EVENTS_QUERY "SELECT id, to_timestamp(timestamp), event_type, job_id, step_id, freq, node_id FROM Events"
#endif

void read_events(int start_time, int end_time, cluster_conf_t *my_conf) 
{
    char query[512];
    char subquery[128];
    int limit = 0;

    init_db_helper(&my_conf->database);
    strcpy(query, EVENTS_QUERY);

    if (start_time > 0)
        add_int_comp_filter(query, "timestamp", start_time, 1);
    else limit = 20;
    if (end_time > 0)
        add_int_comp_filter(query, "timestamp", end_time, 0);

    add_int_comp_filter(query, "event_type", 100, 1);

    if (limit > 0)
    {
        sprintf(subquery, " ORDER BY timestamp desc LIMIT %d", limit);
        strcat(query, subquery);
    }

    if (verbose) printf("QUERY: %s\n", query);
  
  
#if DB_MYSQL
    MYSQL_RES *result = db_run_query_result(query);
#elif DB_PSQL
    PGresult *result = db_run_query_result(query);
#endif
  
    if (result == NULL) 
    {
        printf("Database error\n");
        return;
    }

#if DB_MYSQL
    mysql_print_events(result);
#elif DB_PSQL
    postgresql_print_events(result);
#endif
}
#define GLOBAL_ENERGY_TYPE  1
#define PER_METRIC_TYPE     2
#define ALL_PER_METRIC_TYPE 3
void print_warning_level(int warn_level)
{
    switch(warn_level)
    {
        case 100:
            printf("%12s ", "GRACE PERIOD");
            break;
        case 3:
            printf("%12s ", "NO PROBLEM");
            break;
        case 2:
            printf("%12s ", "WARNING 1");
            break;
        case 1:
            printf("%12s ", "WARNING 2");
            break;
        case 0:
            printf("%12s ", "PANIC");
            break;
        default:
            printf("%12s ", "UNKNOWN");
            break;
    }
}

#if DB_MYSQL
void print_all(MYSQL *connection, int start_time, int end_time, char *inc_query, char type)
{
    char query[512];
    int i;
    char all_nodes = 0;
#if USE_GPUS
    char has_gpus = 0;
#endif

    uint max_power = MAX_ERROR_POWER;
    int def_id = get_default_tag_id(&my_conf);
    if (def_id > -1)
    {
        max_power = my_conf.tags[def_id].error_power;
    }   
    for (i = 0; i < my_conf.num_tags; i++)
        max_power = ear_max(my_conf.tags[i].error_power, max_power);
    
    
    if (type == GLOBAL_ENERGY_TYPE)
        sprintf(query, inc_query, end_time-start_time);
    else if (type == ALL_PER_METRIC_TYPE)
        sprintf(query, inc_query, start_time, end_time, max_power);
    else
        sprintf(query, inc_query, start_time, end_time);

    if (verbose) {
        printf( "query: %s\n", query);
    }
    
    if (mysql_query(connection, query))
    {
        printf( "MYSQL error\n");
        return;
    }
    MYSQL_RES *result = mysql_store_result(connection);
  
    if (result == NULL) 
    {
        printf( "MYSQL error\n");
        return;
    }

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    if (type == GLOBAL_ENERGY_TYPE)
    {
        int has_records = 0;
        while ((row = mysql_fetch_row(result))!= NULL) 
        { 
            if (!has_records)
            {
                printf("%20s %12s %20s %12s %12s %12s %12s %12s %12s %12s %12s\n",
                     "Energy%", "Warning lvl", "Timestamp", "INC th", "p_state", "ENERGY T1", "ENERGY T2",
                     "LIMIT", "TIME T1", "TIME T2", "POLICY");
                has_records = 1;
            }
            for(i = 0; i < num_fields; i++) {
                if (i == 0 || i == 2)
                    printf("%20s ", row[i] ? row[i] : "NULL");
                else if (i == 1)
                    print_warning_level(atoi(row[i]));  
                else
                    printf("%12s ", row[i] ? row[i] : "NULL");
            }
            if (row[0] && all_nodes) { //when getting energy we compute the avg_power
                printf("%15lld", (atoll(row[0]) /(global_end_time - global_start_time)));
            }
            printf("\n");
    	}
        if (!has_records)
        {
            char buff[64];
            time_t s_time = time(NULL) - end_time;
            strtok(ctime_r(&s_time, buff), "\n");
            printf("There are no global energy records in the period starting %s and ending now\n\n", buff);
        }
    }
    else
    {
        if (!strcmp(inc_query, ALL_USERS)) {
            printf( "%15s %15s\n", "Energy (J)", "User");
#if USE_GPUS
            has_gpus = 1;
#endif
        } else if (!strcmp(inc_query, ALL_TAGS)) {
            printf( "%15s %15s\n", "Energy (J)", "Energy tag");
#if USE_GPUS
            has_gpus = 1;
#endif
        } else if (!strcmp(inc_query, ALL_ISLANDS)) {
            printf( "%15s %15s\n", "Energy (J)", "EARDBD");
        } else if (global_end_time > 0) {
#if USE_GPUS
            printf( "%15s %15s %15s %15s\n", "Energy (J)", "Node", "Avg. DC Power", "Avg. GPU Power");
            has_gpus = 1;
#else
            printf( "%15s %15s %15s\n", "Energy (J)", "Node", "Avg. Power");
#endif
            all_nodes = 1;
        }
        else {
            printf( "%15s %15s\n", "Energy (J)", "Node");
        }


        while ((row = mysql_fetch_row(result))!= NULL) 
        { 
#if USE_GPUS
            for(i = 0; i < num_fields; i++) {
                if (has_gpus) {
                    if (!all_nodes || (i != 1 && i != 2 && i != 3))
                        printf("%15s ", row[i] ? row[i] : "NULL");
                } else {
                    for(i = 0; i < num_fields; i++) {
                        printf("%15s ", row[i] ? row[i] : "NULL");
                    }
                }
            }
#else
            for(i = 0; i < num_fields; i++) {
                if (!all_nodes || (i!= 2 && i != 1))
                    printf("%15s ", row[i] ? row[i] : "NULL");
            }
#endif
          
            if (row[0] && all_nodes && (atoll(row[2]) != atoll(row[1]))) { //when getting energy we compute the avg_power
                printf("%15lld", (atoll(row[0]) /(atoll(row[2]) - atoll(row[1]))));
#if USE_GPUS
                printf("%15lld", (atoll(row[3]) /(atoll(row[2]) - atoll(row[1]))));
#endif
    	}
            printf("\n");
        }
    }
    mysql_free_result(result);
}
#elif DB_PSQL
void print_all(PGconn *connection, int start_time, int end_time, char *inc_query, char type)
{
    char query[512];
    int i,j;
    char all_nodes = 0;

    int max_power = MAX_ERROR_POWER;
    int def_id = get_default_tag_id(&my_conf);
    if (def_id > -1)
    {
        max_power = my_conf.tags[def_id].error_power;
    }   
    int i;
    for (i = 0; i < my_conf.num_tags; i++)
        max_power = ear_max(my_conf.tags[i].error_power, max_power);
    
    if (type == GLOBAL_ENERGY_TYPE)
        sprintf(query, inc_query, end_time-start_time);
    else if (type == ALL_PER_METRIC_TYPE)
        sprintf(query, inc_query, start_time, end_time, max_power);
    else
        sprintf(query, inc_query, start_time, end_time);

    if (verbose) {
        printf( "query: %s\n", query);
    }
    
    PGresult *result = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 0);
  
    if (PQresultStatus(result) != PGRES_TUPLES_OK) 
    {
        printf("POSTGRESQL error\n");
        return;
    }

    int num_fields = PQnfields(result);

    if (type == GLOBAL_ENERGY_TYPE)
    {
        int has_records = 0;
        for (j = 0; j < PQntuples(result); j++) 
        { 
            if (!has_records)
            {
                printf("%20s %12s %20s %12s %12s %12s %12s %12s %12s %12s %12s\n",
                     "Energy%", "Warning lvl", "Timestamp", "INC th", "p_state", "ENERGY T1", "ENERGY T2",
                     "LIMIT", "TIME T1", "TIME T2", "POLICY");
                has_records = 1;
            }
            for(i = 0; i < num_fields; i++) {
                if (i == 0 || i == 2)
                    printf("%20s ", strlen(PQgetvalue(result, j, i)) ? PQgetvalue(result, j, i) : "NULL");
                else if (i == 1)
                    print_warning_level(atoi(PQgetvalue(result, j, i)));  
                else
                    printf("%12s ", strlen(PQgetvalue(result, j, i)) ? PQgetvalue(result, j, i) : "NULL");
            }
            if (strlen(PQgetvalue(result, j, 0)) && all_nodes) { //when getting energy we compute the avg_power
                printf("%15lld", atoll(PQgetvalue(result, j, 0))/(global_end_time - global_start_time));
            }
            printf("\n");
    	}
        if (!has_records)
        {
            char buff[64];
            time_t s_time = time(NULL) - end_time;
            strtok(ctime_r(&s_time, buff), "\n");
            printf("There are no global energy records in the period starting %s and ending now\n\n", buff);
        }
    }
    else
    {
        if (!strcmp(inc_query, ALL_USERS)) {
            printf( "%15s %15s\n", "Energy (J)", "User");
        } else if (!strcmp(inc_query, ALL_TAGS)) {
            printf( "%15s %15s\n", "Energy (J)", "Energy tag");
        } else if (!strcmp(inc_query, ALL_ISLANDS)) {
            printf( "%15s %15s\n", "Energy (J)", "EARDBD");
        } else if (global_end_time > 0) {
                printf( "%15s %15s %15s\n", "Energy (J)", "Node", "Avg. Power");
            all_nodes = 1;
        }
        else {
            printf( "%15s %15s\n", "Energy (J)", "Node");
        }


        for (j = 0; j < PQntuples(result); j++) 
        { 
            for(i = 0; i < num_fields; i++) {
                printf("%15s ", strlen(PQgetvalue(result, j, i)) ? PQgetvalue(result, j, i) : "NULL");
            }
          
            if (strlen(PQgetvalue(result, j, 0)) && all_nodes) { //when getting energy we compute the avg_power
                printf("%15lld", atoll(PQgetvalue(result, j, 0))/(global_end_time - global_start_time));
            }
            printf("\n");
        }
    }
    PQclear(result);
}
#endif

int main(int argc,char *argv[])
{
    char path_name[256];
    time_t start_time = 0;
    time_t end_time = time(NULL);
    time_t time_period = 0;
    int divisor = 1;
    int opt;
    char all_users = 0;
    char all_nodes = 0;
    char all_tags = 0;
    char all_eardbds = 0;
    char report_events = 0;
    char global_energy = 0;
    struct tm tinfo = {0};

    if (get_ear_conf_path(path_name) == EAR_ERROR)
    {
        printf( "Error getting ear.conf path.\n"); //error
        exit(1);
    }

    if (read_cluster_conf(path_name, &my_conf) != EAR_SUCCESS) {
        printf( "Error reading configuration\n"); //error
        exit(1);
    }
    
    if (getuid() != 0 && !is_privileged_command(&my_conf))
    {
        printf( "This command can only be executed by privileged users. Contact your admin for more info.\n");
        free_cluster_conf(&my_conf);
        exit(1); //error
    }

#if DB_MYSQL
    MYSQL *connection = mysql_init(NULL);
    if (!connection)
    {
        printf( "Error creating MYSQL object\n"); //error
        free_cluster_conf(&my_conf);
        exit(1);
    }

    if (strlen(my_conf.database.user_commands) < 1) 
        printf( "Warning: commands' user is not defined in ear.conf\n");

    if (!mysql_real_connect(connection, my_conf.database.ip, my_conf.database.user_commands, my_conf.database.pass_commands,
                            my_conf.database.database, my_conf.database.port, NULL, 0))
    {
        printf("Error connecting to the database (%d): %s\n",
                mysql_errno(connection), mysql_error(connection)); //error
        mysql_close(connection);
        free_cluster_conf(&my_conf);
        exit(1);
    }
#elif DB_PSQL
    if (strlen(my_conf.database.user_commands) < 1)
        printf("Warning: commands' user is not defined in ear.conf\n");

    strcpy(my_conf.database.user, my_conf.database.user_commands);
    strcpy(my_conf.database.pass, my_conf.database.pass_commands);

    init_db_helper(&my_conf.database);
    PGconn *connection = postgresql_create_connection();
    if (connection == NULL)
    {
        printf("Error connecting to the database\n");
        free_cluster_conf(&my_conf);
        exit(1);
    }
#endif

    while ((opt = getopt(argc, argv, "t:vhbn:u:s:e:i:gx")) != -1)
    {
        switch(opt)
        {
            case 'h':
                usage(argv[0]);
                break;
            case 'v':
                print_version();
                exit(0);
                break;
            case 'b':
                verbose=1;
                break;
            case 'n':
                node_name = optarg;
                if (!strcmp(node_name, "all"))
                    all_nodes=1;
                break;
            case 'i':
                eardbd_host = optarg;
                if (!strcmp(eardbd_host, "all"))
                    all_eardbds=1;
                break;
            case 'u':
                user_name = optarg;
                if (!strcmp(user_name, "all"))
                    all_users=1;
                break;
            case 't':
                etag = optarg;
                if (!strcmp(etag, "all"))
                    all_tags=1;
                break;
            case 'g':
                time_period = my_conf.eargm.t2*2;
                if (optind < argc && strchr(argv[optind], '-') == NULL)
                    time_period = atoi(argv[optind]);
                global_energy = 1;
                break;
            case 'e':
                if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL)
                {
                    printf( "Incorrect time format. Supported format is YYYY-MM-DD\n"); //error
#if DB_MYSQL
                    mysql_close(connection);
#elif DB_PSQL
                    PQfinish(connection);
#endif
                    free_cluster_conf(&my_conf);
                    exit(1);
                    break;
                }
                end_time = mktime(&tinfo);
                break;
            case 's':
                if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL)
                {
                    printf( "Incorrect time format. Supported format is YYYY-MM-DD\n"); //error
#if DB_MYSQL
                    mysql_close(connection);
#elif DB_PSQL
                    PQfinish(connection);
#endif
                    free_cluster_conf(&my_conf);
                    exit(1);
                    break;
                }
                start_time = mktime(&tinfo);
                break;
            case 'x':
                report_events = 1;
                break;
        }
    }

    if (start_time == 0) start_time = end_time - MAX(my_conf.eard.period_powermon, my_conf.db_manager.insr_time)*4;
    if (!all_users && !all_nodes && !all_tags && !all_eardbds && !global_energy && !report_events)
    {
        long long result = get_sum(connection, start_time, end_time, divisor);
        compute_pow(connection, start_time, end_time, result);
    
        if (!result) {
            printf( "No results in that period of time found\n");
        } else if (result < 0)
        {
            printf( "Error querying the database.\n"); //error
            exit(1);
        }
        else
        {
            char sbuff[64], ebuff[64];
            strtok(ctime_r(&end_time, ebuff), "\n");
            strtok(ctime_r(&start_time, sbuff), "\n");
            printf( "Total energy spent from %s to %s: %llu J\n", sbuff, ebuff, result);
            if (avg_pow <= 0)
            {
                if (user_name == NULL && etag == NULL && verbose)
                    printf( "Error when reading time info from database, could not compute average power.\n"); //error
            }
            else if (avg_pow > 0) {
                printf( "Average power during the reported period: %llu W\n", avg_pow);
            }
        }    
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        free_cluster_conf(&my_conf);
        exit(0);
    }

    if (all_users)
        print_all(connection, start_time, end_time, ALL_USERS, ALL_PER_METRIC_TYPE);
    else if (all_tags)
        print_all(connection, start_time, end_time, ALL_TAGS, ALL_PER_METRIC_TYPE);
    else if (all_nodes)
    {
        compute_pow(connection, start_time, end_time, 0);
        print_all(connection, start_time, end_time, ALL_NODES, PER_METRIC_TYPE);
    }
    else if (all_eardbds)
    {
        compute_pow(connection, start_time, end_time, 0);
        print_all(connection, start_time, end_time, ALL_ISLANDS, PER_METRIC_TYPE);
    }
    else if (global_energy)
    {
        if (start_time > 0)
            print_all(connection, start_time, end_time, GLOB_ENERGY, GLOBAL_ENERGY_TYPE);
        else
            print_all(connection, start_time, time_period, GLOB_ENERGY, GLOBAL_ENERGY_TYPE);
    }
    else if (report_events)
    {
        //read_events(char *user, int job_id, int limit, int step_id, char *job_ids) 
        read_events(start_time, end_time, &my_conf); 

    }
#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    free_cluster_conf(&my_conf);


    exit(0);
}




