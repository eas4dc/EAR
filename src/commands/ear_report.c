/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _XOPEN_SOURCE 700 // to get rid of the warning
#define AGGREGATED    1

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <common/config.h>
#include <common/database/db_helper.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/user.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/version.h>
#include <daemon/log_eard.h>
#include <global_manager/log_eargmd.h>

#if DB_MYSQL
#include <mysql/mysql.h>
#elif DB_PSQL
#include <libpq-fe.h>
#endif

#define PUE              1.2
#define CARBON_INTENSITY 174

#define MAX(a, b)        ((a) > (b) ? (a) : (b))

/* MYSQL QUERIES */
#if DB_MYSQL
#define MET_TIME                                                                                                       \
    "SELECT MIN(start_time), MAX(end_time) FROM Periodic_metrics WHERE start_time"                                     \
    ">= ? AND end_time <= ?"

#define AGGR_TIME                                                                                                      \
    "SELECT MIN(start_time), MAX(end_time) FROM Periodic_aggregations WHERE start_time"                                \
    ">= ? AND end_time <= ?"

#define AGGR_QUERY                                                                                                     \
    "SELECT SUM(dc_energy)/? FROM Periodic_aggregations WHERE start_time"                                              \
    ">= ? AND end_time <= ?"

#define USER_QUERY                                                                                                     \
    "SELECT SUM(DC_power*time)/? FROM Power_signatures WHERE id IN "                                                   \
    "(SELECT Applications.eard_signature_id FROM Applications JOIN Jobs "                                              \
    "ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                              \
    "Jobs.user_name = '%s' AND start_time >= ? AND end_time <= ? AND DC_power < %d)"
#if 0
#define GROUP_QUERY                                                                                                    \
    "SELECT SUM(DC_power*time)/? FROM Power_signatures WHERE id IN "                                                   \
    "(SELECT Applications.eard_signature_id FROM Applications JOIN Jobs "                                              \
    "ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                              \
    "Jobs.user_group LIKE '%%%s%%' AND start_time >= ? AND end_time <= ? AND DC_power < %d)"
#endif

// Same format as ALL
#define USER_QUERY_TABLE                                                                                               \
    "SELECT TRUNCATE(SUM(Power_signatures.DC_power*Power_signatures.time), 0) as energy, Jobs.user_name "              \
    "FROM Power_signatures "                                                                                           \
    "JOIN Applications ON Power_signatures.id = Applications.eard_signature_id "                                       \
    "JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                          \
    "WHERE Jobs.user_name = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d "                           \
    "GROUP BY Jobs.user_name"
// Deprecated
#define GROUP_QUERY                                                                                                    \
    "SELECT SUM((Power_signatures.DC_power*Power_signatures.time) / "                                                  \
    "  (1 + LENGTH(Jobs.user_group) - LENGTH(REPLACE(Jobs.user_group, ',', '')))"                                      \
    ")/? FROM Power_signatures "                                                                                       \
    "JOIN Applications ON Power_signatures.id = Applications.eard_signature_id "                                       \
    "JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                    \
    "FIND_IN_SET('%s', REPLACE(Jobs.user_group, ' ', '')) > 0 "                                                        \
    "AND start_time >= ? AND end_time <= ? AND DC_power < %d"
// Same format as All
#define GROUP_QUERY_TABLE                                                                                              \
    "SELECT TRUNCATE(SUM((Power_signatures.DC_power*Power_signatures.time) / "                                         \
    "  (1 + LENGTH(TRIM(Jobs.user_group)) - LENGTH(REPLACE(TRIM(Jobs.user_group), ',', '')))), 0) as energy, "         \
    "'%s' as user_group "                                                                                              \
    "FROM Power_signatures "                                                                                           \
    "JOIN Applications ON Power_signatures.id = Applications.eard_signature_id "                                       \
    "JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                          \
    "WHERE FIND_IN_SET('%s', REPLACE(Jobs.user_group, ' ', '')) > 0 "                                                  \
    "AND start_time >= %d AND end_time <= %d AND DC_power < %d"

#if USE_GPUS
#define NODE_QUERY_TABLE                                                                                               \
    "SELECT SUM(DC_energy), MIN(start_time), MAX(end_time), SUM(GPU_energy), node_id FROM Periodic_metrics "           \
    "WHERE start_time >= %d AND end_time <= %d AND node_id='%s' "                                                      \
    "GROUP BY node_id"
#else
#define NODE_QUERY_TABLE                                                                                               \
    "SELECT SUM(DC_energy), MIN(start_time), MAX(end_time), node_id FROM Periodic_metrics "                            \
    "WHERE start_time >= %d AND end_time <= %d AND node_id='%s' "                                                      \
    "GROUP BY node_id"
#endif

#define ETAG_QUERY                                                                                                     \
    "SELECT SUM(DC_power*time)/? FROM Power_signatures WHERE id IN "                                                   \
    "(SELECT Applications.eard_signature_id FROM Applications JOIN Jobs "                                              \
    "ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                              \
    "Jobs.e_tag = '%s' AND start_time >= ? AND end_time <= ? AND DC_power < %d)"

#define SUM_QUERY                                                                                                      \
    "SELECT SUM(dc_energy)/? FROM Periodic_metrics WHERE start_time"                                                   \
    ">= ? AND end_time <= ?"

#define TAG_QUERY_TABLE                                                                                                \
    "SELECT TRUNCATE(SUM(Power_signatures.DC_power*Power_signatures.time), 0) as energy, Jobs.e_tag FROM "             \
    "Power_signatures INNER JOIN Applications ON Power_signatures.id=Applications.eard_signature_id "                  \
    "INNER JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                    \
    "WHERE Jobs.e_tag = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d "                               \
    "GROUP BY Jobs.e_tag ORDER BY energy"

#define EARDBD_QUERY_TABLE                                                                                             \
    "SELECT SUM(DC_energy), eardbd_host FROM Periodic_aggregations "                                                   \
    "WHERE start_time >= %d AND end_time <= %d AND eardbd_host='%s' GROUP BY eardbd_host"

/* POSTGRESQL QUERIES */
#elif DB_PSQL
#include <libpq-fe.h>

#define MET_TIME                                                                                                       \
    "SELECT MIN(start_time), MAX(end_time) FROM Periodic_metrics WHERE start_time"                                     \
    ">= %d AND end_time <= %d"

#define AGGR_TIME                                                                                                      \
    "SELECT MIN(start_time), MAX(end_time) FROM Periodic_aggregations WHERE start_time"                                \
    ">= %d AND end_time <= %d"

#define AGGR_QUERY                                                                                                     \
    "SELECT SUM(dc_energy)/%llu FROM Periodic_aggregations WHERE start_time"                                           \
    ">= %d AND end_time <= %d"

#define USER_QUERY                                                                                                     \
    "SELECT SUM(DC_power*time)/%llu FROM Power_signatures WHERE id IN "                                                \
    "(SELECT Applications.eard_signature_id FROM Applications JOIN Jobs "                                              \
    "ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                              \
    "Jobs.user_name = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d)"

#define GROUP_QUERY                                                                                                    \
    "SELECT SUM(DC_power*time)/%llu FROM Power_signatures WHERE id IN "                                                \
    "(SELECT Applications.eard_signature_id FROM Applications JOIN Jobs "                                              \
    "ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                              \
    "Jobs.user_group = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d)"

#define ETAG_QUERY                                                                                                     \
    "SELECT SUM(DC_power*time)/%llu FROM Power_signatures WHERE id IN "                                                \
    "(SELECT Applications.eard_signature_id FROM Applications JOIN Jobs "                                              \
    "ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id WHERE "                              \
    "Jobs.e_tag = '%s' AND start_time >= %d AND end_time <= %d AND DC_power < %d)"

#define SUM_QUERY                                                                                                      \
    "SELECT SUM(dc_energy)/%llu FROM Periodic_metrics WHERE start_time"                                                \
    ">= %d AND end_time <= %d"

#endif

/* COMMON QUERIES */
#define ALL_USERS                                                                                                      \
    "SELECT TRUNCATE(SUM(DC_power*time), 0) as energy, Jobs.user_name FROM "                                           \
    "Power_signatures INNER JOIN Applications On id=Applications.eard_signature_id "                                   \
    "INNER JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                    \
    "WHERE start_time >= %d AND end_time <= %d AND DC_power < %d GROUP BY Jobs.user_name ORDER BY energy"
#if 0
#define ALL_GROUPS                                                                                                     \
    "SELECT TRUNCATE(SUM(DC_power*time), 0) as energy, Jobs.user_group FROM "                                          \
    "Power_signatures INNER JOIN Applications On id=Applications.eard_signature_id "                                   \
    "INNER JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                    \
    "WHERE start_time >= %d AND end_time <= %d AND DC_power < %d GROUP BY Jobs.user_group ORDER BY energy"
#endif
#define ALL_GROUPS                                                                                                     \
    "SELECT TRUNCATE(SUM((Power_signatures.DC_power*Power_signatures.time) / "                                         \
    "  (1 + LENGTH(Jobs.user_group) - LENGTH(REPLACE(Jobs.user_group, ',', '')))), 0) as energy, "                     \
    "groups.group_name FROM "                                                                                          \
    "Power_signatures INNER JOIN Applications ON Power_signatures.id=Applications.eard_signature_id "                  \
    "INNER JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                    \
    "INNER JOIN (SELECT DISTINCT TRIM(SUBSTRING_INDEX(SUBSTRING_INDEX(Jobs.user_group, ',', n.n), ',', -1)) "          \
    "as group_name FROM Jobs "                                                                                         \
    "JOIN (SELECT 1 n UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4 UNION ALL SELECT 5 "                    \
    "UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9 UNION ALL SELECT 10) n "              \
    "ON n.n <= 1 + LENGTH(Jobs.user_group) - LENGTH(REPLACE(Jobs.user_group, ',', ''))) groups "                       \
    "ON FIND_IN_SET(groups.group_name, REPLACE(Jobs.user_group, ' ', '')) > 0 "                                        \
    "WHERE start_time >= %d AND end_time <= %d AND DC_power < %d "                                                     \
    "GROUP BY groups.group_name ORDER BY energy"

#if USE_GPUS
#define ALL_NODES                                                                                                      \
    "select SUM(DC_energy), MIN(start_time), MAX(end_time), SUM(GPU_energy), node_id FROM Periodic_metrics WHERE "     \
    "start_time >= %d "                                                                                                \
    " AND end_time <= %d GROUP BY node_id "
#define FULL_METS                                                                                                      \
    "select node_id, DC_energy/(end_time-start_time), PCK_energy/(end_time-start_time), "                              \
    "DRAM_energy/(end_time-start_time), GPU_energy/(end_time-start_time), temp, avg_f, "                               \
    "from_unixtime(start_time), from_unixtime(end_time) FROM Periodic_metrics "                                        \
    "WHERE start_time >= %d AND end_time <= %d"
#else
#define ALL_NODES                                                                                                      \
    "select SUM(DC_energy), MIN(start_time), MAX(end_time), node_id FROM Periodic_metrics WHERE start_time >= %d "     \
    " AND end_time <= %d GROUP BY node_id "
#define FULL_METS                                                                                                      \
    "select node_id, DC_energy/(end_time-start_time), PCK_energy/(end_time-start_time), "                              \
    "DRAM_energy/(end_time-start_time), temp, avg_f, "                                                                 \
    "from_unixtime(start_time), from_unixtime(end_time) FROM Periodic_metrics "                                        \
    "WHERE start_time >= %d AND end_time <= %d"
#endif

#define ALL_ISLANDS                                                                                                    \
    "SELECT SUM(DC_energy), eardbd_host FROM Periodic_aggregations WHERE start_time >= %d "                            \
    " AND end_time <= %d GROUP BY eardbd_host"

#define ISLANDS_RAW                                                                                                    \
    "SELECT DC_energy, eardbd_host, start_time, end_time, DC_energy/(end_time-start_time) FROM "                       \
    "Periodic_aggregations WHERE start_time >= %d AND end_time <= %d"

#define ALL_TAGS                                                                                                       \
    "SELECT TRUNCATE(SUM(DC_power*time), 0) as energy, Jobs.e_tag FROM "                                               \
    "Power_signatures INNER JOIN Applications ON id=Applications.eard_signature_id "                                   \
    "INNER JOIN Jobs ON Applications.job_id = Jobs.job_id AND Applications.step_id = Jobs.step_id "                    \
    "WHERE start_time >= %d AND end_time <= %d AND DC_power < %d GROUP BY Jobs.e_tag ORDER BY energy"

#if EXP_EARGM
#define GLOB_ENERGY                                                                                                    \
    "SELECT ROUND(energy_percent, 2),warning_level,time,inc_th,p_state,GlobEnergyConsumedT1, "                         \
    "GlobEnergyConsumedT2,GlobEnergyLimit,GlobEnergyPeriodT1,GlobEnergyPeriodT2,GlobEnergyPolicy "                     \
    "FROM Global_energy2 WHERE UNIX_TIMESTAMP(time) >= (UNIX_TIMESTAMP(NOW())-%d) ORDER BY time desc "
#else
#define GLOB_ENERGY                                                                                                    \
    "SELECT ROUND(energy_percent, 2),warning_level,time,inc_th,p_state,GlobEnergyConsumedT1, "                         \
    "GlobEnergyConsumedT2,GlobEnergyLimit,GlobEnergyPeriodT1,GlobEnergyPeriodT2,GlobEnergyPolicy "                     \
    "FROM Global_energy WHERE UNIX_TIMESTAMP(time) >= (UNIX_TIMESTAMP(NOW())-%d) ORDER BY time desc "
#endif

// To be used in print_all
#define GLOBAL_ENERGY_TYPE  1
#define PER_METRIC_TYPE     2
#define ALL_PER_METRIC_TYPE 3
#define USER_TYPE           10
#define GROUP_TYPE          11
#define NODE_TYPE           12
#define TAG_TYPE            13
#define EARDBD_TYPE         14
#define EARDBD_RAW_TYPE     15

char *node_name            = NULL;
char *user_name            = NULL;
char *group_name           = NULL;
char *etag                 = NULL;
char *eardbd_host          = NULL;
int verbose                = 0;
int query_filters          = 0;
unsigned long long avg_pow = 0;
time_t global_start_time   = 0;
time_t global_end_time     = 0;
cluster_conf_t my_conf;

#if DB_PSQL

/***************************************
 *    Auxiliary PostgreSQL functions    *
 ***************************************/
static PGconn *postgresql_create_connection()
{
    char temp[32];
    char **keys, **values;
    PGconn *connection;

    db_conf_t *db_config = &my_conf.database;

    sprintf(temp, "%d", db_config->port);
    strtolow(db_config->database);

    keys   = calloc(4, sizeof(char *));
    values = calloc(4, sizeof(char *));

    keys[0] = "dbname";
    keys[1] = "user";
    keys[2] = "password";
    keys[3] = "host";

    values[0] = db_config->database;
    values[1] = db_config->user;
    values[2] = db_config->pass;
    values[3] = db_config->ip;

    connection = PQconnectdbParams((const char *const *) keys, (const char *const *) values, 0);

    free(keys);
    free(values);

    if (PQstatus(connection) != CONNECTION_OK) {
        verbose(VDBH, "psql.c: ERROR connecting to the database: %s", PQerrorMessage(connection));
        PQfinish(connection);
        return NULL;
    }

    return connection;
}
#endif

void usage(char *app)
{
    printf("Usage: %s [options]\n", app);
    printf("\nOptions:\n");
    printf("  %-32s %s\n", "-h, --help", "Displays this message.");
    printf("  %-32s %s\n", "-v, --version", "Displays the current EAR version.");
    printf("  %-32s %s\n", "-b, --verbose", "Enables verbose mode for debugging purposes.");
    printf("\nFilters:\n");
    printf("  %-32s %s\n", "-s, --start-time <YYYY-MM-DD>", "Sets the start of the period.");
    printf("  %-32s %s\n", "", "Default: end time minus twice the insertion time.");
    printf("  %-32s %s\n", "-e, --end-time <YYYY-MM-DD>", "Sets the end of the period. Default: current time.");
    printf("  %-32s %s\n", "-n, --nodes <node|all>", "Filters by node name. Default: none.");
    printf("  %-32s %s\n", "-u, --users <user|all>", "Filters by username. Default: none.");
    printf("  %-32s %s\n", "-G, --group <group|all>", "Filters by group name. Default: none.");
    printf("  %-32s %s\n", "-t, --etags <tag|all>", "Filters by energy tag. Default: none.");
    printf("  %-32s %s\n", "-i, --islands <eardbd|all>", "Filters by EARDBD island. Default: none.");
    printf("\nOutput:\n");
    printf("  %-32s %s\n", "-d, --expanded", "Expands island results to individual records.");
    printf("  %-32s %s\n", "", "If no island is specified, prints all of them.");
    printf("  %-32s %s\n", "-g, --global-energy", "Shows EAR database global energy records.");
    printf("  %-32s %s\n", "", "Defaults to the two previous EARGM T2 periods.");
    printf("  %-32s %s\n", "", "This option can be modified with --start-time, but not --end-time.");
    printf("  %-32s %s\n", "-x, --events", "Shows daemon events for the selected period.");
    printf("  %-32s %s\n", "-z, --detailed", "Shows detailed periodic metrics for the selected period.");
    printf("  %-32s %s\n", "-w, --total-savings", "Shows the total energy savings in kWh.");
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
    //  sprintf(query, query, value);
    query_filters++;
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
    query_filters++;
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
    query_filters++;
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
    //  sprintf(query, query, value);
    query_filters++;
}

#if DB_MYSQL
long long ereport_stmt_error(MYSQL_STMT *statement)
{
    printf("Error preparing statement (%d): %s\n", mysql_stmt_errno(statement), mysql_stmt_error(statement));
    mysql_stmt_close(statement);
    return -1;
}
#endif

#if DB_MYSQL
long long get_sum(MYSQL *connection, int start_time, int end_time, unsigned long long divisor)
{

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement) {
        printf("Error creating statement (%d): %s\n", mysql_errno(connection),
               mysql_error(connection)); // error
        return -1;
    }

    uint max_power = MAX_ERROR_POWER;
    int def_id     = get_default_tag_id(&my_conf);
    char is_node   = 0;
    if (node_name != NULL) {
        my_node_conf_t *n_c = get_my_node_conf(&my_conf, node_name);
        if (n_c != NULL) {
            max_power = n_c->max_error_power;
            is_node   = 1;
        }
    }
    if (!is_node) {
        if (def_id > -1) {
            max_power = my_conf.tags[def_id].error_power;
        }
        for (uint32_t i = 0; i < my_conf.num_tags; i++)
            max_power = ear_max(my_conf.tags[i].error_power, max_power);
    }

    char query[2048];

    if (node_name != NULL) {
        strcpy(query, SUM_QUERY);
        strcat(query, " AND node_id='");
        strcat(query, node_name);
        strcat(query, "'");
    } else if (user_name != NULL) {
        sprintf(query, USER_QUERY, user_name, max_power);
    } else if (etag != NULL) {
        sprintf(query, ETAG_QUERY, etag, max_power);
    } else if (group_name != NULL) {
        sprintf(query, GROUP_QUERY, group_name, max_power);
    } else if (eardbd_host != NULL) {
        strcpy(query, AGGR_QUERY);
        strcat(query, " AND eardbd_host='");
        strcat(query, eardbd_host);
        strcat(query, "'");
    }

#if AGGREGATED
    else
        strcpy(query, AGGR_QUERY);
#else
    else
        strcpy(query, SUM_QUERY);
#endif

    if (verbose) {
        printf("QUERY: %s\n", query);
    }
    if (mysql_stmt_prepare(statement, query, strlen(query)))
        return ereport_stmt_error(statement);

    // Query parameters binding
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = 1;

    bind[1].buffer_type = bind[2].buffer_type = MYSQL_TYPE_LONG;

    bind[0].buffer = (char *) &divisor;
    bind[1].buffer = (char *) &start_time;
    bind[2].buffer = (char *) &end_time;

    // Result parameters
    long long result = 0;
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    res_bind[0].is_unsigned = 1;
    res_bind[0].buffer      = &result;

    if (mysql_stmt_bind_param(statement, bind))
        return ereport_stmt_error(statement);
    if (mysql_stmt_bind_result(statement, res_bind))
        return ereport_stmt_error(statement);
    if (mysql_stmt_execute(statement))
        return ereport_stmt_error(statement);
    if (mysql_stmt_store_result(statement))
        return ereport_stmt_error(statement);

    int status = mysql_stmt_fetch(statement);
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
        result = -2;

    if (mysql_stmt_free_result(statement)) {
        printf("ERROR when freing result.\n"); // error
    }

    if (mysql_stmt_close(statement)) {
        printf("ERROR when freeing statement\n"); // error
    }

    return result;
}
#elif DB_PSQL
long long get_sum(PGconn *connection, int start_time, int end_time, unsigned long long divisor)
{

    long long result = 0;
    char query[512];

    uint max_power = MAX_ERROR_POWER;
    int def_id     = get_default_tag_id(&my_conf);
    char is_node   = 0;
    if (node_name != NULL) {
        my_node_conf_t *n_c = get_my_node_conf(&my_conf, node_name);
        if (n_c != NULL) {
            max_power = n_c->max_error_power;
            is_node   = 1;
        }
    }
    if (!is_node) {
        if (def_id > -1) {
            max_power = my_conf.tags[def_id].error_power;
        }
        int i;
        for (i = 0; i < my_conf.num_tags; i++)
            max_power = ear_max(my_conf.tags[i].error_power, max_power);
    }

    if (node_name != NULL) {
        sprintf(query, SUM_QUERY, divisor, start_time, end_time);
        strcat(query, " AND node_id='");
        strcat(query, node_name);
        strcat(query, "'");
    } else if (user_name != NULL) {
        sprintf(query, USER_QUERY, divisor, user_name, start_time, end_time, max_power);
    } else if (etag != NULL) {
        sprintf(query, ETAG_QUERY, divisor, etag, start_time, end_time, max_power);
    } else if (eardbd_host != NULL) {
        sprintf(query, AGGR_QUERY, divisor, start_time, end_time);
        strcat(query, " AND eardbd_host='");
        strcat(query, eardbd_host);
        strcat(query, "'");
    }
#if AGGREGATED
    else
        sprintf(query, AGGR_QUERY, divisor, start_time, end_time);
#else
    else
        sprintf(query, SUM_QUERY, divisor, start_time, end_time);
#endif

    if (verbose) {
        printf("QUERY: %s\n", query);
    }

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQgetisnull(res, 0, 0)) {
        PQclear(res);
        return 0;
    }

    result = (htonl(*(long long *) (PQgetvalue(res, 0, 0))));

    PQclear(res);

    return result;
}
#endif

#if DB_MYSQL
void compute_pow(MYSQL *connection, int start_time, int end_time, unsigned long long result)
{
    char query[512];
    if (user_name == NULL && group_name == NULL && etag == NULL) {
        MYSQL_STMT *statement = mysql_stmt_init(connection);
        if (!statement) {
            printf("Error creating statement (%d): %s\n", mysql_errno(connection), mysql_error(connection));
            return;
        }

        if (node_name != NULL && strcmp(node_name, "all")) {
            strcpy(query, MET_TIME);
            strcat(query, " AND node_id='");
            strcat(query, node_name);
            strcat(query, "'");
        } else if (eardbd_host != NULL) {
            if (strcmp(eardbd_host, "all")) {
                strcpy(query, AGGR_TIME);
                strcat(query, " AND eardbd_host='");
                strcat(query, eardbd_host);
                strcat(query, "'");
            } else
                strcpy(query, AGGR_TIME);
        } else
#if AGGREGATED
            strcpy(query, AGGR_TIME);
#else
            strcpy(query, MET_TIME);
#endif

        if (verbose) {
            printf("QUERY: %s\n", query);
        }

        if (mysql_stmt_prepare(statement, query, strlen(query))) {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        // time_t start, end;
        MYSQL_BIND sec_bind[2];
        memset(sec_bind, 0, sizeof(sec_bind));
        sec_bind[0].buffer_type = sec_bind[1].buffer_type = MYSQL_TYPE_LONG;
        sec_bind[0].buffer                                = (char *) &start_time;
        sec_bind[1].buffer                                = (char *) &end_time;

        MYSQL_BIND sec_res[2];
        memset(sec_res, 0, sizeof(sec_res));
        sec_res[0].buffer_type = sec_res[1].buffer_type = MYSQL_TYPE_LONGLONG;
        sec_res[0].buffer                               = &global_start_time;
        sec_res[1].buffer                               = &global_end_time;

        if (mysql_stmt_bind_param(statement, sec_bind)) {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        if (mysql_stmt_bind_result(statement, sec_res)) {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        if (mysql_stmt_execute(statement)) {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        if (mysql_stmt_store_result(statement)) {
            avg_pow = ereport_stmt_error(statement);
            return;
        }
        int status = mysql_stmt_fetch(statement);
        if (status != 0 && status != MYSQL_DATA_TRUNCATED)
            avg_pow = -2;

        char sbuff[64], ebuff[64];
        if (verbose) {
            printf("original  \t start_time: %d\t end_time: %d\n\n", start_time, end_time);
            printf("from query\t start_time: %ld\t end_time: %ld\n\n", global_start_time, global_end_time);
            printf("from query\t start_time: %s\t end_time: %s\n\n", ctime_r(&global_start_time, sbuff),
                   ctime_r(&global_end_time, ebuff));
            printf("result: %llu\n", result);
        }
        if (global_start_time != global_end_time && result > 0)
            avg_pow = result / (global_end_time - global_start_time);
        else if (start_time > 0) {
            global_start_time = start_time;
            global_end_time   = end_time;
        }

        if (verbose) {
            printf("avg_pow after computation: %llu\n", avg_pow);
            printf("end-start: %ld\n", global_end_time - global_start_time);
        }

        mysql_stmt_close(statement);
    }
}
#elif DB_PSQL
void compute_pow(PGconn *connection, int start_time, int end_time, unsigned long long result)
{
    char query[512], final_query[512];
    if (user_name == NULL && etag == NULL) {
        if (node_name != NULL && strcmp(node_name, "all")) {
            strcpy(query, MET_TIME);
            strcat(query, " AND node_id='");
            strcat(query, node_name);
            strcat(query, "'");
        } else if (eardbd_host != NULL) {
            if (strcmp(eardbd_host, "all")) {
                strcpy(query, AGGR_TIME);
                strcat(query, " AND eardbd_host='");
                strcat(query, eardbd_host);
                strcat(query, "'");
            } else
                strcpy(query, AGGR_TIME);
        } else
            strcpy(query, MET_TIME);

        sprintf(final_query, query, start_time, end_time);
        if (verbose) {
            printf("QUERY: %s\n", final_query);
        }

        // time_t start, end;

        PGresult *res = PQexecParams(connection, final_query, 0, NULL, NULL, NULL, NULL, 1);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            avg_pow = -2;
            return;
        }

        global_start_time = (htonl(*(int *) (PQgetvalue(res, 0, 0))));
        global_end_time   = (htonl(*(int *) (PQgetvalue(res, 0, 1))));

        char sbuff[64], ebuff[64];
        if (verbose) {
            printf("original  \t start_time: %d\t end_time: %d\n\n", start_time, end_time);
            printf("from query\t start_time: %ld\t end_time: %ld\n\n", global_start_time, global_end_time);
            printf("from query\t start_time: %s\t end_time: %s\n\n", ctime_r(&global_start_time, sbuff),
                   ctime_r(&global_end_time, ebuff));
            printf("result: %llu\n", result);
        }
        if (global_start_time != global_end_time && result > 0)
            avg_pow = result / (global_end_time - global_start_time);

        if (verbose) {
            printf("avg_pow after computation: %llu\n", avg_pow);
            printf("end-start: %ld\n", global_end_time - global_start_time);
        }

        PQclear(res);
    }
}
#endif
/* EARD runtime events */
void print_event_type(int type)
{
    ear_event_t ev;
    ev.event = type;
    char buff[15];
    event_type_to_str(&ev, buff, 15);
    printf("%15s ", buff);
}

#if DB_MYSQL
void mysql_print_events(MYSQL_RES *result)
{

    int i;
    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    int has_records = 0;
    while ((row = mysql_fetch_row(result)) != NULL) {
        if (!has_records) {
            printf("%12s %20s %15s %8s %8s %20s %12s\n", "Event ID", "Timestamp", "Event type", "Job id", "Step id",
                   "Value", "node_id");
            has_records = 1;
        }
        for (i = 0; i < num_fields; i++) {
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
    if (!has_records) {
        printf("There are no events with the specified properties.\n\n");
    }
}
#elif DB_PSQL
void postgresql_print_events(PGresult *res)
{
    int i, j, num_fields, has_records = 0;
    num_fields = PQnfields(res);

    for (i = 0; i < PQntuples(res); i++) {
        if (!has_records) {
            printf("%12s %22s %15s %8s %8s %20s %12s\n", "Event ID", "Timestamp", "Event type", "Job id", "Step id",
                   "Value", "node_id");
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
#define EVENTS_QUERY "SELECT id, FROM_UNIXTIME(timestamp), event_type, job_id, step_id, value, node_id FROM Events"
#elif DB_PSQL
#define EVENTS_QUERY "SELECT id, to_timestamp(timestamp), event_type, job_id, step_id, value, node_id FROM Events"
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
    else
        limit = 20;
    if (end_time > 0)
        add_int_comp_filter(query, "timestamp", end_time, 0);

    add_int_comp_filter(query, "event_type", 100, 1);

    if (limit > 0) {
        sprintf(subquery, " ORDER BY timestamp desc LIMIT %d", limit);
        strcat(query, subquery);
    }

    if (verbose)
        printf("QUERY: %s\n", query);

#if DB_MYSQL
    MYSQL_RES *result = db_run_query_result(query);
#elif DB_PSQL
    PGresult *result = db_run_query_result(query);
#endif

    if (result == NULL) {
        printf("Database error\n");
        return;
    }

#if DB_MYSQL
    mysql_print_events(result);
#elif DB_PSQL
    postgresql_print_events(result);
#endif
}

#define ACCUM_EVENT_TYPE 2002

void read_event_accum_2002(cluster_conf_t *my_conf)
{
    char query[2048];

    init_db_helper(&my_conf->database);

#if DB_MYSQL
    snprintf(query, sizeof(query),
             "SELECT e.node_id, e.timestamp, e.value "
             "FROM Events e "
             "JOIN ("
             "  SELECT node_id, MAX(timestamp) AS max_ts "
             "  FROM Events "
             "  WHERE event_type = %d "
             "  GROUP BY node_id"
             ") last "
             "ON e.node_id = last.node_id "
             "AND e.timestamp = last.max_ts "
             "WHERE e.event_type = %d "
             "ORDER BY e.node_id",
             ACCUM_EVENT_TYPE, ACCUM_EVENT_TYPE);

#elif DB_PSQL
    snprintf(query, sizeof(query),
             "SELECT node_id, timestamp, value "
             "FROM ("
             "  SELECT DISTINCT ON (node_id) node_id, timestamp, value "
             "  FROM Events "
             "  WHERE event_type = %d "
             "  ORDER BY node_id, timestamp DESC"
             ") x "
             "ORDER BY node_id",
             ACCUM_EVENT_TYPE);
#endif

    /*
     * Optional time filters.
     *
     * If the event is cumulative and you really want the latest ever,
     * do NOT apply start/end filters.
     *
     * If you want latest record within selected period, add timestamp filters.
     *
     * For now I assume "ultimo record" means latest available globally,
     * because the accumulated value is per node.
     */

    if (verbose) {
        printf("QUERY: %s\n", query);
    }

#if DB_MYSQL
    MYSQL_RES *result = db_run_query_result(query);

    if (result == NULL) {
        printf("Database error\n");
        return;
    }

    MYSQL_ROW row;
    int has_records = 0;
    float total     = 0;

    while ((row = mysql_fetch_row(result)) != NULL) {
        if (!has_records) {
            printf("%20s %20s %20s\n", "Node", "Timestamp", "Energy saving (KWH)");
            has_records = 1;
        }

        const char *node = row[0] ? row[0] : "NULL";
        const char *ts   = row[1] ? row[1] : "NULL";
        float val        = row[2] ? atof(row[2]) / 1000000.0 : 0.0;

        printf("%20s %20s %20f\n", node, ts, val);

        if (row[2]) {
            total += val;
        }
    }

    if (!has_records) {
        printf("There are no accumulated event records with event_type=%d.\n", ACCUM_EVENT_TYPE);
    } else {
        printf("\n%20s %20s %20f\n", "TOTAL", "", total);
    }

    mysql_free_result(result);

#elif DB_PSQL
    PGresult *result = db_run_query_result(query);

    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK) {
        printf("Database error\n");
        if (result != NULL) {
            PQclear(result);
        }
        return;
    }

    int rows    = PQntuples(result);
    float total = 0;

    if (rows > 0) {
        printf("%20s %20s %20s\n", "Node", "Timestamp", "Value");
    }

    for (int i = 0; i < rows; i++) {
        const char *node = PQgetvalue(result, i, 0);
        const char *ts   = PQgetvalue(result, i, 1);
        const char *val  = PQgetvalue(result, i, 2);

        float saving = val && strlen(val) ? atof(val) / 1000000.0 : 0;

        printf("%20s %20s %20f\n", node && strlen(node) ? node : "NULL", ts && strlen(ts) ? ts : "NULL", saving);

        total += saving;
    }

    if (rows == 0) {
        printf("There are no accumulated event records with event_type=%d.\n", ACCUM_EVENT_TYPE);
    } else {
        printf("\n%20s %20s %20%\n", "TOTAL", "", total);
    }

    PQclear(result);
#endif
}

void print_warning_level(int warn_level)
{
    switch (warn_level) {
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

void print_mets(int start_time, int end_time, char *nodes, cluster_conf_t *my_conf)
{
    char ***results;
    int num_columns, num_rows = 0;
    char query[1024];

    init_db_helper(&my_conf->database);

    sprintf(query, FULL_METS, start_time, end_time);
    if (nodes != NULL) {
        strcat(query, " AND node_id IN ('");
        strcat(query, nodes);
        strcat(query, "')");
    }

    db_run_query_string_results(query, &results, &num_columns, &num_rows);

    if (num_rows < 1) {
        printf("No periodic_metrics in the specified period of time\n");
        return;
    }

#if USE_GPUS
    printf("%10s\t%10s\t%10s\t%10s\t%10s\t%10s\t%15s\t%20s\t%20s\n", "Node", "DC power", "PCK power", "DRAM Power",
           "GPU power", "Temperature", "Avg. CPU Freq", "Start time", "End time");
    int total_fields = 7;
#else
    printf("%10s\t%10s\t%10s\t%10s\t%10s\t%15s\t%20s\t%20s\n", "Node", "DC power", "PCK power", "DRAM Power",
           "Temperature", "Avg. CPU Freq", "Start time", "End time");
    int total_fields = 6;
#endif
    int i, j;
    for (i = 0; i < num_rows; i++) {
        for (j = 0; j < num_columns; j++) {
            if (j < total_fields - 1)
                printf("%10s\t", results[i][j]);
            else if (j < total_fields)
                printf("%15s\t", results[i][j]);
            else
                printf("%20s\t", results[i][j]);
        }
        printf("\n");
    }
}

#if DB_MYSQL
void print_all(MYSQL *connection, int start_time, int end_time, char *inc_query, char type)
{
    char query[2048];
    char all_nodes = 0;

    uint max_power = MAX_ERROR_POWER;
    int def_id     = get_default_tag_id(&my_conf);
    if (def_id > -1) {
        max_power = my_conf.tags[def_id].error_power;
    }
    for (uint32_t i = 0; i < my_conf.num_tags; i++)
        max_power = ear_max(my_conf.tags[i].error_power, max_power);

    // Query selector
    if (type == GLOBAL_ENERGY_TYPE) {
        sprintf(query, inc_query, end_time - start_time);
    } else if (type == USER_TYPE) {
        if (!strcmp(inc_query, ALL_USERS))
            sprintf(query, inc_query, start_time, end_time, max_power);
        else
            sprintf(query, inc_query, user_name, start_time, end_time, max_power);
    } else if (type == GROUP_TYPE) {
        if (!strcmp(inc_query, ALL_GROUPS))
            sprintf(query, inc_query, start_time, end_time, max_power);
        else
            sprintf(query, inc_query, group_name, group_name, start_time, end_time, max_power);
    } else if (type == NODE_TYPE) {
        if (!strcmp(inc_query, ALL_NODES))
            sprintf(query, inc_query, start_time, end_time);
        else
            sprintf(query, inc_query, start_time, end_time, node_name);
    } else if (type == TAG_TYPE) {
        if (!strcmp(inc_query, ALL_TAGS))
            sprintf(query, inc_query, start_time, end_time, max_power);
        else
            sprintf(query, inc_query, etag, start_time, end_time, max_power);
    } else if (type == EARDBD_TYPE) {
        if (!strcmp(inc_query, ALL_ISLANDS))
            sprintf(query, inc_query, start_time, end_time);
        else
            sprintf(query, inc_query, start_time, end_time, eardbd_host);
    } else if (type == ALL_PER_METRIC_TYPE) {
        sprintf(query, inc_query, start_time, end_time, max_power);
    } else if (type == EARDBD_RAW_TYPE) {
        sprintf(query, inc_query, start_time, end_time);
    } else {
        sprintf(query, inc_query, start_time, end_time);
    }
    if (verbose) {
        printf("query: %s\n", query);
    }

    if (mysql_query(connection, query)) {
        printf("No results found or error connecting to the DB\n");
        return;
    }
    MYSQL_RES *result = mysql_store_result(connection);

    if (result == NULL) {
        printf("No results found or error connecting to the DB\n");
        return;
    }

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    if (type == GLOBAL_ENERGY_TYPE) {
        int has_records = 0;
        while ((row = mysql_fetch_row(result)) != NULL) {
            if (!has_records) {
                printf("%20s %12s %20s %12s %12s %12s %12s %12s %12s %12s %12s\n", "Energy%", "Warning lvl",
                       "Timestamp", "INC th", "p_state", "ENERGY T1", "ENERGY T2", "LIMIT", "TIME T1", "TIME T2",
                       "POLICY");
                has_records = 1;
            }
            for (int32_t i = 0; i < num_fields; i++) {
                if (i == 0 || i == 2)
                    printf("%20s ", row[i] ? row[i] : "NULL");
                else if (i == 1)
                    print_warning_level(atoi(row[i]));
                else
                    printf("%12s ", row[i] ? row[i] : "NULL");
            }
            if (row[0] && all_nodes) { // when getting energy we compute the avg_power
                printf("%15lld", (atoll(row[0]) / (global_end_time - global_start_time)));
            }
            printf("\n");
        }
        if (!has_records) {
            char buff[64];
            time_t s_time = time(NULL) - end_time;
            strtok(ctime_r(&s_time, buff), "\n");
            printf("There are no global energy records in the period starting %s and ending now\n\n", buff);
        }
    } else {

        int has_records = 0;
        while ((row = mysql_fetch_row(result)) != NULL) {
            // HEADER
            if (!has_records) // on the first iteration
            {
                has_records = 1;

                if (type == USER_TYPE) {
                    printf("%15s %15s %20s\n", "Energy (J)", "User", "Carbon footprint(g)");
                } else if (type == GROUP_TYPE) {
                    printf("%15s %15s %20s\n", "Energy (J)", "Group", "Carbon footprint(g)");
                } else if (type == TAG_TYPE) {
                    printf("%15s %15s %20s\n", "Energy (J)", "Energy tag", "Carbon footprint(g)");
                } else if (type == EARDBD_TYPE) {
                    printf("%15s %15s %20s\n", "Energy (J)", "EARDBD", "Carbon footprint(g)");
                } else if (type == EARDBD_RAW_TYPE) {
                    printf("%15s %15s %15s %15s %15s %20s\n", "Energy (J)", "EARDBD", "Start time", "End time", "Power",
                           "Carbon footprint(g)");
                } else if (type == NODE_TYPE || global_end_time > 0) {
#if USE_GPUS
                    printf("%15s %15s %15s %15s %15s %20s\n", "Energy (J)", "Node", "Active time(s)",
                           "Avg. DC Power(W)", "Avg. GPU Power(W)", "Carbon footprint(g)");
#else
                    printf("%15s %15s %15s %15s %20s\n", "Energy (J)", "Node", "Active time(s)", "Avg. DC Power(W)",
                           "Carbon footprint(g)");
#endif
                    all_nodes = 1;
                }
                printf("\n");
            }

            // printing of the main values
            if (!row[0] || !row[1] || !row[2] || !row[3] || !row[4])
                continue;
            // Energy
            printf("%15s ", row[0]);
            // Node
            printf("%15s ", row[4]);
            // Time
            printf("%15lld", (atoll(row[2]) - atoll(row[1])));
            // Node Avg. DC Power(W)
            if ((atoll(row[2]) != atoll(row[1]))) {
                printf("%15lld ", (atoll(row[0]) / (atoll(row[2]) - atoll(row[1]))));
            }
#if USE_GPUS
            // Avg. GPU Power(W)
            if (atoll(row[3]) && (atoll(row[2]) != atoll(row[1])))
                printf("%15lld ", (atoll(row[3]) / (atoll(row[2]) - atoll(row[1]))));
            else
                printf("%15s", " 0 ");
#endif
            // Carbon footprint formula: Energy X Power usage effectiveness X Carbon intensity
            printf("%20.2lf", ((double) atoll(row[0]) / (3600 * 1000) * PUE * CARBON_INTENSITY));
            printf("\n");
        }
        if (!has_records) {
            char sbuff[64], ebuff[64];
            time_t s_time = start_time;
            time_t e_time = end_time;
            strtok(ctime_r(&s_time, sbuff), "\n");
            strtok(ctime_r(&e_time, ebuff), "\n");
            printf("There are no records in the period starting %s and ending %s\n\n", sbuff, ebuff);
        }
    }
    mysql_free_result(result);
}

#elif DB_PSQL
void print_all(PGconn *connection, int start_time, int end_time, char *inc_query, char type)
{
    char query[512];
    int i, j;
    char all_nodes = 0;

    int max_power = MAX_ERROR_POWER;
    int def_id    = get_default_tag_id(&my_conf);
    if (def_id > -1) {
        max_power = my_conf.tags[def_id].error_power;
    }
    for (i = 0; i < my_conf.num_tags; i++)
        max_power = ear_max(my_conf.tags[i].error_power, max_power);

    if (type == GLOBAL_ENERGY_TYPE)
        sprintf(query, inc_query, end_time - start_time);
    else if (type == ALL_PER_METRIC_TYPE)
        sprintf(query, inc_query, start_time, end_time, max_power);
    else
        sprintf(query, inc_query, start_time, end_time);

    if (verbose) {
        printf("query: %s\n", query);
    }

    PGresult *result = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        printf("POSTGRESQL error\n");
        return;
    }

    int num_fields = PQnfields(result);

    if (type == GLOBAL_ENERGY_TYPE) {
        int has_records = 0;
        for (j = 0; j < PQntuples(result); j++) {
            if (!has_records) {
                printf("%20s %12s %20s %12s %12s %12s %12s %12s %12s %12s %12s\n", "Energy%", "Warning lvl",
                       "Timestamp", "INC th", "p_state", "ENERGY T1", "ENERGY T2", "LIMIT", "TIME T1", "TIME T2",
                       "POLICY");
                has_records = 1;
            }
            for (i = 0; i < num_fields; i++) {
                if (i == 0 || i == 2)
                    printf("%20s ", strlen(PQgetvalue(result, j, i)) ? PQgetvalue(result, j, i) : "NULL");
                else if (i == 1)
                    print_warning_level(atoi(PQgetvalue(result, j, i)));
                else
                    printf("%12s ", strlen(PQgetvalue(result, j, i)) ? PQgetvalue(result, j, i) : "NULL");
            }
            if (strlen(PQgetvalue(result, j, 0)) && all_nodes) { // when getting energy we compute the avg_power
                printf("%15lld", atoll(PQgetvalue(result, j, 0)) / (global_end_time - global_start_time));
            }
            printf("\n");
        }
        if (!has_records) {
            char buff[64];
            time_t s_time = time(NULL) - end_time;
            strtok(ctime_r(&s_time, buff), "\n");
            printf("There are no global energy records in the period starting %s and ending now\n\n", buff);
        }
    } else {
        if (!strcmp(inc_query, ALL_USERS)) {
            printf("%15s %15s\n", "Energy (J)", "User");
        } else if (!strcmp(inc_query, ALL_TAGS)) {
            printf("%15s %15s\n", "Energy (J)", "Energy tag");
        } else if (!strcmp(inc_query, ALL_ISLANDS)) {
            printf("%15s %15s\n", "Energy (J)", "EARDBD");
        } else if (!strcmp(inc_query, ISLANDS_RAW)) {
            printf("%15s %15s %15s %15s %15s\n", "Energy (J)", "EARDBD", "Start time", "End time", "Power");
        } else if (global_end_time > 0) {
            printf("%15s %15s %15s\n", "Energy (J)", "Node", "Avg. Power");
            all_nodes = 1;
        } else {
            printf("%15s %15s\n", "Energy (J)", "Node");
        }

        for (j = 0; j < PQntuples(result); j++) {
            for (i = 0; i < num_fields; i++) {
                printf("%15s ", strlen(PQgetvalue(result, j, i)) ? PQgetvalue(result, j, i) : "NULL");
            }

            if (strlen(PQgetvalue(result, j, 0)) && all_nodes) { // when getting energy we compute the avg_power
                printf("%15lld", atoll(PQgetvalue(result, j, 0)) / (global_end_time - global_start_time));
            }
            printf("\n");
        }
    }
    PQclear(result);
}
#endif

int main(int argc, char *argv[])
{
    char path_name[256];
    time_t start_time  = 0;
    time_t end_time    = time(NULL);
    time_t time_period = 0;
    int c;
    char all_users        = 0;
    char all_groups       = 0;
    char all_nodes        = 0;
    char all_tags         = 0;
    char all_eardbds      = 0;
    char global_energy    = 0;
    char islands_expanded = 0;
    struct tm tinfo       = {0};
    char report_savings   = 0;
    char report_events    = 0;
    char report_detailed  = 0;

    VCCONF = 2;

    if (check_and_unset_environment_variables()) {
        printf("Warning: using MySQL/MariaDB environment variables is not supported, unsetting them.\n");
    }

    if (state_fail(get_ear_conf_path(path_name))) {
        printf("Error getting ear.conf path, load the ear module\n");
        return EXIT_FAILURE;
    }
    if (state_fail(read_cluster_conf(path_name, &my_conf))) {
        printf("Impossible to read ear.conf\n");
        return EXIT_FAILURE;
    }
    if (state_fail(user_set_euid(UID_REAL))) {
        printf("Effective user can not be switch to real: %s\n", state_msg);
        return EXIT_FAILURE;
    }

    if (getuid() != 0 && !is_privileged_command(&my_conf)) {
        printf("This command can only be executed by privileged users. Contact your admin for more info.\n");
        free_cluster_conf(&my_conf);
        exit(EXIT_FAILURE);
    }

#if DB_MYSQL
    MYSQL *connection = mysql_init(NULL);
    if (!connection) {
        printf("Error creating MYSQL object\n"); // error
        free_cluster_conf(&my_conf);
        exit(1);
    }

    if (strlen(my_conf.database.user_commands) < 1)
        printf("Warning: commands' user is not defined in ear.conf\n");

    if (!mysql_real_connect(connection, my_conf.database.ip, my_conf.database.user_commands,
                            my_conf.database.pass_commands, my_conf.database.database, my_conf.database.port, NULL,
                            0)) {
        printf("Error connecting to the database (%d): %s\n", mysql_errno(connection),
               mysql_error(connection)); // error
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
    if (connection == NULL) {
        printf("Error connecting to the database\n");
        free_cluster_conf(&my_conf);
        exit(1);
    }
#endif

    int option_idx                      = 0;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},           {"version", no_argument, 0, 'v'},
        {"verbose", no_argument, 0, 'b'},        {"global-energy", no_argument, 0, 'g'},
        {"events", no_argument, 0, 'x'},         {"total-savings", no_argument, 0, 'w'},
        {"expanded", no_argument, 0, 'd'},       {"detailed", no_argument, 0, 'z'},
        {"nodes", required_argument, 0, 'n'},    {"users", required_argument, 0, 'u'},
        {"group", required_argument, 0, 'G'},    {"etags", required_argument, 0, 't'},
        {"islands", required_argument, 0, 'i'},  {"start-time", required_argument, 0, 's'},
        {"end-time", required_argument, 0, 'e'},
    };
    while (1) {
        c = getopt_long(argc, argv, "t:vhzdbn:u:G:s:e:i:gxw", long_options, &option_idx);

        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'v':
                print_version();
                exit(0);
                break;
            case 'b':
                verbose = 1;
                break;
            case 'n':
                node_name = optarg;
                if (!strcmp(node_name, "all"))
                    all_nodes = 1;
                break;
            case 'i':
                eardbd_host = optarg;
                all_eardbds = 0; // reset it in case -d was specified before
                if (!strcmp(eardbd_host, "all"))
                    all_eardbds = 1;
                break;
            case 'd':
                islands_expanded = 1;
                if (eardbd_host == NULL)
                    all_eardbds = 1;
                break;
            case 'G':
                group_name = optarg;
                if (!strcmp(group_name, "all"))
                    all_groups = 1;
                break;
            case 'u':
                user_name = optarg;
                if (!strcmp(user_name, "all"))
                    all_users = 1;
                break;
            case 't':
                etag = optarg;
                if (!strcmp(etag, "all"))
                    all_tags = 1;
                break;
            case 'g':
                time_period = my_conf.eargm.t2 * 2;
                if (optind < argc && strchr(argv[optind], '-') == NULL)
                    time_period = atoi(argv[optind]);
                global_energy = 1;
                break;
            case 'e':
                if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL) {
                    printf("Incorrect time format. Supported format is YYYY-MM-DD\n"); // error
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
                if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL) {
                    printf("Incorrect time format. Supported format is YYYY-MM-DD\n"); // error
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
            case 'w':
                report_savings = 1;
                break;
            case 'z':
                report_detailed = 1;
                all_nodes       = 0;
                break;
        }
    }

    if (start_time == 0)
        start_time = end_time - MAX(my_conf.eard.period_powermon, my_conf.db_manager.aggr_time) * 4;
#if DB_MYSQL
    if (user_name == NULL && group_name == NULL && node_name == NULL && etag == NULL && eardbd_host == NULL &&
        !global_energy && !report_events && !report_detailed && !report_savings) {
        unsigned long long divisor = 1;
        long long result           = get_sum(connection, start_time, end_time, divisor);
        compute_pow(connection, start_time, end_time, result);

        if (!result) {
            char sbuff[64], ebuff[64];
            strtok(ctime_r(&end_time, ebuff), "\n");
            strtok(ctime_r(&start_time, sbuff), "\n");
            printf("No results in that period of time found (from %s to %s)\n", sbuff, ebuff);
        } else if (result < 0) {
            printf("Error querying the database.\n"); // error
            exit(1);
        } else {
            char sbuff[64], ebuff[64];
            strtok(ctime_r(&end_time, ebuff), "\n");
            strtok(ctime_r(&start_time, sbuff), "\n");
            printf("Total energy spent from %s to %s: %llu J\n", sbuff, ebuff, result);
            double computing_energy = result * PUE;
            double carbon_footprint = computing_energy / (3600 * 1000) * CARBON_INTENSITY;
            if (avg_pow <= 0) {
                if (user_name == NULL && etag == NULL && verbose)
                    printf("Error when reading time info from database, could not compute average power.\n"); // error
            } else if (avg_pow > 0) {
                printf("Average power during the reported period: %llu W\n", avg_pow);
            }
            printf("Carbon footprint: %.2lf g\n", carbon_footprint);
        }
        mysql_close(connection);
        free_cluster_conf(&my_conf);
        exit(0);
    }
#endif
    // Run the query and print the results
    if (all_users)
        print_all(connection, start_time, end_time, ALL_USERS, USER_TYPE);
    else if (user_name != NULL)
        print_all(connection, start_time, end_time, USER_QUERY_TABLE, USER_TYPE);
    else if (all_groups)
        print_all(connection, start_time, end_time, ALL_GROUPS, GROUP_TYPE);
    else if (group_name != NULL)
        print_all(connection, start_time, end_time, GROUP_QUERY_TABLE, GROUP_TYPE);
    else if (all_tags)
        print_all(connection, start_time, end_time, ALL_TAGS, TAG_TYPE);
    else if (etag != NULL)
        print_all(connection, start_time, end_time, TAG_QUERY_TABLE, TAG_TYPE);
    else if (all_nodes) {
        compute_pow(connection, start_time, end_time, 0);
        print_all(connection, start_time, end_time, ALL_NODES, NODE_TYPE);
    } else if (node_name != NULL) {
        compute_pow(connection, start_time, end_time, 0);
        print_all(connection, start_time, end_time, NODE_QUERY_TABLE, NODE_TYPE);
    } else if (all_eardbds) {
        compute_pow(connection, start_time, end_time, 0);
        if (!islands_expanded)
            print_all(connection, start_time, end_time, ALL_ISLANDS, EARDBD_TYPE);
        else
            print_all(connection, start_time, end_time, ISLANDS_RAW, EARDBD_RAW_TYPE);
    } else if (eardbd_host != NULL) {
        compute_pow(connection, start_time, end_time, 0);
        print_all(connection, start_time, end_time, EARDBD_QUERY_TABLE, EARDBD_TYPE);
    } else if (global_energy) {
        if (start_time > 0)
            print_all(connection, start_time, end_time, GLOB_ENERGY, GLOBAL_ENERGY_TYPE);
        else
            print_all(connection, start_time, time_period, GLOB_ENERGY, GLOBAL_ENERGY_TYPE);
    } else if (report_events) {
        read_events(start_time, end_time, &my_conf);
    } else if (report_detailed) {
        print_mets(start_time, end_time, node_name, &my_conf);
    } else if (report_savings) {
        read_event_accum_2002(&my_conf);
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    free_cluster_conf(&my_conf);

    exit(0);
}
