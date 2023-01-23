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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
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
#include <common/types/version.h>
#include <common/types/configuration/cluster_conf.h>

#if DB_MYSQL
#define CLEAN_PERIODIC_QUERY "DELETE FROM Periodic_metrics WHERE end_time <= UNIX_TIMESTAMP(SUBTIME(NOW(),'%d 0:0:0.00'))"
#elif DB_PSQL
#define CLEAN_PERIODIC_QUERY "DELETE FROM Periodic_metrics WHERE end_time <= (date_part('epoch',NOW())::int - (%d*24*3600))"
#endif

#define OUT_QUERY 1
#define RUN_QUERY 2

void usage(char *app)
{
	printf("Usage:%s [options]\n", app);
    printf("\t-d num_days\t\tREQUIRED: Specify how many days will be kept in database. (defaut: 0 days).\n");
    printf("\t-p\t\t\tSpecify the password for MySQL's root user.\n");
    printf("\t-o\t\t\tPrint the query instead of running it (default: off).\n");
    printf("\t-r\t\t\tExecute the query (default: on).\n");
    printf("\t-h\t\t\tDisplay this message.\n");
    printf("\t-v\t\t\tShow current EAR version.\n");
    printf("The application will remove any Periodic metrics older than num_days\n");
	exit(0);
}
  
int main(int argc,char *argv[])
{
    char passw[256], query[256];
    int num_days = -1;
	char out_type = RUN_QUERY, opt;

	strcpy(passw, "");
#if PRIVATE_PASS
	struct termios t;
	while ((opt = getopt(argc, argv, "u:pd:j:ovalrh")) != -1) {
#else
		while ((opt = getopt(argc, argv, "u:p:d:j:ovalrh")) != -1) {
#endif
			switch(opt)
			{
				case 'd':
					num_days = atoi(optarg);
					break;
				case 'p':
#if PRIVATE_PASS
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
#else
					strcpy(passw, optarg);
#endif
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
				case 'v':
					print_version();
					break;
			}
		}

		if (num_days < 0) {
			printf("Specifying the numbers of days to keep in storage is required. \n\n");
			usage(argv[0]);
		}

		cluster_conf_t my_cluster;
		char ear_path[256];
		if (get_ear_conf_path(ear_path) == EAR_ERROR)
		{
			printf("Error getting ear.conf path\n"); //error
			exit(0);
		}

		read_cluster_conf(ear_path, &my_cluster);

		init_db_helper(&my_cluster.database);

		sprintf(query, CLEAN_PERIODIC_QUERY, num_days);

		if (out_type == RUN_QUERY) {
#if DB_MYSQL
			db_run_query(query, "root", passw); 
#elif DB_PSQL
			db_run_query(query, "postgres", passw); 
#endif
			printf("Database successfully cleaned.\n");
		} else {
			printf("Query to run: %s\n", query);
		}

		free_cluster_conf(&my_cluster);


		exit(1);
	}
