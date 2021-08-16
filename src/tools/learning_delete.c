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

#if DB_MYSQL
#include <mysql/mysql.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <common/string_enhanced.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/database/db_helper.h>

// Multiline comments throw a warning
//#define ALL_QUERY   "DELETE Learning_signatures, Learning_applications, Learning_jobs FROM Learning_jobs " 
//                    "INNER JOIN Learning_applications INNER JOIN Learning_signatures ON Learning_jobs.id=job_id " 
//                    "AND Learning_jobs.step_id=Learning_applications.step_id AND Learning_signatures.id=signature_id"

#define ALL_QUERY1  "DELETE FROM Power_signatures WHERE id IN (SELECT power_signature_id from Learning_applications)"
#define ALL_QUERY2  "DELETE FROM Learning_applications"
#define ALL_QUERY3  "DELETE FROM Learning_signatures"
#define ALL_QUERY4  "DELETE FROM Learning_jobs"

#define POW_SIG_QUERY "DELETE FROM Power_signatures WHERE id IN (SELECT power_signature_id from Learning_applications " \
                        "INNER JOIN Learning_jobs ON Learning_jobs.id=job_id AND Learning_jobs.step_id=Learning_applications.step_id "\
                        "WHERE " 
#define NODE_QUERY  "DELETE Learning_signatures, Learning_applications, Learning_jobs FROM Learning_jobs " \
                    "INNER JOIN Learning_applications INNER JOIN Learning_signatures ON Learning_jobs.id=job_id " \
                    "AND Learning_jobs.step_id=Learning_applications.step_id AND Learning_signatures.id=signature_id "\
                    "WHERE "

void usage(char *name) 
{
    printf("\n");
    printf("Usage: %s\t [options].\t At least one option is required.\n", name);
    printf("Available options:\n"\
            "\t-n [node_name1, nn2]\t Specifies from which nodes the learning phase will be deleted from, or all for every entry. [default: all]\n"\
            "\t-j [job_id1, job_id2] \t Specifies which job_ids will be deleted from the learning phase. Only available for specific nodes.\n"\
            "\t-f [freq1, freq2]\t Specifies what frequencies will be deleted from the learning phase. Only available for specific nodes.\n"\
            "\t-p\t\t\t Requests database root password.\n"\
            "\t-r\t\t\t Runs the queries and deletes the entries.\n"\
            "\t-o\t\t\t Prints the queries that would be executed.\n"\
            "\t-h\t\t\t Shows this message.\n");
    printf("\n");
    exit(0);
}

int main(int argc,char *argv[])
{
	char passw[256];
    char node_name[256];
    char job_ids[256] = "";
    char freqs[256] = "";
    struct termios t;
    int opt, i;
	char all_nodes = 0;
    char to_run = 0;
	
    strcpy(passw, "");
	if (argc < 2) {
        usage(argv[0]);
    }
    
    while ((opt = getopt(argc, argv, "n:roj:f:hp")) != -1) {
        switch (opt) {
            case 'n':
                if (!strcmp(optarg, "all")) all_nodes = 1;
                else strcpy(node_name, optarg);
                break;
            case 'r':
                to_run = 1;
                break;
            case 'o':
                to_run = 0;
                break;
            case 'j':
                strcpy(job_ids, optarg);
                break;
            case 'f':
                strcpy(freqs, optarg);
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
                printf("\n");
                strclean(passw, '\n');
                break;
            case 'h':
                usage(argv[0]);
                break;
            default:
                printf("Unknown argument\n");
                break;
        }

    }
            
    char ear_path[256];
	cluster_conf_t my_conf;
    if (get_ear_conf_path(ear_path)==EAR_ERROR){
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path,&my_conf);
    int num_queries = 0;
    char **queries;
    
    if (all_nodes)
    {
        num_queries = 4;
        queries = calloc(num_queries, sizeof(char *));
        for (i = 0; i < num_queries; i++)
            queries[i] = calloc(1024, sizeof(char));

        strcpy(queries[0], ALL_QUERY1);
        strcpy(queries[1], ALL_QUERY2);
        strcpy(queries[2], ALL_QUERY3);
        strcpy(queries[3], ALL_QUERY4);
    }
    else
    {
        num_queries = 2;
        queries = calloc(num_queries, sizeof(char *));
        for (i = 0; i < num_queries; i++)
            queries[i] = calloc(1024, sizeof(char));

        /* Preparing node_list for query */
        char node_list[512];
        char *token;
        strcpy(node_list, " node_id IN (");
        token = strtok(node_name, ",");
        strcat(node_list, "\"");
        strcat(node_list, token);
        strcat(node_list, "\"");
        token = strtok(NULL, ",");
        while (token != NULL)
        {
            strcat(node_list, ",");
            strcat(node_list, "\"");
            strcat(node_list, token);
            strcat(node_list, "\"");
            token = strtok(NULL, ",");
        }
        strcat(node_list, ")");
        /* node_list finished */

        /*base queries and node_list */
        strcpy(queries[0], POW_SIG_QUERY);
        strcpy(queries[1], NODE_QUERY);

        strcat(queries[0], node_list);
        strcat(queries[1], node_list);

        /* if any job_id or frequency is specified, we add it to the query */
        if (strlen(job_ids) > 1) {
            strcat(queries[0], " AND job_id IN (");
            strcat(queries[0], job_ids);
            strcat(queries[0], ")" );

            strcat(queries[1], " AND job_id IN (");
            strcat(queries[1], job_ids);
            strcat(queries[1], ")" );
        }

        if (strlen(freqs) > 1) {
            strcat(queries[0], " AND Learning_jobs.def_f IN (");
            strcat(queries[0], freqs);
            strcat(queries[0], ")" );

            strcat(queries[1], " AND Learning_jobs.def_f IN (");
            strcat(queries[1], freqs);
            strcat(queries[1], ")" );
        }

        /*closing last parenthesis in power_signatures query"*/
        strcat(queries[0], ")");


    }
        

    if (!to_run)
    {
        for (i = 0; i < num_queries; i++)
            printf("%s\n", queries[i]);

    }
    else
    {
#if DB_MYSQL
        MYSQL *connection = mysql_init(NULL);
        
        if (connection == NULL)
        {
            verbose(0, "Error creating MYSQL object: %s", mysql_error(connection)); //error
            exit(1);
        }

        if(!mysql_real_connect(connection, my_conf.database.ip, "root", passw, my_conf.database.database, my_conf.database.port, NULL, 0))
        {
            verbose(0, "Error connecting to the database(%d):%s", mysql_errno(connection), mysql_error(connection)); //error
            mysql_close(connection);
            exit(0);
        }
        
        for (i = 0; i < num_queries; i++)
        {
            if (mysql_query(connection, queries[i]))
            {
                verbose(0, "Error with query: %s\n", queries[i]);
                verbose(0, "MYSQL error(%d): %s", mysql_errno(connection), mysql_error(connection)); //error
                exit(0);
            }
        }
        mysql_close(connection);

        if (all_nodes) {
            printf("Successfully deleted all records.\n");
        } else {
            printf("Successfully deleted all records for node %s.\n", node_name);
        }
#else
        printf("The learning_delete tool has only been implemented for MySQL databases.\n");
#endif
    }
	return 0;
}
