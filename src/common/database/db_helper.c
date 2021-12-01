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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/string_enhanced.h>
#include <common/database/db_helper.h>
#if DB_MYSQL
#include <common/database/mysql_io_functions.h>
#elif DB_PSQL
#include <common/database/postgresql_io_functions.h>
#endif

db_conf_t *db_config = NULL;
int current_step_id = -1;
int current_job_id = 0;

#define _MAX(X,Y)			(X > Y ? X : Y)
#define _MMAAXX(W,X,Y,Z) 	(_MAX(W,X) > _MAX(Y,Z) ? _MAX(W,X) : _MAX(Y,Z))
#define _BULK_ELMS(V)		USHRT_MAX / V
#define _BULK_SETS(T,V)		T / V
#define APP_VARS	        APPLICATION_ARGS
#define PSI_VARS	        POWER_SIGNATURE_ARGS
#define NSI_VARS	        SIGNATURE_ARGS
#define JOB_VARS	        JOB_ARGS
#define PER_VARS	        PERIODIC_METRIC_ARGS
#define LOO_VARS			LOOP_ARGS
#define AGG_VARS			PERIODIC_AGGREGATION_ARGS
#define EVE_VARS			EAR_EVENTS_ARGS
#define IS_SIG_FULL_QUERY   "SELECT COUNT(*) FROM information_schema.columns where TABLE_NAME='Signatures' AND TABLE_SCHEMA='%s'"
#define IS_NODE_FULL_QUERY  "SELECT COUNT(*) FROM information_schema.columns where TABLE_NAME='Periodic_metrics' AND TABLE_SCHEMA='%s'"


#define PAINT(N) \
	verbose(VDBH, "Elements to insert %d", N); \
	verbose(VDBH, "Bulk elements %d", bulk_elms); \
	verbose(VDBH, "Bulk sets %d", bulk_sets)

void init_db_helper(db_conf_t *conf)
{
    db_config = conf;
    char query[256];
    int num_sig_args, num_per_args;

#if DB_MYSQL
    sprintf(query, IS_SIG_FULL_QUERY, db_config->database);
    num_sig_args = run_query_int_result(query);
#elif DB_PSQL
    strcpy(query, "SELECT * FROM Signatures LIMIT 5");
    num_sig_args = get_num_columns(query);
#endif
    if (num_sig_args == EAR_ERROR)
        set_signature_simple(db_config->report_sig_detail);
    else if (num_sig_args < 20)
        set_signature_simple(0);
    else 
        set_signature_simple(1);


#if DB_MYSQL
    sprintf(query, IS_NODE_FULL_QUERY, db_config->database);
    num_per_args = run_query_int_result(query);
#elif DB_PSQL
    strcpy(query, "SELECT * FROM Periodic_metrics LIMIT 5");
    num_per_args = get_num_columns(query);
#endif
    if (num_per_args == EAR_ERROR)
        set_node_detail(db_config->report_node_detail);
    else if (num_per_args < 8)
        set_node_detail(0);
    else
        set_node_detail(1);

}

#if DB_MYSQL
float run_query_float_result(char *query)
{
    MYSQL_RES *result = db_run_query_result(query);
    float num_indexes;
    if (result == NULL) {
        verbose(VDBH, "Error while retrieving result");
        return EAR_ERROR;
    } else {
        MYSQL_ROW row;
        unsigned int num_fields;
        unsigned int i;

        num_fields = mysql_num_fields(result);
        while ((row = mysql_fetch_row(result)))
        {
            mysql_fetch_lengths(result);
            for(i = 0; i < num_fields; i++)
                num_indexes = atof(row[i]);
        }
    }
    return num_indexes;
}

int run_query_int_result(char *query)
{
    MYSQL_RES *result = db_run_query_result(query);
    int num_indexes;
    if (result == NULL) {
        verbose(VDBH, "Error while retrieving result");
        return EAR_ERROR;
    } else {
        MYSQL_ROW row;
        unsigned int num_fields;
        unsigned int i;

        num_fields = mysql_num_fields(result);
        while ((row = mysql_fetch_row(result)))
        {
            mysql_fetch_lengths(result);
            for(i = 0; i < num_fields; i++)
                num_indexes = atoi(row[i]);
        }
    }
    return num_indexes;
}
#elif DB_PSQL
int get_num_columns(char *query)
{
    PGconn *connection = postgresql_create_connection();

    if (connection == NULL)
    {
        verbose(VDBH, "ERROR connecting to database");
        return EAR_ERROR;
    }

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) //-> if it returned data
    {
        PQclear(res);
        return EAR_ERROR;
    }
   
    int num_columns = PQnfields(res);
    PQclear(res);

    return num_columns;

}
#endif

#if DB_PSQL
int get_num_rows(PGconn *connection, char *query)
{
    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) //-> if it returned data
    {
        PQclear(res);
        return EAR_ERROR;
    }
   
    int num_rows = PQntuples(res);
    PQclear(res);

    return num_rows;

}
#endif

#if DB_MYSQL
MYSQL *mysql_create_connection()
{
    MYSQL *connection = mysql_init(NULL);

    if (connection == NULL)
    {
        verbose(VDBH, "ERROR creating MYSQL object.");
        return NULL;
    }

    if (db_config == NULL)
    {
        verbose(VDBH, "Database configuration not initialized.");
        return NULL;
    }

    unsigned int time_out = 20;
    mysql_options(connection,MYSQL_OPT_CONNECT_TIMEOUT,(char*)&time_out);
    if (!mysql_real_connect(connection, db_config->ip, db_config->user, db_config->pass, db_config->database, db_config->port, NULL, 0))
    {
        verbose(VDBH, "ERROR connecting to the database: %s", mysql_error(connection));
        mysql_close(connection);
        return NULL;
    }

    return connection;
}
#endif

#if DB_PSQL
PGconn *postgresql_create_connection()
{
    char temp[32];
    char **keys, **values;
    PGconn *connection;

    sprintf(temp, "%d", db_config->port);
    strtolow(db_config->database);

    keys = calloc(4, sizeof(char *));
    values = calloc(4, sizeof(char*));

    keys[0] = "dbname";
    keys[1] = "user";
    keys[2] = "password";
    keys[3] = "host";

    values[0] = db_config->database;
    values[1] = db_config->user;
    values[2] = db_config->pass;
    values[3] = db_config->ip;

#if 0
    if (db_config->port > 0)
        connection = PQsetdbLogin(db_config->ip, temp, NULL,NULL, db_config->database, db_config->user, db_config->pass);
    else
        connection = PQsetdbLogin(db_config->ip, NULL, NULL,NULL, db_config->database, db_config->user, db_config->pass);
#endif

    connection = PQconnectdbParams((const char * const *)keys, (const char * const *)values, 0);

    free(keys);
    free(values);

    if (PQstatus(connection) != CONNECTION_OK)
    {
        verbose(VDBH, "ERROR connecting to the database: %s", PQerrorMessage(connection));
        PQfinish(connection);
        return NULL;
    }

    return connection;

}
#endif

int db_insert_application(application_t *application)
{
#if DB_MYSQL
    MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
    PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_application(connection, application) < 0)
#elif DB_PSQL
    if (postgresql_insert_application(connection, application) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing application to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;

}

int db_batch_insert_applications(application_t *applications, int num_apps)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(num_apps, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
    PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
#if DB_MYSQL
    	if (mysql_batch_insert_applications(connection, &applications[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
        	return EAR_ERROR;
    	}
#elif DB_PSQL
        if (postgresql_batch_insert_applications(connection, &applications[e], bulk_elms) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_apps)
	{
#if DB_MYSQL
		if (mysql_batch_insert_applications(connection, &applications[e], num_apps - e) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
            return EAR_ERROR;
        }
#elif DB_PSQL
        if (postgresql_batch_insert_applications(connection, &applications[e], num_apps - e) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
#endif
	}	 

	//
#if DB_MYSQL
	mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif

    return EAR_SUCCESS;
}

int db_batch_insert_applications_learning(application_t *applications, int num_apps)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(num_apps, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
#if DB_MYSQL
    	if (mysql_batch_insert_applications(connection, &applications[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
        	return EAR_ERROR;
    	}
#elif DB_PSQL
    	if (postgresql_batch_insert_applications(connection, &applications[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
        	return EAR_ERROR;
    	}
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_apps)
	{
#if DB_MYSQL
		if (mysql_batch_insert_applications(connection, &applications[e], num_apps - e) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
            return EAR_ERROR;
        }
#elif DB_PSQL
		if (postgresql_batch_insert_applications(connection, &applications[e], num_apps - e) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
#endif
	}	 

	//
#if DB_MYSQL
	mysql_close(connection);
#elif DB_PSQL
	PQfinish(connection);
#endif

    return EAR_SUCCESS;
}


int db_batch_insert_applications_no_mpi(application_t *applications, int num_apps)
{
	int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(num_apps, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

	// Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
	{
#if DB_MYSQL
		if (mysql_batch_insert_applications_no_mpi(connection, &applications[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_applications_no_mpi(connection, &applications[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_apps)
	{
#if DB_MYSQL
		if (mysql_batch_insert_applications_no_mpi(connection, &applications[e], num_apps - e) < 0) {
			verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_applications_no_mpi(connection, &applications[e], num_apps - e) < 0) {
			verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;

}

int db_insert_loop(loop_t *loop)
{    

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_loop(connection, loop) < 0)
#elif DB_PSQL
    if (postgresql_insert_loop(connection, loop) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing loop signature to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_batch_insert_loops(loop_t *loops, int num_loops)
{
	int bulk_elms = _BULK_ELMS(_MAX(LOO_VARS, NSI_VARS));
	int bulk_sets = _BULK_SETS(num_loops, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

	// Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
	{
#if DB_MYSQL
		if (mysql_batch_insert_loops(connection, &loops[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing loop signature to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_loops(connection, &loops[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing loop signature to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_loops)
	{
#if DB_MYSQL
		if (mysql_batch_insert_loops(connection, &loops[e], num_loops - e) < 0) {
			verbose(VDBH, "ERROR while batch writing loop signature to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_loops(connection, &loops[e], num_loops - e) < 0) {
			verbose(VDBH, "ERROR while batch writing loop signature to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_insert_power_signature(power_signature_t *pow_sig)
{
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_power_signature(connection, pow_sig) < 0)
#elif DB_PSQL
    if (postgresql_insert_power_signature(connection, pow_sig) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing power_signature to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_insert_periodic_aggregation(periodic_aggregation_t *per_agg)
{
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_periodic_aggregation(connection, per_agg) < 0)
#elif DB_PSQL
    if (postgresql_insert_periodic_aggregation(connection, per_agg) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing periodic_aggregation to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_insert_periodic_metric(periodic_metric_t *per_met)
{
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_periodic_metric(connection, per_met) < 0)
#elif DB_PSQL
    if (postgresql_insert_periodic_metric(connection, per_met) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing periodic_metric to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_batch_insert_periodic_metrics(periodic_metric_t *per_mets, int num_mets)
{
	int bulk_elms = _BULK_ELMS(PER_VARS);
	int bulk_sets = _BULK_SETS(num_mets, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

	// Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
	{
#if DB_MYSQL
		if (mysql_batch_insert_periodic_metrics(connection, &per_mets[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing periodic metrics to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_periodic_metrics(connection, &per_mets[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing periodic metrics to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_mets)
	{
#if DB_MYSQL
		if (mysql_batch_insert_periodic_metrics(connection, &per_mets[e], num_mets - e) < 0) {
			verbose(VDBH, "ERROR while batch writing periodic metrics to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_periodic_metrics(connection, &per_mets[e], num_mets - e) < 0) {
			verbose(VDBH, "ERROR while batch writing periodic metrics to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}

#if DB_MYSQL	
	mysql_close(connection);
#elif DB_PSQL
	PQfinish(connection);
#endif

    return EAR_SUCCESS;
}

int db_batch_insert_periodic_aggregations(periodic_aggregation_t *per_aggs, int num_aggs)
{
	int bulk_elms = _BULK_ELMS(AGG_VARS);
	int bulk_sets = _BULK_SETS(num_aggs, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

	// Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
	{
#if DB_MYSQL
		if (mysql_batch_insert_periodic_aggregations(connection, &per_aggs[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing aggregations to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_periodic_aggregations(connection, &per_aggs[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing aggregations to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_aggs)
	{
#if DB_MYSQL
		if (mysql_batch_insert_periodic_aggregations(connection, &per_aggs[e], num_aggs - e) < 0) {
			verbose(VDBH, "ERROR while batch writing aggregations to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_periodic_aggregations(connection, &per_aggs[e], num_aggs - e) < 0) {
			verbose(VDBH, "ERROR while batch writing aggregations to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif

    return EAR_SUCCESS;
}

int db_insert_ear_event(ear_event_t *ear_ev)
{
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_ear_event(connection, ear_ev) < 0)
#elif DB_PSQL
    if (postgresql_insert_ear_event(connection, ear_ev) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing ear_event to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_batch_insert_ear_event(ear_event_t *ear_evs, int num_events)
{
	int bulk_elms = _BULK_ELMS(EVE_VARS);
	int bulk_sets = _BULK_SETS(num_events, bulk_elms);
	int e, s;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

	// Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
	{
#if DB_MYSQL
		if (mysql_batch_insert_ear_events(connection, &ear_evs[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing ear_event to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_ear_events(connection, &ear_evs[e], bulk_elms) < 0) {
			verbose(VDBH, "ERROR while batch writing ear_event to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < num_events)
	{
#if DB_MYSQL
		if (mysql_batch_insert_ear_events(connection, &ear_evs[e], num_events - e) < 0) {
			verbose(VDBH, "ERROR while batch writing ear_event to database.");
            mysql_close(connection);
			return EAR_ERROR;
		}
#elif DB_PSQL
		if (postgresql_batch_insert_ear_events(connection, &ear_evs[e], num_events - e) < 0) {
			verbose(VDBH, "ERROR while batch writing ear_event to database.");
            PQfinish(connection);
			return EAR_ERROR;
		}
#endif
	}

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

int db_insert_gm_warning(gm_warning_t *warning)
{
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    if (mysql_insert_gm_warning(connection, warning) < 0)
#elif DB_PSQL
    if (postgresql_insert_gm_warning(connection, warning) < 0)
#endif
    {
        verbose(VDBH, "ERROR while writing gm_warning to database.");
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
        return EAR_ERROR;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
    
    return EAR_SUCCESS;
}

#if DB_MYSQL
int stmt_error(MYSQL *connection, MYSQL_STMT *statement)
{
    verbose(VMYSQL, "Error preparing statement (%d): %s\n",
            mysql_stmt_errno(statement), mysql_stmt_error(statement));
    mysql_stmt_close(statement);
    mysql_close(connection);
    return EAR_ERROR;
}
#endif

#define PSQL_METRICS_SUM_QUERY       "SELECT SUM(DC_energy)/%lu DIV 1, MAX(id) FROM Periodic_metrics WHERE end_time" \
                                     ">= %d AND end_time <= %d"
#define PSQL_AGGREGATED_SUM_QUERY    "SELECT SUM(DC_energy)/%lu DIV 1, MAX(id) FROM Periodic_aggregations WHERE end_time"\
                                     ">= %d AND end_time <= %d"

#if DB_PSQL
int postgresql_select_acum_energy(PGconn *connection, int start_time, int end_time, ulong divisor, char is_aggregated, uint *last_index, ulong *energy)
{
    char query[256];

    if (is_aggregated)
        sprintf(query, PSQL_AGGREGATED_SUM_QUERY, divisor, start_time, end_time);
    else
        sprintf(query, PSQL_METRICS_SUM_QUERY, divisor, start_time, end_time);

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
       verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }


    *energy = htonl( *((uint *)PQgetvalue(res, 0, 0)));
    *last_index = htonl( *((uint *)PQgetvalue(res, 0, 1)));
    
    PQclear(res);
    return EAR_SUCCESS;
}
#endif

#define METRICS_SUM_QUERY       "SELECT SUM(DC_energy)/? DIV 1, MAX(id) FROM Periodic_metrics WHERE end_time" \
                                ">= ? AND end_time <= ?"
#define AGGREGATED_SUM_QUERY    "SELECT SUM(DC_energy)/? DIV 1, MAX(id) FROM Periodic_aggregations WHERE end_time"\
                                ">= ? AND end_time <= ?"

#if DB_MYSQL
int mysql_select_acum_energy(MYSQL *connection, int start_time, int end_time, ulong divisor, char is_aggregated, uint *last_index, ulong *energy)
{

    char query[256];
    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement)
    {
        verbose(VDBH, "Error creating statement (%d): %s\n", mysql_errno(connection),
                mysql_error(connection));
        mysql_close(connection);
        return EAR_ERROR;
    }

    if (is_aggregated)
    {
        if (mysql_stmt_prepare(statement, AGGREGATED_SUM_QUERY, strlen(AGGREGATED_SUM_QUERY)))
            return stmt_error(connection, statement);
    }
    else
    {
        sprintf(query, METRICS_SUM_QUERY);
        if (mysql_stmt_prepare(statement, query, strlen(query)))
            return stmt_error(connection, statement);
    }

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
    MYSQL_BIND res_bind[2];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = res_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    res_bind[0].buffer = energy;
    res_bind[1].buffer = last_index;

    if (mysql_stmt_bind_param(statement, bind)) return stmt_error(connection, statement);
    if (mysql_stmt_bind_result(statement, res_bind)) return stmt_error(connection, statement);
    if (mysql_stmt_execute(statement)) return stmt_error(connection, statement);
    if (mysql_stmt_store_result(statement)) return stmt_error(connection, statement);

    int status = mysql_stmt_fetch(statement);
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
    {
        return stmt_error(connection, statement);
    }

    mysql_stmt_close(statement);
    return EAR_SUCCESS;

}
#endif

int db_select_acum_energy(int start_time, int end_time, ulong divisor, char is_aggregated, uint *last_index, ulong *energy)
{
    *energy = 0;
    int ret = EAR_ERROR;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
    PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    ret = mysql_select_acum_energy(connection, start_time, end_time, divisor, is_aggregated, last_index, energy);
    mysql_close(connection);
#elif DB_PSQL
    ret = postgresql_select_acum_energy(connection, start_time, end_time, divisor, is_aggregated, last_index, energy);
    PQfinish(connection);
#endif

    return ret;

}

#if 0
#define METRICS_SUM_QUERY       "SELECT SUM(DC_energy)/? DIV 1, MAX(id) FROM Periodic_metrics WHERE end_time" \
                                ">= ? AND end_time <= ? AND DC_energy <= %d"
#define AGGREGATED_SUM_QUERY    "SELECT SUM(DC_energy)/? DIV 1, MAX(id) FROM Periodic_aggregations WHERE end_time"\
                                ">= ? AND end_time <= ?"
#endif

#if DB_MYSQL
int db_select_acum_energy_nodes(int start_time, int end_time, ulong divisor, uint *last_index, ulong *energy, long num_nodes, char **nodes)
{
    char *query;
    int i, total_length=0;

    for (i = 0; i < num_nodes; i++)
        total_length += (strlen(nodes[i]) + 8); //8 comes from (''), and spaces in the query for each node

    query = calloc(total_length, sizeof(char));

    *energy = 0;
	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement)
    {
        verbose(VDBH, "Error creating statement (%d): %s\n", mysql_errno(connection),
                mysql_error(connection));
        mysql_close(connection);
        return EAR_ERROR;
    }

    sprintf(query, METRICS_SUM_QUERY);
    if (num_nodes > 0)
    {
        strcat(query, " AND node_id IN (");
        for (i = 0; i < num_nodes; i++)
        {
            strcat(query, "'");
            strcat(query, nodes[i]);
            i == num_nodes - 1 ? strcat(query, ")'") : strcat(query, "', ");
        }

    }



    if (mysql_stmt_prepare(statement, query, strlen(query)))
        return stmt_error(connection, statement);

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
    MYSQL_BIND res_bind[2];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = res_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    res_bind[0].buffer = energy;
    res_bind[1].buffer = last_index;

    if (mysql_stmt_bind_param(statement, bind)) return stmt_error(connection, statement);
    if (mysql_stmt_bind_result(statement, res_bind)) return stmt_error(connection, statement);
    if (mysql_stmt_execute(statement)) return stmt_error(connection, statement);
    if (mysql_stmt_store_result(statement)) return stmt_error(connection, statement);

    int status = mysql_stmt_fetch(statement);
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
    {
        free(query);
        return stmt_error(connection, statement);
    }

    mysql_stmt_close(statement);
    mysql_close(connection);
    free(query);

    return EAR_SUCCESS;

}
#endif

#define METRICS_ID_SUM_QUERY       "SELECT SUM(DC_energy)/? DIV 1, MAX(id) FROM Periodic_metrics WHERE " \
                                "id > %d"
#define AGGREGATED_ID_SUM_QUERY    "SELECT SUM(DC_energy)/? DIV 1, MAX(id) FROM Periodic_aggregations WHERE "\
                                "id > %d"

#if DB_MYSQL
int mysql_select_acum_energy_idx(MYSQL *connection, ulong divisor, char is_aggregated, uint *last_index, ulong *energy)
{
    char query[256];
    *energy = 0;

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement)
    {
        verbose(VDBH, "Error creating statement (%d): %s\n", mysql_errno(connection),
                mysql_error(connection));
        mysql_close(connection);
        return EAR_ERROR;
    }

    if (is_aggregated)
    {
        sprintf(query, AGGREGATED_ID_SUM_QUERY, *last_index);
        if (mysql_stmt_prepare(statement, query, strlen(query)))
            return stmt_error(connection, statement);
    }
    else
    {
        sprintf(query, METRICS_ID_SUM_QUERY, *last_index);
        if (mysql_stmt_prepare(statement, query, strlen(query)))
            return stmt_error(connection, statement);
    }
    //Query parameters binding
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = 1;

    bind[0].buffer = (char *)&divisor;

    //Result parameters
    MYSQL_BIND res_bind[2];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = res_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    res_bind[0].buffer = energy;
    res_bind[1].buffer = last_index;

    if (mysql_stmt_bind_param(statement, bind)) return stmt_error(connection, statement);
    if (mysql_stmt_bind_result(statement, res_bind)) return stmt_error(connection, statement);
    if (mysql_stmt_execute(statement)) return stmt_error(connection, statement);
    if (mysql_stmt_store_result(statement)) return stmt_error(connection, statement);

    int status = mysql_stmt_fetch(statement);
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
    {
        return stmt_error(connection, statement);
    }

    mysql_stmt_close(statement);

    return EAR_SUCCESS;

}
#endif

#define PSQL_METRICS_ID_SUM_QUERY       "SELECT SUM(DC_energy)/%lu DIV 1, MAX(id) FROM Periodic_metrics WHERE " \
                                        "id > %d "
#define PSQL_AGGREGATED_ID_SUM_QUERY    "SELECT SUM(DC_energy)/%lu DIV 1, MAX(id) FROM Periodic_aggregations WHERE "\
                                        "id > %d"

#if DB_PSQL
int postgresql_select_acum_energy_idx(PGconn *connection, ulong divisor, char is_aggregated, uint *last_index, ulong *energy)
{
    char query[256];

    if (is_aggregated)
        sprintf(query, PSQL_AGGREGATED_ID_SUM_QUERY, divisor, *last_index);
    else
        sprintf(query, PSQL_METRICS_ID_SUM_QUERY, divisor, *last_index);

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
       verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }


    *energy = htonl( *((uint *)PQgetvalue(res, 0, 0)));
    *last_index = htonl( *((uint *)PQgetvalue(res, 0, 1)));
    
    PQclear(res);
    return EAR_SUCCESS;
}
#endif

int db_select_acum_energy_idx(ulong divisor, char is_aggregated, uint *last_index, ulong *energy)
{
    *energy = 0;
    int ret = EAR_ERROR;

#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
    PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
    ret = mysql_select_acum_energy_idx(connection, divisor, is_aggregated, last_index, energy);
    mysql_close(connection);
#elif DB_PSQL
    ret = postgresql_select_acum_energy_idx(connection, divisor, is_aggregated, last_index, energy);
    PQfinish(connection);
#endif

    return ret;

}

int db_read_loops_query(loop_t **loops, char *query)
{
    int num_loops = 0;
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
    PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
   	num_loops = mysql_retrieve_loops(connection, query, loops);
#elif DB_PSQL
    num_loops = postgresql_retrieve_loops(connection, query, loops);
#endif
    
   
  	if (num_loops == EAR_MYSQL_ERROR){
#if DB_MYSQL
        verbose(VDBH, "Error retrieving information from database (%d): %s\n", mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
		return num_loops;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif

    return num_loops;
}

int db_read_applications_query(application_t **apps, char *query)
{
    int num_apps = 0;
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
    PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

#if DB_MYSQL
   	num_apps = mysql_retrieve_applications(connection, query, apps, 0);
#elif DB_PSQL
   	num_apps = postgresql_retrieve_applications(connection, query, apps, 0);
#endif
    
   
  	if (num_apps == EAR_MYSQL_ERROR){
#if DB_MYSQL
        verbose(VDBH, "Error retrieving information from database (%d): %s\n", mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
		return num_apps;
    }

#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif

    return num_apps;
}

int db_run_query(char *query, char *user, char *passw)
{

    if (db_config == NULL)
    {
        verbose(VDBH, "Database configuration not initialized.");
		return EAR_MYSQL_ERROR;
    }
    if (user == NULL)
        user = db_config->user;
    if (passw == NULL)
        passw = db_config->pass;

#if DB_MYSQL
    MYSQL *connection = mysql_init(NULL);

    if (connection == NULL)
    {
        verbose(VDBH, "Error creating MYSQL object: %s \n", mysql_error(connection));
        exit(1);
    }

    if (!mysql_real_connect(connection, db_config->ip, user, passw, db_config->database, db_config->port, NULL, 0))
    {
        verbose(VDBH, "Error connecting to the database(%d):%s\n", mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
		return EAR_MYSQL_ERROR;
    }

    if (mysql_query(connection, query))
    {
        verbose(VDBH, "Error when executing query(%d): %s\n", mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
        return EAR_MYSQL_ERROR;
    }

    mysql_close(connection);

#elif DB_PSQL
    strcpy(db_config->user, user);
    strcpy(db_config->pass, passw);
    PGconn *connection = postgresql_create_connection();
    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        verbose(VDBH, "%s\n", PQresultErrorMessage(res));
        PQclear(res);
        PQfinish(connection);
        return EAR_MYSQL_ERROR;
    }
    PQclear(res);
    PQfinish(connection);
    
#endif
    return EAR_SUCCESS;
}

#if DB_MYSQL
MYSQL_RES *db_run_query_result(char *query)
{
	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return NULL;
    }

    if (mysql_query(connection, query))
    {
        verbose(VDBH, "Error when executing query(%d): %s\n", mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
        return NULL;
    }

    MYSQL_RES *result;
    result = mysql_store_result(connection);

    mysql_close(connection);

    return result;
}

#elif DB_PSQL
PGresult *db_run_query_result(char *query)
{
	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return NULL;
    }

    PGresult *result;
    result = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 0); 
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        verbose(VDBH, "Error when executing query: %s\n", PQerrorMessage(connection));
        PQfinish(connection);
        return NULL;
    }

    PQfinish(connection);

    return result;
}
#endif

void db_reset_counters()
{
    current_job_id = 0;
    current_step_id = -1;
}

int db_read_applications(application_t **apps,uint is_learning, int max_apps, char *node_name)
{
    int num_apps = 0;
#if DB_MYSQL
	MYSQL *connection = mysql_create_connection();
#elif DB_PSQL
	PGconn *connection = postgresql_create_connection();
#endif

    if (connection == NULL) {
        return EAR_ERROR;
    }

    if (max_apps < 1)
    {
        verbose(VDBH, "ERROR: querying less than 1 app is not possible (%d requested).\n", max_apps);
        return EAR_ERROR;
    }

    char query[512];
    if (is_learning && node_name != NULL)
    {
/*        sprintf(query,  "SELECT Learning_applications.* FROM Learning_applications INNER JOIN "\
                        "Learning_jobs ON job_id = id where job_id < (SELECT max(id) FROM (SELECT (id) FROM "\
                        "Learning_jobs WHERE id > %d ORDER BY id asc limit %u) as t1)+1 and "\
                        "job_id > %d AND node_id='%s' GROUP BY job_id, step_id", current_job_id, max_apps, current_job_id, node_name); */
        sprintf(query,  "SELECT Learning_applications.* FROM Learning_applications WHERE (job_id > %d AND node_id='%s') OR "\
                        "(job_id = %d AND step_id > %d AND node_id = '%s') ORDER BY job_id LIMIT %d", 
                        current_job_id, node_name, current_job_id, current_step_id, node_name, max_apps);
    }
    else if (is_learning && node_name == NULL)
    {
        sprintf(query,  "SELECT Learning_applications.* FROM Learning_applications INNER JOIN "\
                        "Learning_jobs ON job_id = id where job_id < (SELECT max(id) FROM (SELECT (id) FROM "\
                        "Learning_jobs WHERE id > %d ORDER BY id asc limit %u) as t1)+1 and "\
                        "job_id > %d GROUP BY job_id, step_id", current_job_id, max_apps, current_job_id);
    }
    else if (!is_learning && node_name != NULL)
    {
/*        sprintf(query,  "SELECT Applications.* FROM Applications INNER JOIN "\
                        "Jobs ON job_id = id where job_id < (SELECT max(id) FROM (SELECT (id) FROM "\
                        "Jobs WHERE id > %d ORDER BY id asc limit %u) as t1)+1 and "\
                        "job_id > %d AND node_id='%s' GROUP BY job_id, step_id", current_job_id, max_apps, current_job_id, node_name); */
        sprintf(query,  "SELECT Applications.* FROM Applications INNER JOIN Signatures ON signature_id = Signatures.id WHERE (job_id > %d AND node_id='%s') OR "\
                        "(job_id = %d AND step_id > %d AND node_id = '%s') AND  "\
                        " time > 60 AND DC_power > 100 AND DC_power < 1000 ORDER BY job_id LIMIT %d", 
                        current_job_id, node_name, current_job_id, current_step_id, node_name, max_apps);
    }
    else
    {
        sprintf(query,  "SELECT Applications.* FROM Applications INNER JOIN "\
                        "Jobs ON job_id = id where job_id < (SELECT max(id) FROM (SELECT (id) FROM "\
                        "Jobs WHERE id > %d ORDER BY id asc limit %u) as t1)+1 and "\
                        "job_id > %d GROUP BY job_id, step_id", current_job_id, max_apps, current_job_id);
    }


#if DB_MYSQL
   	num_apps = mysql_retrieve_applications(connection, query, apps, is_learning);
#elif DB_PSQL
   	num_apps = postgresql_retrieve_applications(connection, query, apps, is_learning);
#endif
   
  	if (num_apps == EAR_MYSQL_ERROR){
#if DB_MYSQL
        verbose(VDBH, "Error retrieving information from database (%d): %s\n", mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
#elif DB_PSQL
        verbose(VDBH, "Error retrieving information from database: %s\n", PQerrorMessage(connection));
        PQfinish(connection);
#endif
		return num_apps;
    }
    if (num_apps > 0)
    {
        current_job_id = (*apps)[num_apps - 1].job.id;
        current_step_id = (*apps)[num_apps - 1].job.step_id;
    }
    else if (num_apps == 0)
        db_reset_counters();
    else
        verbose(VDBH, "EAR's mysql internal error: %d\n", num_apps);
#if DB_MYSQL
    mysql_close(connection);
#elif DB_PSQL
    PQfinish(connection);
#endif
	return num_apps;
}

#define LEARNING_APPS_QUERY     "SELECT COUNT(*) FROM Learning_applications WHERE node_id = '%s'"
//#define LEARNING_APPS_ALL_QUERY "SELECT COUNT(*) FROM Learning_applications"
#define LEARNING_APPS_ALL_QUERY "SELECT COUNT(*) FROM (SELECT * FROM Learning_applications GROUP BY job_id, step_id) AS t1"
#define APPS_QUERY              "SELECT COUNT(*) FROM Applications INNER JOIN Signatures ON signature_id = Signatures.id WHERE node_id = '%s' AND "\
                                "time > 60 AND DC_power > 100 AND DC_power < 1000"
#define APPS_ALL_QUERY          "SELECT COUNT(*) FROM (SELECT * FROM Applications GROUP BY job_id, step_id) AS t1"
//#define APPS_ALL_QUERY          "SELECT COUNT(*) FROM Applications"

#if DB_PSQL
ulong postgresql_get_num_applications(char *query)
{
   
    ulong result = 0;
	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    PGresult *res = PQexecParams(connection, query, 0, NULL, NULL, NULL, NULL, 1); //0 indicates text mode, 1 is binary

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        verbose(VMYSQL, "ERROR while reading signature id: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return EAR_ERROR;
    }

    result = htonl( *((uint *)PQgetvalue(res, 0, 0)));
    
    PQclear(res);

    return result;
}
#endif

#if DB_MYSQL
ulong mysql_get_num_applications(char *query)
{
   
	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return 0;
    }

    MYSQL_STMT *statement = mysql_stmt_init(connection);
    if (!statement)
    {
        verbose(VDBH, "Error creating statement (%d): %s\n", mysql_errno(connection),
                mysql_error(connection));
        mysql_close(connection);
        return 0;
    }

    if (mysql_stmt_prepare(statement, query, strlen(query)))
                                      return mysql_statement_error(statement);
    //Result parameters
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    ulong result = 0;
    res_bind[0].buffer = &result;

    if (mysql_stmt_bind_result(statement, res_bind)) return stmt_error(connection, statement);
    if (mysql_stmt_execute(statement)) return stmt_error(connection, statement);
    if (mysql_stmt_store_result(statement)) return stmt_error(connection, statement);

    int status = mysql_stmt_fetch(statement);
    if (status != 0 && status != MYSQL_DATA_TRUNCATED)
        result = 0;

    mysql_stmt_close(statement);
    mysql_close(connection);

    return result;
}
#endif

ulong get_num_applications(char is_learning, char *node_name)
{
    
    ulong ret = 0;
    char query[256];

    if (is_learning && node_name != NULL)
        sprintf(query, LEARNING_APPS_QUERY, node_name);
    else if (!is_learning && node_name != NULL)
        sprintf(query, APPS_QUERY, node_name);
    else if (is_learning && node_name == NULL)
        sprintf(query, LEARNING_APPS_ALL_QUERY);
    else if (!is_learning && node_name == NULL)
        sprintf(query, APPS_ALL_QUERY);

#if DB_MYSQL
    ret = mysql_get_num_applications(query);
#elif DB_PSQL
    ret = postgresql_get_num_applications(query);
#endif

    return ret;
}

#define MAX_DC_POWER_QUERY  "SELECT MAX(DC_power) FROM Learning_applications INNER JOIN Power_signatures ON "\
                            "power_signature_id = id INNER JOIN Learning_jobs ON job_id = Learning_jobs.id AND "\
                            "Learning_applications.step_id = Learning_jobs.step_id WHERE app_id LIKE '%%%s%%' AND "\
                            "Learning_jobs.def_f = %lu"

#define MMAX_DC_POWER_QUERY "SELECT MAX(max_DC_power) FROM Learning_applications INNER JOIN Power_signatures ON "\
                            "power_signature_id = id INNER JOIN Learning_jobs ON job_id = Learning_jobs.id AND "\
                            "Learning_applications.step_id = Learning_jobs.step_id WHERE app_id LIKE '%%%s%%' AND "\
                            "Learning_jobs.def_f = %lu"

float get_max_dc_power(char is_max, char *app_name, ulong freq)
{
    char query[512];
    if (is_max)
        sprintf(query, MMAX_DC_POWER_QUERY, app_name, freq);
    else
        sprintf(query, MAX_DC_POWER_QUERY, app_name, freq);

#if DB_MYSQL
    return run_query_float_result(query);
#else
    warning("Method not implemented for current database library");
    return 0;
#endif
}
