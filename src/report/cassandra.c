/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <cassandra.h>
#include <report/report.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>
  
CassFuture* connect_future = NULL;
CassCluster* cluster = NULL;
CassSession* session = NULL;

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
	verbose(0, "cassandra report_init");

	cluster = cass_cluster_new();
	session = cass_session_new();
	//cass_cluster_set_contact_points(cluster, cconf->database.ip);
	cass_cluster_set_contact_points(cluster, "127.0.0.1");
	connect_future = cass_session_connect(session, cluster);
	if (cass_future_error_code(connect_future) == CASS_OK) {
		verbose(0, "cassandra connection successfully created");
	} else {
			const char* message;
			size_t message_length;
			cass_future_error_message(connect_future, &message, &message_length);
			verbose(0, "error trying to connect : %s", message);
	}

	return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
	cass_session_close(session); //close session and free all the global structures

	cass_future_free(connect_future);
	cass_future_free(connect_future);
	cass_session_free(session);
	return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *mets, uint count)
{
	int i;
	char tmp_query[512];
#if USE_GPUS
	const char *base_query = "INSERT INTO EAR_test.periodic_metrics (start_time, end_time, "\
							  "dc_energy, node_id, job_id, step_id, avg_f, "\
							  "temp, dram_energy, pck_energy, gpu_energy) "\
							  "VALUES (%d, %d, %d, '%s', %d, %d, %d, %d, %d, %d, %d) ";
#else
	const char *base_query = "INSERT INTO EAR_test.periodic_metrics (start_time, end_time, "\
							  "dc_energy, node_id, job_id, step_id, avg_f, "\
							  "temp, dram_energy, pck_energy, gpu_energy) "\
							  "VALUES (%d, %d, %d, '%s', %d, %d, %d, %d, %d, %d) ";
#endif
	for (i = 0; i < count; i++)
	{
		//format the query
#if USE_GPUS
		sprintf(tmp_query, base_query, mets[i].start_time, mets[i].end_time, mets[i].DC_energy, 
									mets[i].node_id, mets[i].job_id, mets[i].step_id, 
									mets[i].avg_f, mets[i].temp, mets[i].DRAM_energy,
									mets[i].PCK_energy, mets[i].GPU_energy);
#else
		sprintf(tmp_query, base_query, mets[i].start_time, mets[i].end_time, mets[i].DC_energy, 
									mets[i].node_id, mets[i].job_id, mets[i].step_id, 
									mets[i].avg_f, mets[i].temp, mets[i].DRAM_energy,
									mets[i].PCK_energy);
#endif
		verbose(0, "inserting query: %s\n\n", tmp_query);

		//create local structures and run the query
		CassStatement* statement = cass_statement_new(tmp_query, 0);
		CassFuture* result_future = cass_session_execute(session, statement); //query exec

		if (cass_future_error_code(result_future) == CASS_OK) {
			verbose(0, "successfully inserted periodic_metric into cassandra");
		} else {
			const char* message;
			size_t message_length;
			cass_future_error_message(result_future, &message, &message_length);
			verbose(0, "error running query: %s", message);
		}
		
		//free local structures
		cass_statement_free(statement);
		cass_future_free(result_future);

	}

	verbose(0, "cassandra report_applications");
	return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
	verbose(0, "cassandra report_loops");
	return EAR_SUCCESS;
}
