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

#include <commands/query_helpers.h>

void usage(char *name)
{
    printf("\n");
    printf("Usage: %s\t [options].\t At least a job id is required.\n", name);
    printf("Available options:\n"
           "\t-j [job_id.step_id] \t Specifies which jobs to retrieve.\n"
           "\t-h\t\t\t Shows this message.\n");
    printf("\n");
    //"\t-n [node_name1, nn2]\t Specifies from which nodes the learning phase will be deleted from, or all for every
    // entry. [default: all]\n"
    exit(0);
}

#define LOOPS_QUERY "SELECT * FROM Loops "
#define JOBS_QUERY  "SELECT * FROM Jobs "

int main(int argc, char *argv[])
{
    char node_name[256];
    char job_ids[256] = "";
    int job_id = -1, step_id = -1;
    int opt;
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

    char loops_query[512] = {0};
    char jobs_query[512]  = {0};

    strcpy(loops_query, LOOPS_QUERY);
    add_int_filter(loops_query, "job_id", job_id);
    if (step_id >= 0) {
        add_int_filter(loops_query, "step_id", step_id);
    }
    strcat(loops_query, " ORDER BY job_id, step_id, local_id, total_iterations");

    reset_query_filters();
    strcpy(jobs_query, JOBS_QUERY);
    add_int_filter(jobs_query, "job_id", job_id);
    if (step_id >= 0) {
        add_int_filter(jobs_query, "step_id", step_id);
    }

    typedef struct app_summary {
        application_t app;
        time_t total_time;
    } app_summary_t;

    loop_t *loops       = NULL;
    job_t *jobs         = NULL;
    app_summary_t *apps = NULL;
    int32_t num_apps    = 0;

    int32_t num_loops = db_read_loops_query(&loops, loops_query);
    int32_t num_jobs  = db_read_jobs_query(&jobs, jobs_query);

    time_t previous_time = 0;
    for (int32_t i = 0; i < num_loops; i++) {
        int32_t app_idx = -1;
        for (int32_t j = 0; j < num_apps; j++) {
            /* Appid/local_id is not used as a filter for now since it causes issues (some Jobs report it as 0
             * where the loop reports the pid*/
            if (!strncmp(loops[i].node_id, apps[j].app.node_id, GENERIC_NAME) && loops[i].jid == apps[j].app.job.id &&
                loops[i].step_id == apps[j].app.job.step_id) {
                app_idx = j;
                break;
            }
        }

        if (app_idx < 0) {
            previous_time = 0;
            apps          = realloc(apps, sizeof(app_summary_t) * (num_apps + 1));
            app_idx       = num_apps;
            num_apps++;

            apps[app_idx] = (app_summary_t) {0};

            /* Copy common data between loops of a same node */
            strcpy(apps[app_idx].app.node_id, loops[i].node_id);
            apps[app_idx].app.is_mpi = 1;
            /* Appid/local_id is not used as a filter for now since it causes issues (some Jobs report it as 0
             * where the loop reports the pid*/
            bool found = false;
            for (int32_t j = 0; j < num_jobs; j++) {
                if (jobs[j].id == loops[i].jid && jobs[j].step_id == loops[i].step_id) {
                    memcpy(&apps[app_idx].app.job, &jobs[j], sizeof(job_t));
                    previous_time = jobs[j].start_mpi_time;
                    found         = true;
                    break;
                }
            }
            if (!found)
                printf("%sWarning: a Job entry for loop with jid %lu and stepid %lu has not been found%s\n", COL_RED,
                       loops[i].jid, loops[i].step_id, COL_CLR);
        }

        /* Assume that the first loop is 10 seconds long if we did not find a start_time. */
        if (previous_time <= 0)
            previous_time = loops[i].total_iterations - 10;

        /* Aggregate signatures with weights depending on the time */
        signature_apply_weight(&loops[i].signature, &loops[i].signature, loops[i].total_iterations - previous_time);
        acum_sig_metrics(&apps[app_idx].app.signature, &loops[i].signature);
        /* Workaround for perc_MPI which is not used by acum_sig_metrics */
        apps[app_idx].app.signature.perc_MPI += loops[i].signature.perc_MPI;
        apps[app_idx].total_time += (loops[i].total_iterations - previous_time);

        previous_time = loops[i].total_iterations;
    }

    printf("Printing a total of %d applications!\n", num_apps);
    for (int32_t i = 0; i < num_apps; i++) {
        /* Workaround so we can use the function compute_avg_sig which does not compute perc_MPI*/
        apps[i].app.signature.perc_MPI /= apps[i].total_time;
        compute_avg_sig(&apps[i].app.signature, &apps[i].app.signature, apps[i].total_time);
        apps[i].app.signature.time = apps[i].total_time; // time is the one that we must accumulate without averaging
        verbose_application_data(0, &apps[i].app);
    }

    if (num_loops > 0)
        free(loops);

    free_cluster_conf(&my_conf);

    return 0;
}
