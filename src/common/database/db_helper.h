/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/types/periodic_aggregation.h>
#include <common/types/power_signature.h>
#include <common/types/periodic_metric.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/application.h>
#include <common/types/gm_warning.h>
#include <common/types/loop.h>
#include <common/types/log.h>
#include <common/config.h>
#if DB_MYSQL
#include <mysql/mysql.h>
#elif DB_PSQL
#include "libpq-fe.h"
#endif

#if 0
#if DB_MYSQL
static MYSQL *mysql_create_connection();
#elif DB_PSQL
static PGconn *postgresql_create_connection();
#endif
#endif

#define _MAX(X,Y)			(X > Y ? X : Y)
#define _MMAAXX(W,X,Y,Z) 	(_MAX(W,X) > _MAX(Y,Z) ? _MAX(W,X) : _MAX(Y,Z))
#define _BULK_ELMS(V)		USHRT_MAX / V
#define _BULK_SETS(T,V)		T / V
#define APP_VARS	        APPLICATION_ARGS
#define PSI_VARS	        POWER_SIGNATURE_ARGS
#define NSI_VARS	        SIMPLE_SIGNATURE_ARGS
#define JOB_VARS	        JOB_ARGS
#define PER_VARS	        SIMPLE_PERIODIC_METRIC_ARGS
#define LOO_VARS			LOOP_ARGS
#define AGG_VARS			PERIODIC_AGGREGATION_ARGS
#define EVE_VARS			EAR_EVENTS_ARGS

#if DB_MYSQL || DB_PSQL

void init_db_helper(db_conf_t *conf);

/** Given an application, inserts it to the database currently selected */
int db_insert_application(application_t *application);

int db_insert_loop(loop_t *loop);

int db_insert_ear_event(ear_event_t *ear_ev);

int db_insert_gm_warning(gm_warning_t *warning);

int db_insert_power_signature(power_signature_t *pow_sig);

int db_insert_periodic_metric(periodic_metric_t *per_met);

int db_insert_periodic_aggregation(periodic_aggregation_t *per_agg);

int db_batch_insert_periodic_metrics(periodic_metric_t *per_mets, int num_mets);

int db_batch_insert_periodic_aggregations(periodic_aggregation_t *per_aggs, int num_aggs);

int db_batch_insert_ear_event(ear_event_t *ear_evs, int num_events);

int db_batch_insert_applications(application_t *applications, int num_apps);

int db_batch_insert_applications_learning(application_t *applications, int num_apps);

int db_batch_insert_applications_no_mpi(application_t *applications, int num_apps);

int db_batch_insert_loops(loop_t *loops, int num_loops);

/** Returns the accumulated energy (units depend on divisor, divisor=1 means mJ) for a given period.
*   The is_aggregated parameter indicates if the data is to be retrieved from the aggregated table 
*   or the individual one.*/
int db_select_acum_energy(int start_time, int end_time, ulong  divisor, char is_aggregated, uint *last_index, ulong *energy);

int db_select_acum_energy_idx(ulong divisor, char is_aggregated, uint *last_index, ulong *energy);

int db_select_acum_energy_nodes(int start_time, int end_time, ulong divisor, uint *last_index, ulong *energy, long num_nodes, char **nodes);

/** Reads applications from the normal DB or the learning DB depending on is_learning. It allocates 
*   memory for apps. Returns the number of applications readed */
int db_read_applications(application_t **apps,uint is_learning, int max_apps, char *node_name);

/** Reads from DB the number of applications that can be found in the corresponding tables. */
ulong get_num_applications(char is_learning, char *node_name);

int db_run_query(char *query, char *user, char *passw);

#if DB_MYSQL
MYSQL_RES *db_run_query_result(char *query);
#elif DB_PSQL
PGresult *db_run_query_result(char *query);

int get_num_rows(PGconn *connection, char *query);
#endif

float get_max_dc_power(char is_max, char *app_name, ulong freq);
float run_query_float_result(char *query);

int run_query_int_result(char *query);

int get_num_columns(char *query);

int db_read_applications_query(application_t **apps, char *query);

int db_read_loops_query(loop_t **loops, char *query);

void db_reset_counters();


/** Runs the received query to database, and stores the results as a string in results.
 *  the number of columns in each row is stored in num_columns, and the number of 
 *  rows in num_rows. Returns EAR_SUCCESS on completion, or EAR_ERROR if anything fails. */
int db_run_query_string_results(char *query, char ****results, int *num_columns, int *num_rows);

/** Frees the result of a db_run_query_string_results  * */
void db_free_results(char ***results, int num_cols, int num_rows);
#endif
