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

#include <common/config.h>
#if DB_MYSQL
#include <common/types/log.h>
#include <common/types/job.h>
#include <common/types/loop.h>
#include <common/types/signature.h>
#include <common/types/gm_warning.h>
#include <common/types/application.h>
#include <common/types/periodic_metric.h>
#include <common/types/power_signature.h>
#include <common/types/periodic_aggregation.h>

#include <mysql/mysql.h>

//
#define EAR_TYPE_APPLICATION    1
#define EAR_TYPE_LOOP           2

//number of arguments inserted into periodic_metrics
#define PERIODIC_AGGREGATION_ARGS   4
#define EAR_EVENTS_ARGS             6
#define POWER_SIGNATURE_ARGS        9
#define APPLICATION_ARGS            5
#define LOOP_ARGS                   8
#define JOB_ARGS                    16

#define PERIODIC_METRIC_ARGS        10

#if USE_GPUS
#define GPU_SIGNATURE_ARGS          5
#endif

#if !DB_SIMPLE
#define SIGNATURE_ARGS              23
#define AVG_SIGNATURE_ARGS          24
#else
#define SIGNATURE_ARGS              13
#define AVG_SIGNATURE_ARGS          14
#endif





typedef struct
{
    char type;
    application_t *app;
    loop_t *loop;
} signature_container_t;

/** Sets the database layer to operate with full signatures or simplified one. */
void set_signature_simple(char full_sig);

/** Sets the database layer to operate with normal periodic_metrics or exteded ones. */
void set_node_detail(char node_det);

/** Sets the database layer to operate with full periodic_metrics or simplified one. */
void set_periodic_metrics_simple(char full_periodic);

/** Given a MYSQL connection and an application, inserts said application into
*   the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_application(MYSQL *connection, application_t *app);

/** Given a MYSQL connection and an array of applications, insert said applications
*   into the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR
*   or EAR_MYSQL_STMT_ERROR on error. */ 
int mysql_batch_insert_applications(MYSQL *connection, application_t *apps, int num_apps);


/** Given a MYSQL connection and an array of applications, insert said applications
*   into the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR
*   or EAR_MYSQL_STMT_ERROR on error. Used for non-mpi applications. */ 
int mysql_batch_insert_applications_no_mpi(MYSQL *connection, application_t *apps, int num_apps);

/** Given a MYSQL connection and a valid MYSQL query, stores in apps the 
*   applications found in the database corresponding to the query. Returns the 
*   number of applications found on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error. */
int mysql_retrieve_applications(MYSQL *connection, char *query, application_t **apps, char is_learning);

/** Given a MYSQL connection and a loop, inserts said loop into
*   the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_loop(MYSQL *connection, loop_t *loop);

/** Given a MYSQL connection and an array of loops, inserts said loops into
*   the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error.*/
int mysql_batch_insert_loops(MYSQL *connection, loop_t *loop, int num_loops);

/** Given a MYSQL connection and a valid MYSQL query, stores in loops the 
*   loops found in the database corresponding to the query. Returns the 
*   number of loops found on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error. */
int mysql_retrieve_loops(MYSQL *connection, char *query, loop_t **loops);

/** Given a MYSQL connection and a job, inserts said job into
*   the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_job(MYSQL *connection, job_t *job, char is_learning);

/** Given a MYSQL connection and an array of applications, insert said applications' jobs
*   into the database. Returns EAR_SUCCESS on success, and either EAR_MYSQL_ERROR
*   or EAR_MYSQL_STMT_ERROR on error.  */ 
int mysql_batch_insert_jobs(MYSQL *connection, application_t *app, int num_apps);

/** Given a MYSQL connection and a valid MYSQL query, stores in jobs the 
*   jobs found in the database corresponding to the query. Returns the 
*   number of jobs found on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error. */
int mysql_retrieve_jobs(MYSQL *connection, char *query, job_t **jobs);


/** Given a MYSQL connection and a valid MYSQL query, stores in applications the 
*   applications found in the database corresponding to the query. It looks in learning
*   or "normal" tables depending on is_learning. Returns the 
*   number of applications found on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error. */
int mysql_retrieve_applications(MYSQL *connection, char *query, application_t **apps, char is_learning);

/** Given a MYSQL connection and a signature, inserts said signature into
*   the database. Returns the signature's database id on success, and either 
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_signature(MYSQL *connection, signature_t *sig, char is_learning);

/** Given a MYSQL connection and an array of applications, inserts said application's
*   signatures into the database. Returns the first signature's database id on 
*   success, and either EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_batch_insert_signatures(MYSQL *connection, signature_container_t cont, char is_learning, int num_sigs);

/** Given a MYSQL connection and a valid MYSQL query, stores in sigs the 
*   signatures found in the database corresponding to the query. Returns the 
*   number of signatures found on success, and either EAR_MYSQL_ERROR or
*   EAR_MYSQL_STMT_ERROR on error. */
int mysql_retrieve_signatures(MYSQL *connection, char *query, signature_t **sigs);

/** Given a MYSQL connection and a power_signature, inserts said power_signature into
*   the database. Returns the power_signature's database id on success, and either 
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_power_signature(MYSQL *connection, power_signature_t *pow_sig);

/** Given a MYSQL connection and an array of aplications , inserts said application's
*   power_signatures into the database. Returns the first power_signature's database
*   id on success, and either EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_batch_insert_power_signatures(MYSQL *connection, application_t *pow_sig, int num_sigs);

/** Given a MYSQL connection and a periodic_metric, inserts said periodic_metric into
*   the database. Returns the periodic_metric's database id on success, and either 
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_periodic_metric(MYSQL *connection, periodic_metric_t *per_met);

/** Given a MYSQL connection and num_mets periodic_metrics, inserts said 
*   periodic_metrics into the database. Returns EAR_SUCCESS on success, and either
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error. */
int mysql_batch_insert_periodic_metrics(MYSQL *connection, periodic_metric_t *per_mets, int num_mets);

/** Given a MYSQL connection and a periodic_aggregation, inserts said periodic_aggregation
*   into the database. Returns the periodic_aggregation's database id on success, and 
*   either EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_periodic_aggregation(MYSQL *connection, periodic_aggregation_t *per_agg);

/** Given a MYSQL connection and num_aggs periodic_aggregations, inserts said 
*   periodic_agregations into the database. Returns EAR_SUCCESS on success, and either
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error. */
int mysql_batch_insert_periodic_aggregations(MYSQL *connection, periodic_aggregation_t *per_aggs, int num_aggs);

/** Given a MYSQL connection and an EAR event, inserts said event into
*   the database. Returns the event's database id on success, and either 
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_ear_event(MYSQL *connection, ear_event_t *ear_ev);

/** Given a MYSQL connection and num_evs EAR events, inserts said events into
*   the database. Returns the EAR_SUCCESS success, and either 
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_batch_insert_ear_events(MYSQL *connection, ear_event_t *ear_ev, int num_evs);

/** Given a MYSQL connection and an global manager warning, inserts said 
*   warning into the database. Returns EAR_SUCCESS on success, and either 
*   EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_insert_gm_warning(MYSQL *connection, gm_warning_t *warning);

/** Given a MYSQL connection and num_sigs applications, inserts the application's signatures
*   to the database using a query to calculate the moving average of all the signatures in 
*   a job. Returns EAR_SUCCESS on succkess, and either EAR_MYSQL_ERROR
*   or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_batch_insert_avg_signatures(MYSQL *connection, application_t *app, int num_sigs);

#if USE_GPUS
/** Given a MYSQL connection and an array of applications or loops, inserts said application's
*   signatures into the database. Returns the first signature's database id on 
*   success, and either EAR_MYSQL_ERROR or EAR_MYSQL_STMT_ERROR on error.*/
int mysql_batch_insert_gpu_signatures(MYSQL *connection, signature_container_t cont, int num_sigs);

/** Given a MYSQL connection and a query, retrieves the gpu signatures corresponding to that query */
int mysql_retrieve_gpu_signatures(MYSQL *connection, char *query, gpu_signature_t *gpu_sig);
#endif

/** PENDING */
int mysql_statement_error(MYSQL_STMT *statement);

/** Given a MYSQL connection and a query, retrieves the pwoer_signatures corresponding to that query */
int mysql_retrieve_power_signatures(MYSQL *connection, char *query, power_signature_t **pow_sigs);


#endif
