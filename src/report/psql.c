/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <libpq-fe.h>
#include <report/report.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/database/postgresql_io_functions.h>

static db_conf_t *db_config = NULL;

#if DB_PSQL


/***************************************
 *    PostgreSQL specific functions    *
 ***************************************/
static PGconn *postgresql_create_connection()
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

    connection = PQconnectdbParams((const char * const *)keys, (const char * const *)values, 0);

    free(keys);
    free(values);

    if (PQstatus(connection) != CONNECTION_OK)
    {
        verbose(VDBH, "psql.c: ERROR connecting to the database: %s", PQerrorMessage(connection));
        PQfinish(connection);
        return NULL;
    }

    return connection;

}

int postgresql_get_num_columns(char *query)
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




/**************************
 *    Report functions    *
 **************************/
state_t report_init(report_id_t *id, cluster_conf_t *conf)
{
	debug("psql report_init");

    db_config = calloc(1, sizeof(db_conf_t));
    memcpy(db_config, &conf->database, sizeof(db_conf_t));
    char query[256];
    int num_sig_args, num_per_args;
    strcpy(query, "SELECT * FROM Signatures LIMIT 5");
    num_sig_args = postgresql_get_num_columns(query);
    if (num_sig_args == EAR_ERROR)
        set_signature_detail(db_config->report_sig_detail);
    else if (num_sig_args < 20)
        set_signature_detail(0);
    else 
        set_signature_detail(1);

    strcpy(query, "SELECT * FROM Periodic_metrics LIMIT 5");
    num_per_args = postgresql_get_num_columns(query);
    if (num_per_args == EAR_ERROR)
        set_node_detail(db_config->report_node_detail);
    else if (num_per_args < 8)
        set_node_detail(0);
    else
        set_node_detail(1);

    return EAR_SUCCESS;
}


state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (postgresql_batch_insert_applications(connection, &apps[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (postgresql_batch_insert_applications(connection, &apps[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
	}	 

	PQfinish(connection);

    return EAR_SUCCESS;

}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (postgresql_batch_insert_loops(connection, &loops[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing loops to database.");
            PQfinish(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (postgresql_batch_insert_loops(connection, &loops[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing loops to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
	}	 

	PQfinish(connection);

    return EAR_SUCCESS;

}

state_t report_events(report_id_t *id, ear_event_t *eves, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (postgresql_batch_insert_ear_events(connection, &eves[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing events to database.");
            PQfinish(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (postgresql_batch_insert_ear_events(connection, &eves[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing events to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
	}	 

	PQfinish(connection);

    return EAR_SUCCESS;

}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *mets, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (postgresql_batch_insert_periodic_metrics(connection, &mets[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing periodic_metrics to database.");
            PQfinish(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (postgresql_batch_insert_periodic_metrics(connection, &mets[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing periodic_metrics to database.");
            PQfinish(connection);
            return EAR_ERROR;
        }
	}	 

	PQfinish(connection);

    return EAR_SUCCESS;

}

state_t report_misc(report_id_t *id, uint type, const char *data, uint count)
{
    int bulk_elms, bulk_sets, e, s;

	PGconn *connection = postgresql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    switch (type)
    {
        case PERIODIC_AGGREGATIONS:
            bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
            bulk_sets = _BULK_SETS(count, bulk_elms);
            periodic_aggregation_t *aggs = (periodic_aggregation_t *)data;

            // Inserting full bulks one by one
            for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
            {
                if (postgresql_batch_insert_periodic_aggregations(connection, &aggs[e], bulk_elms) < 0) {
                    verbose(VDBH, "ERROR while batch writing periodic_aggregations to database.");
                    PQfinish(connection);
                    return EAR_ERROR;
                }
            }
            // Inserting the lagging bulk, the incomplete last one
            if (e < count)
            {
                if (postgresql_batch_insert_periodic_aggregations(connection, &aggs[e], count - e) < 0) {
                    verbose(VDBH, "ERROR while batch writing periodic_aggregations to database.");
                    PQfinish(connection);
                    return EAR_ERROR;
                }
            }	 
            break;
        case EARGM_WARNINGS:
            if (postgresql_insert_gm_warning(connection, (gm_warning_t *)&data) < 0) {
                verbose(VDBH, "ERROR while batch writing periodic_aggregations to database.");
                PQfinish(connection);
                return EAR_ERROR;
            }

            break;
    }

    PQfinish(connection);

    return EAR_SUCCESS;

}

state_t report_dispose(report_id_t *id) 
{
    if (db_config != NULL) free(db_config);
    return EAR_SUCCESS;
}
#endif
