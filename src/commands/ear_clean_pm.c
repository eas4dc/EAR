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

#if DB_MYSQL
#define CLEAN_PERIODIC_QUERY "DELETE FROM Periodic_metrics WHERE end_time <= UNIX_TIMESTAMP(SUBTIME(NOW(),'%d 0:0:0.00'))"
#elif DB_PSQL
#define CLEAN_PERIODIC_QUERY "DELETE FROM Periodic_metrics WHERE end_time <= (date_part('epoch',NOW())::int - (%d*24*3600))"
#endif

void print_version()
{
    char msg[256];
    sprintf(msg, "EAR version %s\n", RELEASE);
    printf(msg);
    exit(0);
}

void usage(char *app)
{
	printf("Usage:%s num_days [options]\n", app);
    printf("\t-p\t\tSpecify the password for MySQL's root user.\n");
    printf("The application will remove any Periodic metrics older than num_days\n");
	exit(0);
}
  
int main(int argc,char *argv[])
{
    char passw[256], query[256];
    int num_days;

    if (argc > 3 || argc < 2) usage(argv[0]);
    else if (argc == 3)
    {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        printf("Introduce root's password:");
        fflush(stdout);
        fgets(passw, sizeof(passw), stdin);
        t.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        strclean(passw, '\n');
        verbose(0, " ");
    }
    else
        strcpy(passw, "");

    num_days = atoi(argv[1]);
    if (num_days < 0 || num_days > 365)
    {
        verbose(0, "Invalid number of days."); //error
    }

    cluster_conf_t my_cluster;
    char ear_path[256];
    if (get_ear_conf_path(ear_path) == EAR_ERROR)
    {
        verbose(0, "Error getting ear.conf path"); //error
        exit(0);
    }

    read_cluster_conf(ear_path, &my_cluster);

    init_db_helper(&my_cluster.database);

    sprintf(query, CLEAN_PERIODIC_QUERY, num_days);

#if DB_MYSQL
    db_run_query(query, "root", passw); 
#elif DB_PSQL
    db_run_query(query, "postgres", passw); 
#endif

    free_cluster_conf(&my_cluster);

    verbose(0, "Database successfully cleaned.");

    exit(1);
}
