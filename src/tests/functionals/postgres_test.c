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
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <common/database/db_helper.h>
#include <common/types/configuration/cluster_conf.h>

int main(int argc,char *argv[])
{
	cluster_conf_t my_cluster;
	my_node_conf_t *my_node_conf;
	char nodename[256];
	char ear_path[256];
    int num_apps;
   
	if (argc>1){
		strcpy(ear_path,argv[1]);
	}else{
		if (get_ear_conf_path(ear_path)==EAR_ERROR){
			printf("Error getting ear.conf path\n");
			exit(0);
		}
	}
	read_cluster_conf(ear_path,&my_cluster);
//	print_cluster_conf(&my_cluster);
    
    init_db_helper(&my_cluster.database);

    application_t *apps;
    num_apps = db_read_applications_query(&apps, "SELECT * FROM Applications");
	
    int i;
    for (i = 0; i < num_apps; i++)
        printf("current app id: %lu\t step_id: %lu\t node_id: %s\t\n", apps[i].job.id, apps[i].job.step_id, apps[i].node_id);
    free_cluster_conf(&my_cluster);
    
	return 0;
}
