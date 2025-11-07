/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/database/db_helper.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <stdio.h>

int64_t compute_table_size(char ***values, int columns, int rows)
{
    int64_t i;
    int64_t total_size = 0;
    for (i = 0; i < rows; i++) {
        // the size is always in the second column
        int64_t current_size = 0;
        if (!strncmp(values[i][1], "bigint", 6) || !strncmp(values[i][1], "double", 6)) {
            current_size = 8;
        } else if (!strncmp(values[i][1], "int", 3) || !strncmp(values[i][1], "float", 5)) {
            current_size = 4;
        } else if (!strncmp(values[i][1], "smallint", 8)) {
            current_size = 2;
        } else if (!strncmp(values[i][1], "varchar", 7)) {
            char *aux = &values[i][1][8]; // first number in varchar(12345)
            while (*aux && *aux != ')') {
                int64_t aux_int = *aux - '0';
                current_size    = current_size * 10 + aux_int;
                aux++;
            }
        }
        total_size += current_size;
    }
    return total_size;
}

state_t get_table_size(char *table, int64_t *size)
{
    char ***values = NULL;
    char query[1024];
    int32_t columns = 0, rows = 0;
    state_t ret = EAR_SUCCESS;

    sprintf(query, "DESCRIBE %s", table);
    ret = db_run_query_string_results(query, &values, &columns, &rows);
    if (ret != EAR_SUCCESS) {
        printf("Error reading %s database description\n", table);
        return ret;
    }

    *size = compute_table_size(values, columns, rows);

    for (int64_t i = 0; i < rows; i++) {
        for (int64_t j = 0; j < columns; j++) {
            free(values[i][j]);
        }
        free(values[i]);
    }
    free(values);

    return ret;
}

void usage(char *app)
{
    printf("%s [num_nodes] [monitoring period (in seconds)] [jobs/day] [nodes/job] [avg job duration (in seconds)] "
           "[avg GPUs used/job] [days_to_compute (default 1)] [unit (default MB)]\n",
           app);
    exit(0);
}

int main(int argc, char *argv[])
{
    cluster_conf_t my_conf = {0};
    char conf_path[256]    = {0};

    if (argc < 5)
        usage(argv[0]);

    if (get_ear_conf_path(conf_path) == EAR_ERROR) {
        printf("ERROR while getting ear.conf path\n");
        exit(0);
    }

    int64_t num_nodes       = atoi(argv[1]);
    int64_t mon_period      = atoi(argv[2]);
    int64_t job_day         = atoi(argv[3]);
    int64_t nodes_job       = atoi(argv[4]);
    int64_t duration_job    = atoi(argv[5]);
    int64_t gpus_job        = atoi(argv[6]);
    int64_t days_to_compute = 1;
    int64_t unit            = 1000000;
    char unit_name[64]      = "MBytes";
    if (argc >= 8)
        days_to_compute = atoi(argv[7]);
    if (argc >= 9) {
        if (!strncasecmp(argv[8], "TB", 2)) {
            unit = 1000000000000;
            strcpy(unit_name, "TBytes");
        }
        if (!strncasecmp(argv[8], "GB", 2)) {
            unit = 1000000000;
            strcpy(unit_name, "GBytes");
        }
        if (!strncasecmp(argv[8], "KB", 2)) {
            unit = 1000;
            strcpy(unit_name, "KBytes");
        }
        days_to_compute = atoi(argv[7]);
    }

    read_cluster_conf(conf_path, &my_conf);

    init_db_helper(&my_conf.database);

    /* Initial data reading */
    int64_t periodic_metric_size = 0;
    get_table_size("Periodic_metrics", &periodic_metric_size);
    printf("Periodic_metrics      size per entry is %ld bytes\n", periodic_metric_size);
    int64_t periodic_aggregation_size = 0;
    get_table_size("Periodic_aggregations", &periodic_aggregation_size);
    printf("Periodic_aggregations size per entry is %ld bytes\n", periodic_aggregation_size);
    int64_t application_size = 0;
    get_table_size("Applications", &application_size);
    printf("Applications          size per entry is %ld bytes\n", application_size);
    int64_t signature_size = 0;
    get_table_size("Signatures", &signature_size);
    printf("Signatures            size per entry is %ld bytes\n", signature_size);
    int64_t pow_signature_size = 0;
    get_table_size("Power_signatures", &pow_signature_size);
    printf("Power_signatures      size per entry is %ld bytes\n", pow_signature_size);
    int64_t gpu_signature_size = 0;
    get_table_size("GPU_signatures", &gpu_signature_size);
    printf("GPU_signatures        size per entry is %ld bytes\n", gpu_signature_size);
    int64_t loop_size = 0;
    get_table_size("Loops", &loop_size);
    printf("Loops                 size per entry is %ld bytes\n", loop_size);
    int64_t job_size = 0;
    get_table_size("Jobs", &job_size);
    printf("Jobs                  size per entry is %ld bytes\n", job_size);
    int64_t event_size = 0;
    get_table_size("Events", &event_size);
    printf("Events                size per entry is %ld bytes\n", event_size);
    int64_t global_energy_size = 0;
    get_table_size("Global_energy", &global_energy_size);
    printf("Global_energy         size per entry is %ld bytes\n", global_energy_size);

    int64_t periodic_metrics_estimate = periodic_metric_size * num_nodes * ((days_to_compute * 24 * 3600) / mon_period);
    double periodic_metrics_double    = (double) periodic_metrics_estimate / unit;

    int64_t jobs_estimate =
        nodes_job * job_day * days_to_compute *
            (application_size + pow_signature_size + signature_size + gpu_signature_size * gpus_job) +
        job_day * days_to_compute * job_size;
    double jobs_double = (double) jobs_estimate / unit;

    int64_t loops_estimate = nodes_job * job_day * days_to_compute * duration_job / 10 *
                             (loop_size + signature_size + gpu_signature_size * gpus_job);
    double loops_double = (double) loops_estimate / unit;
    printf("\n----------------------\n");
    printf("Estimates for %d days:\n", days_to_compute);
    printf("Monitoring size estimate (periodic metrics) %.3lf %s\n", periodic_metrics_double, unit_name);
    printf("Job accounting size estimate (applications, jobs, signatures, power_signatures, gpu_signatures) %.3lf %s\n",
           jobs_double, unit_name);
    printf("Loop accounting size estimate (loops & signatures) %.3lf %s\n", loops_double, unit_name);

    free_cluster_conf(&my_conf);
}
