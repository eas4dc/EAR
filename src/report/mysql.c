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

//#define SHOW_DEBUGS 1
#include <stdio.h>
#include <report/report.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/database/mysql_io_functions.h>


#if DB_MYSQL


static db_conf_t *db_config = NULL;

#define IS_SIG_FULL_QUERY   "SELECT COUNT(*) FROM information_schema.columns where TABLE_NAME='Signatures' AND TABLE_SCHEMA='%s'"
#define IS_NODE_FULL_QUERY  "SELECT COUNT(*) FROM information_schema.columns where TABLE_NAME='Periodic_metrics' AND TABLE_SCHEMA='%s'"

/**************************************
 *      MySQL specific functions      *
 **************************************/
static MYSQL *mysql_create_connection()
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

    if (!mysql_real_connect(connection, db_config->ip, db_config->user, db_config->pass, db_config->database, db_config->port, NULL, 0))
    {
        verbose(VDBH, "ERROR connecting to the database: %s", mysql_error(connection));
        mysql_close(connection);
        return NULL;
    }

    return connection;
}

MYSQL_RES *mysql_run_query_result(char *query)
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

int mysql_run_query_int_result(char *query)
{
    MYSQL_RES *result = mysql_run_query_result(query);
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






/****************************
 *     Report functions     *
 ****************************/

state_t report_init(report_id_t *id, cluster_conf_t *conf)
{
	debug("mysql report_init");
	verbose(VDBH,"mysql init");

    db_config = calloc(1, sizeof(db_conf_t));
    memcpy(db_config, &conf->database, sizeof(db_conf_t));
    char query[256];
    int num_sig_args, num_per_args;

    sprintf(query, IS_SIG_FULL_QUERY, db_config->database);
    num_sig_args = mysql_run_query_int_result(query);

    if (num_sig_args == EAR_ERROR)
        set_signature_simple(db_config->report_sig_detail);
    else if (num_sig_args < 20)
        set_signature_simple(0);
    else 
        set_signature_simple(1);


    sprintf(query, IS_NODE_FULL_QUERY, db_config->database);
    num_per_args = mysql_run_query_int_result(query);
    if (num_per_args == EAR_ERROR) {
        set_node_detail(db_config->report_node_detail);
    } else if (num_per_args < 8) {
        set_node_detail(0);
    } else {
        set_node_detail(1);
    }

	return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (mysql_batch_insert_applications(connection, &apps[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (mysql_batch_insert_applications(connection, &apps[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing applications to database.");
            mysql_close(connection);
            return EAR_ERROR;
        }
	}	 

	mysql_close(connection);

    return EAR_SUCCESS;

}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

	verbose(VDBH,"Inserting %u loops in mysql DB", count);

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (mysql_batch_insert_loops(connection, &loops[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing loops to database.");
            mysql_close(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (mysql_batch_insert_loops(connection, &loops[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing loops to database.");
            mysql_close(connection);
            return EAR_ERROR;
        }
	}	 

	mysql_close(connection);

    return EAR_SUCCESS;

}

state_t report_events(report_id_t *id, ear_event_t *eves, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (mysql_batch_insert_ear_events(connection, &eves[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing events to database.");
            mysql_close(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (mysql_batch_insert_ear_events(connection, &eves[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing events to database.");
            mysql_close(connection);
            return EAR_ERROR;
        }
	}	 

	mysql_close(connection);

    return EAR_SUCCESS;

}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *mets, uint count)
{
    int bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
	int bulk_sets = _BULK_SETS(count, bulk_elms);
	int e, s;

	MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    // Inserting full bulks one by one
	for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
    {
    	if (mysql_batch_insert_periodic_metrics(connection, &mets[e], bulk_elms) < 0) {
        	verbose(VDBH, "ERROR while batch writing periodic_metrics to database.");
            mysql_close(connection);
        	return EAR_ERROR;
    	}
	}
	// Inserting the lagging bulk, the incomplete last one
	if (e < count)
	{
		if (mysql_batch_insert_periodic_metrics(connection, &mets[e], count - e) < 0) {
            verbose(VDBH, "ERROR while batch writing periodic_metrics to database.");
            mysql_close(connection);
            return EAR_ERROR;
        }
	}	 

	mysql_close(connection);

    return EAR_SUCCESS;

}

state_t report_misc(report_id_t *id, uint type, const char *data, uint count)
{
    int bulk_elms, bulk_sets, e, s;

    MYSQL *connection = mysql_create_connection();

    if (connection == NULL) {
        return EAR_ERROR;
    }

    periodic_aggregation_t *aggs = (periodic_aggregation_t *)data;
    switch (type)
    {
        case PERIODIC_AGGREGATIONS:
            bulk_elms = _BULK_ELMS(_MMAAXX(APP_VARS, PSI_VARS, NSI_VARS, JOB_VARS));
            bulk_sets = _BULK_SETS(count, bulk_elms);
            // Inserting full bulks one by one
            for (e = 0, s = 0; s < bulk_sets; e += bulk_elms, s += 1)
            {
                if (mysql_batch_insert_periodic_aggregations(connection, &aggs[e], bulk_elms) < 0) {
                    verbose(VDBH, "ERROR while batch writing periodic_aggregations to database.");
                    mysql_close(connection);
                    return EAR_ERROR;
                }
            }
            // Inserting the lagging bulk, the incomplete last one
            if (e < count)
            {
                if (mysql_batch_insert_periodic_aggregations(connection, &aggs[e], count - e) < 0) {
                    verbose(VDBH, "ERROR while batch writing periodic_aggregations to database.");
                    mysql_close(connection);
                    return EAR_ERROR;
                }
            }	 
            break;
        case EARGM_WARNINGS:
            if (mysql_insert_gm_warning(connection, (gm_warning_t *)data) < 0) {
                    verbose(VDBH, "ERROR while writing gm_warning to database.");
                    mysql_close(connection);
                    return EAR_ERROR;
            }
            break;
        default:
            verbose(VDBH, "Trying to insert unknown type to database");
            break;
    }

    mysql_close(connection);

    return EAR_SUCCESS;

}

state_t report_dispose(report_id_t *id) 
{
    if (db_config != NULL) free(db_config);
    return EAR_SUCCESS;
}
#endif

