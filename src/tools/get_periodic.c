/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/database/db_helper.h>

#define DIRECT_PERIODIC_QUERY "SELECT * FROM Periodic_metrics WHERE job_id IN (%s)"
#define JOB_QUERY "SELECT min(start_time), max(end_time) FROM Jobs WHERE job_id IN (%s)"
#define APP_QUERY "SELECT distinct(node_id) FROM Applications WHERE job_id IN (%s)"
#define INDIRECT_PERIODIC_QUERY "SELECT * FROM Periodic_metrics WHERE end_time >= %s AND start_time <= %s AND node_id IN (%s)"

void usage(char *argv)
{
	printf("Usage: %s [OPTIONS]\n", argv);
	printf("\nOptions:\n");
	printf("\t-j\t--job-id  <id.step>\tSpecifies which job-id the periodic metrics will be retrieved from. It can be a list of jobs.\n");
	printf("\t-n\t--node-id <\"id\">\tSpecifies which nodes the periodic metrics will be retrieved from. It can be a list of nodes\n");
	printf("\t-f\t--file <filename>\tRedirects the output to the file specified.\n");
	printf("\t-x\t--no-filter\t\tPeriodic metrics will be filtered by their job id field, not by time.\n");
	printf("\t-v\t--verbose\t\tDisplays execution messages.\n");
	printf("\t-h\t--help \t\t\tDisplays this message.\n");
    printf("Note: for more efficient executions, when possible execute the program with the node-ids specified in a \n\t list (i.e -n \"node1\",\"node2\") to avoid running an additional query\n\n");
	exit(1);
}

void prepare_query(char *query, char *BASE_QUERY, char *job_ids, char *job_id, char *step_id, char *node_ids)
{
	char aux_query[256];
	if (job_ids != NULL) {
		sprintf(query, BASE_QUERY, job_ids);
	} else {
		sprintf(query, BASE_QUERY, job_id);
		if (step_id != NULL) {
			sprintf(aux_query, " AND step_id = %s", step_id);
			strcat(query, aux_query);
		}
	}
	if (node_ids != NULL) {
		sprintf(aux_query, " AND node_id IN (%s)", node_ids);
	}
}

void print_header(int fd, char ***values, int columns, int rows)
{
	dprintf(fd, "%s", values[0][0]);

	int i;
	for (i = 1; i < rows; i++) {
		dprintf(fd, ";%s", values[i][0]);
	}
	dprintf(fd, "\n");
}

void print_values(int fd, char ***values, int columns, int rows)
{
	int i, j;
	for (i = 0; i < rows; i++) {
			dprintf(fd, "%s", values[i][0]);
		for (j = 1; j < columns; j++) {
			dprintf(fd, ";%s", values[i][j]);
		}
		dprintf(fd, "\n");
	}
}

int main(int argc, char *argv[])
{
	char buffer[256], query[1024];
	char ***values = NULL;
	char *job_ids = NULL, *job_id = NULL, *step_id = NULL, *node_ids = NULL;
	int no_filter = 0, rows, columns;
	cluster_conf_t my_conf;
	int i, option_idx, ret, fd = STDOUT_FILENO; 
	char c, debug = 0;

	if (argc < 2) usage(argv[0]);

	static struct option long_options[] = {
		{"help",      no_argument,       0, 'h'},
		{"verbose",   no_argument,       0, 'v'},
		{"no-filter", no_argument,       0, 'x'},
		{"job-id",    required_argument, 0, 'n'},
		{"node-id",   required_argument, 0, 'j'},
		{"file",      required_argument, 0, 'f'},
	};

	while (1)
	{
		c = getopt_long(argc, argv, "f:xn:hj:", long_options, &option_idx);
		if (c == -1) break;
		switch (c)
		{
			case 'j':
				if (strchr(optarg, ',')) {
					job_ids = optarg;
				} else {
					job_id = strtok(optarg, ".");
					step_id = strtok(NULL, ".");
				}
				break;
			case 'n':
				node_ids = optarg;
				break;
			case 'f':
				fd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR|S_IWUSR|S_IRGRP);
				break;
			case 'x':
				no_filter = 1;
				break;
			case 'v':
				debug = 1;
				break;
			case 'h':
				usage(argv[0]);
				break;
		}
	}

	if (job_ids == NULL && job_id == NULL) {
		printf("At least 1 job needs to be specified\n");
	}
	if (debug) {
		if (job_ids != NULL) printf("job_ids: %s\n", job_ids);
		if (node_ids != NULL) printf("node_ids: %s\n", node_ids);
		if (job_id != NULL && step_id != NULL)
			printf("job_id %s step_id %s, filter %d\n", job_id, step_id, no_filter);
		else if (job_id != NULL)
			printf("job_id %s step_id NULL, filter %d\n", job_id, no_filter);
	}

	if (get_ear_conf_path(buffer) == EAR_ERROR) {
		printf("ERROR while getting ear.conf path\n");
		exit(0);
	}

	read_cluster_conf(buffer, &my_conf);

	init_db_helper(&my_conf.database);

	//get and print header
	sprintf(query, "DESCRIBE Periodic_metrics");
	ret = db_run_query_string_results(query, &values, &columns, &rows);
	if (ret != EAR_SUCCESS) {
		printf("Error retrieving data\n");
		goto cleanup;
	}
	// Print header and free values
	print_header(fd, values, columns, rows);
	db_free_results(values, columns, rows);

	if (no_filter) { //1 query
		// Prepare query
		prepare_query(query, DIRECT_PERIODIC_QUERY, job_ids, job_id, step_id, node_ids);

		// Run query
		ret = db_run_query_string_results(query, &values, &columns, &rows);
		if (debug) {
			printf("query: %s\n", query);
			printf("num rows: %d, num cols %d\n", rows, columns);
		}
		if (ret != EAR_SUCCESS) {
			printf("Error retrieving data\n");
			goto cleanup;
		}
		// Print result and free it
		print_values(fd, values, columns, rows);
		db_free_results(values, columns, rows);
	} else { // up to 3 queries
		// Prepare query
		prepare_query(query, JOB_QUERY, job_ids, job_id, step_id, NULL);
		if (debug) printf("query: %s\n", query);

		// Run first query to get start and end time
		ret = db_run_query_string_results(query, &values, &columns, &rows);
		if (debug) printf("num rows: %d, num cols %d\n", rows, columns);

		if (ret != EAR_SUCCESS || rows != 1 || columns != 2) {
			printf("Error retrieving start and end_time\n");
			goto cleanup;
		}
		char start_time[32], end_time[32];
		strcpy(start_time, values[0][0]);
		strcpy(end_time,   values[0][1]);

		char list_nodes[512];
		int len = 0, max_len = 512; //max len is the capacity of list_nodes

		// Free results
		db_free_results(values, columns, rows);
		// If the nodelist is provided, copy it. Otherwise, get it from Applications
		if (node_ids != NULL) {
			strcpy(list_nodes, node_ids);
		} else {
			// Prepare query
			prepare_query(query, APP_QUERY, job_ids, job_id, step_id, NULL);
			if (debug) printf("query: %s\n", query);
			// Run query to get the list of nodes
			ret = db_run_query_string_results(query, &values, &columns, &rows);
			if (debug) printf("num rows: %d, num cols %d\n", rows, columns);

			if (ret != EAR_SUCCESS || rows < 1 || columns != 1) {
				printf("Error retrieving nodelist\n");
				goto cleanup;
			}
			// Format the list of nodes as a string
			strcpy(list_nodes, "");
			for (i = 0; i < rows-1; i++) {
				len += snprintf(list_nodes+len, max_len-len, "\"%s\",", values[i][0]);
			}
			len += snprintf(list_nodes+len, max_len-len, "\"%s\"", values[rows-1][0]);
			// Once processed, free results
			db_free_results(values, columns, rows);
		}

		// Finally, get the last query and the results 
		// Prepare the query
		sprintf(query, INDIRECT_PERIODIC_QUERY, start_time, end_time, list_nodes);
		if (debug) printf("query: %s\n", query);
		// Run the query
		ret = db_run_query_string_results(query, &values, &columns, &rows);
		if (debug) printf("num rows: %d, num cols %d\n", rows, columns);
		if (ret != EAR_SUCCESS) {
			printf("Error retrieving data\n");
			goto cleanup;
		}
		// Print result and free it
		print_values(fd, values, columns, rows);
		db_free_results(values, columns, rows);
	}

cleanup:
	if (fd != STDOUT_FILENO) {
		close(fd);
	}
	free_cluster_conf(&my_conf);
}
