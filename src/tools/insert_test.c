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


void usage(char *name) 
{
    printf("\n");
    printf("Usage: %s\t\n", name);
    printf("\tThis will insert 10 applications with job_id current timestamp, step_ids from 0 to 1, application_name \n"\
            "\t'test_insert.current_time' and everything else to 0. The application will have a Signature, where the\n"\
            "\tavg_f is 112 (for testing purposes)\n");
    printf("\n");
    exit(0);
}

int main(int argc,char *argv[])
{
    
    int i;
    int num_apps = 10;
    char test_name[256];
    time_t ttime = time(NULL);
    char ear_path[256];
	cluster_conf_t my_conf;

    if (argc > 1) { usage(argv[0]); }
    
    //ear conf
    if (get_ear_conf_path(ear_path)==EAR_ERROR){
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path, &my_conf);

    //db init
	init_db_helper(&my_conf.database);

    sprintf(test_name, "test_insert.%ld", ttime);
    application_t *apps = calloc(num_apps, sizeof(application_t));

    for (i = 0; i < num_apps; i++) {
        apps[i].job.id = ttime;
        apps[i].job.step_id = i;
        strcpy(apps[i].job.app_id, test_name);
        apps[i].is_mpi = 1;
        apps[i].signature.avg_f = 112;
    }

    db_batch_insert_applications(apps, num_apps);


	free_cluster_conf(&my_conf);

	return 0;
}
