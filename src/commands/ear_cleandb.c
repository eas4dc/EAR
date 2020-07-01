/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*   
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*   
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING  
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
	verbose(0, "Usage:%s num_days [options]", app);
    verbose(0, "\t-p\t\tSpecify the password for MySQL's root user.");
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
