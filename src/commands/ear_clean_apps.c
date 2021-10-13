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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <mysql/mysql.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/types/configuration/cluster_conf.h>

int query_filters = 0;

//#warning "check this query"

#if USE_GPUS
#define NUM_APP_Q   5
#define NUM_LOOP_Q  3
#else
#define NUM_APP_Q   4
#define NUM_LOOP_Q  2
#endif
#define NUM_QUERIES NUM_APP_Q+NUM_LOOP_Q

#if USE_GPUS
#define CLEAN_GPU_APPS  "DELETE Applications,Jobs,Signatures,Power_signatures,GPU_signatures FROM Applications inner join Jobs on id = job_id AND "\
                        "Applications.step_id = Jobs.step_id inner join Signatures on Signatures.id = signature_id inner join Power_signatures on "\
                        "Power_signatures.id = power_signature_id INNER JOIN GPU_signatures ON GPU_signatures.id >= min_GPU_sig_id AND "\
                        "GPU_signatures.id <= max_GPU_sig_id "

#define CLEAN_GPU_LOOPS "DELETE Loops,Signatures,GPU_signatures FROM Loops inner join Jobs on id = job_id AND "\
                        "Loops.step_id = Jobs.step_id inner join Signatures on Signatures.id = signature_id "\
                        "INNER JOIN GPU_signatures ON GPU_signatures.id >= min_GPU_sig_id AND "\
                        "GPU_signatures.id <= max_GPU_sig_id "
#endif

#define CLEAN_MPI_APPS  "DELETE Applications,Jobs,Signatures,Power_signatures from Applications inner join Jobs on id = job_id and Applications.step_id = Jobs.step_id "\
                        "inner join Signatures on Signatures.id = signature_id inner join Power_signatures on Power_signatures.id = power_signature_id "

#define CLEAN_NMPI_APPS "DELETE Applications,Jobs,Power_signatures from Applications inner join Jobs on id = job_id and Applications.step_id = Jobs.step_id "\
                        "inner join Power_signatures on Power_signatures.id = power_signature_id "

#define CLEAN_LOOPS_JOBS "DELETE Signatures,Loops FROM Loops INNER JOIN Jobs ON Jobs.id = job_id AND Jobs.step_id = Loops.step_id INNER JOIN Signatures ON Loops.signature_id = Signatures.id "

#define CLEANUP_LOOPS_JOBS "DELETE Loops FROM Loops INNER JOIN Jobs ON Jobs.id = job_id AND Jobs.step_id = Loops.step_id"

#define CLEAN_LOOPS "DELETE Signatures,Loops FROM Loops INNER JOIN Signatures ON Loops.signature_id = Signatures.id "

#define CLEANUP_LOOPS "DELETE Loops FROM Loops "

#define CLEANUP_APPS "DELETE Applications,Jobs from Applications inner join Jobs on id = job_id and Applications.step_id = Jobs.step_id "

#define CLEANUP_JOBS "DELETE Jobs from Jobs "


void print_version()
{
    char msg[256];
    sprintf(msg, "EAR version %s\n", RELEASE);
    printf(msg);
    exit(0);
}

void usage(char *app)
{
	printf("Usage:%s [-j/-d] [options]\n", app);
    printf("\t-p\t\tThe program will request the database user's password.\n");
#if DB_MYSQL
    printf("\t-u user\t\tDatabase user to execute the operation (it needs DELETE privileges). [default: root]\n");
#elif DB_PSQL
    printf("\t-u user\t\tDatabase user to execute the operation (it needs DELETE privileges). [default: postgres]\n");
#endif
    printf("\t-j jobid.stepid\tJob id and step id to delete. If no step_id is introduced, every step within the job will be deleted\n");
    printf("\t-d ndays\tDays to preserve. It will delete any jobs older than ndays.\n");
    printf("\t-o\t\tPrints out the queries that would be executed. Exclusive with -r. [default:on]\n");
    printf("\t-r\t\tRuns the queries that would be executed. Exclusive with -o. [default:off]\n");
    printf("\t-l\t\tDeletes Loops and its Signatures. [default:off]\n");
    printf("\t-a\t\tDeletes Applications and related tables. [default:off]\n");
    printf("\t-h\t\tDisplays this message\n");
    printf("\n");

	exit(0);
}

void add_time_filter(char *query, char *addition, int value)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");
    
    strcat(query_tmp, addition);
    strcat(query_tmp, "<= UNIX_TIMESTAMP(SUBTIME(NOW(), '%d 0:0:0.00'))");
    sprintf(query, query_tmp, value);
    query_filters ++;
}

void add_int_filter(char *query, char *addition, int value)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");
    
    strcat(query_tmp, addition);
    strcat(query_tmp, "=");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters ++;
}
#define OUT_QUERY 1
#define RUN_QUERY 2

void send_query(char *query, char out_type, cluster_conf_t *my_cluster, char *user, char *passw)
{
    if (out_type == OUT_QUERY) {
        printf("%s\n", query);
    }
    else if (out_type == RUN_QUERY) {
        init_db_helper(&my_cluster->database);
        db_run_query(query, user, passw); 
        printf("Database successfully cleaned.\n");
    }
}
  
int main(int argc,char *argv[])
{
    char user[256], passw[256], query[NUM_QUERIES][512];
    int num_days = 0;
    int job_id = -1, step_id = -1;
    char rm_loops = 0, rm_apps = 0;
    int i;
    char opt, *token;
    struct termios t;
    char out_type = OUT_QUERY;

    /* No password as default */
    strcpy(passw, "");
    /* Root user as default */
#if DB_PSQL
    strcpy(user, "postgres");
#else
    strcpy(user, "root");
#endif

    while ((opt = getopt(argc, argv, "u:pd:j:oalrh")) != -1) {
        switch(opt)
        {
            case 'd':
                num_days = atoi(optarg);
                break;
            case 'j':
                job_id = atoi(strtok(optarg, "."));
                token = strtok(NULL, ".");
                if (token != NULL) step_id = atoi(token);
                break;
            case 'p':
                tcgetattr(STDIN_FILENO, &t);
                t.c_lflag &= ~ECHO;
                tcsetattr(STDIN_FILENO, TCSANOW, &t);

                printf("Introduce root's password:");
                fflush(stdout);
                fgets(passw, sizeof(passw), stdin);
                t.c_lflag |= ECHO;
                tcsetattr(STDIN_FILENO, TCSANOW, &t);
                strclean(passw, '\n');
                printf(" \n");
                break;
            case 'u':
                strcpy(user, optarg);
                break;
            case 'l':
                rm_loops = 1;
                break;
            case 'a':
                rm_apps = 1;
                break;
            case 'o':
                out_type = OUT_QUERY;
                break;
            case 'r':
                out_type = RUN_QUERY;
                break;
            case 'h':
                usage(argv[0]);
                break;
        }
    }

    cluster_conf_t my_cluster;
    char ear_path[256];
    if (get_ear_conf_path(ear_path) == EAR_ERROR)
    {
        printf("Error getting ear.conf path\n"); //error
        exit(0);
    }
    read_cluster_conf(ear_path, &my_cluster);
            
    if (job_id < 0 && num_days < 1) {
        printf("A job id or a number of days must be specified, stopping.\n");
        free_cluster_conf(&my_cluster);
        exit(0);
    }
    if (!rm_apps && !rm_loops) {
        printf("Either loops or applications need to be asked for removal, stopping.\n");
        free_cluster_conf(&my_cluster);
        exit(0);
    }
#if USE_GPUS
    uint idx = 1;
#else
    uint idx = 0;
#endif

    int queries_id = 0;
    //loops come first or we won't be able to find them
    if (rm_loops) {
        if (num_days > 0) {
#if USE_GPUS
            strcpy(query[queries_id      ], CLEAN_GPU_LOOPS);
#endif
            strcpy(query[queries_id+idx  ], CLEAN_LOOPS_JOBS);
            strcpy(query[queries_id+idx+1], CLEANUP_LOOPS_JOBS);
        }
        else {
#if USE_GPUS
            strcpy(query[queries_id      ], CLEAN_GPU_LOOPS);
#endif
            strcpy(query[queries_id+idx  ], CLEAN_LOOPS);
            strcpy(query[queries_id+idx+1], CLEANUP_LOOPS);
        }
        queries_id += NUM_LOOP_Q;
    }
    if (rm_apps) {
#if USE_GPUS
        strcpy(query[queries_id      ], CLEAN_GPU_APPS);
#endif
        strcpy(query[queries_id+idx  ], CLEAN_MPI_APPS);
        strcpy(query[queries_id+idx+1], CLEAN_NMPI_APPS);
        strcpy(query[queries_id+idx+2], CLEANUP_APPS);
        strcpy(query[queries_id+idx+3], CLEANUP_JOBS);
        queries_id += NUM_APP_Q;
    }

    for (i = 0; i < queries_id; i++) {
        query_filters = 0;
        if (rm_loops && num_days < 1 && i < NUM_LOOP_Q) { //this is more precise as it doesn't need Jobs to exist to delete them
            if (job_id > -1) {
                add_int_filter(query[i], "job_id", job_id);
            }
            if (step_id > -1) {
                add_int_filter(query[i], "step_id", step_id);
            }
            continue;
        }
        if (job_id > -1) {
            add_int_filter(query[i], "Jobs.id", job_id);
        }
        if (step_id > -1) {
            add_int_filter(query[i], "Jobs.step_id", step_id);
        }
        if (num_days > 0) {
            add_time_filter(query[i], "Jobs.end_time", num_days);
        }
    }


    for (i = 0; i < queries_id; i++)
        send_query(query[i], out_type, &my_cluster, user, passw);


    free_cluster_conf(&my_cluster);


    exit(1);
}
