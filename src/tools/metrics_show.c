/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#if DB_MYSQL
#include <mysql/mysql.h>
#endif

#include <common/database/db_helper.h>
#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/string_enhanced.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

void usage(char *name)
{
    printf("\n");
    printf("Usage: %s\t [options].\t At least a job id is required.\n", name);
    printf("Available options:\n"
           "\t-j [job_id.step_id] \t Specifies which jobs to retrieve.\n"
           "\t-t [LOOPS|APP] \t Specifies from where the signatures will be retrieved. (default: APP).\n"
           "\t-h\t\t\t Shows this message.\n");
    printf("\n");
    //"\t-n [node_name1, nn2]\t Specifies from which nodes the learning phase will be deleted from, or all for every
    // entry. [default: all]\n"
    exit(0);
}

#define APPS_QUERY                                                                                                     \
    "SELECT Jobs.job_id, Jobs.step_id, ROUND(AVG(CPI), 4), ROUND(AVG(GBS), 4), ROUND(AVG(TPI), 4), "                   \
    "ROUND(AVG(DC_power), 4), ROUND(AVG(Gflops), 4) FROM "                                                             \
    "Applications INNER JOIN Jobs on Applications.step_id = Jobs.step_id AND Applications.job_id = Jobs.job_id INNER " \
    "JOIN Signatures "                                                                                                 \
    "ON signature_id = Signatures.id WHERE job_id = %d %s GROUP BY Jobs.job_id, Jobs.step_id"

#define LOOP_QUERY                                                                                                     \
    "SELECT job_id, step_id, ROUND(CPI, 4), ROUND(GBS, 4), ROUND(TPI, 4), ROUND(DC_power, 4), "                        \
    "ROUND(Gflops, 4) FROM Loops INNER JOIN Signatures ON signature_id = Signatures.id WHERE job_id = %d %s"

#define APPS  0
#define LOOPS 1

int main(int argc, char *argv[])
{
    char node_name[256];
    char job_ids[256] = "";
    int job_id = -1, step_id = -1;
    int opt, i, j;
    // char all_nodes = 1; Variable set but no used.
    char type = APPS;
    char *token;

    if (argc < 2) {
        usage(argv[0]);
    }

    while ((opt = getopt(argc, argv, "n:j:t:h")) != -1) {
        switch (opt) {
            case 'n':
                /* all_nodes set but not used.
                   if (!strcmp(optarg, "all")) all_nodes = 1;
                   else strcpy(node_name, optarg);
                   */
                if (strcmp(optarg, "all")) {
                    strcpy(node_name, optarg);
                }
                break;
            case 'j':
                strcpy(job_ids, optarg);
                job_id = atoi(strtok(optarg, "."));
                token  = strtok(NULL, ".");
                if (token)
                    step_id = atoi(token);

                break;
            case 'h':
                usage(argv[0]);
                break;
            case 't':
                if (!strcasecmp(optarg, "LOOPS"))
                    type = LOOPS;
                break;
            default:
                printf("Unknown argument\n");
                break;
        }
    }

    if (job_id < 0) {
        printf("Warning, a job_id MUST be specified\n");
        exit(0);
    }

    char ear_path[256];
    cluster_conf_t my_conf;
    if (get_ear_conf_path(ear_path) == EAR_ERROR) {
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path, &my_conf);

    if (strlen(my_conf.database.user_commands) < 1) {
        fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
    } else {
        strcpy(my_conf.database.user, my_conf.database.user_commands);
        strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    }
    init_db_helper(&my_conf.database);

    char ***results;
    int num_cols = 0;
    int num_rows = 0;

    char query[512];
    char tmp[64];
    strcpy(tmp, "");

    if (type == LOOPS) {
        if (step_id >= 0)
            sprintf(tmp, "AND step_id = %d ", step_id);
        sprintf(query, LOOP_QUERY, job_id, tmp);
    } else {
        if (step_id >= 0)
            sprintf(tmp, "AND Jobs.step_id = %d ", step_id);
        sprintf(query, APPS_QUERY, job_id, tmp);
    }
    // printf("running query %s\n", query);

    printf("%10s %10s %10s %10s %10s %10s %10s\n", "Job", "Step", "CPI", "GBS", "TPI", "Power", "Gflops");
    db_run_query_string_results(query, &results, &num_cols, &num_rows);
    for (i = 0; i < num_rows; i++) {
        for (j = 0; j < num_cols; j++) {
            printf("%10s ", results[i][j]);
        }
        printf("\n");
    }

    db_free_results(results, num_cols, num_rows);

    free_cluster_conf(&my_conf);

    return 0;
}
