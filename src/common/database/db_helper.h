/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*   It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*   Copyright (C) 2017
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

#if DB_MYSQL
MYSQL *mysql_create_connection();
#elif DB_PSQL
PGconn *postgresql_create_connection();
#endif


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

int run_query_int_result(char *query);

int get_num_columns(char *query);

int db_read_applications_query(application_t **apps, char *query);

void db_reset_counters();

#endif
