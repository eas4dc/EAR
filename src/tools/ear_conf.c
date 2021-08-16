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
#include <common/types/configuration/cluster_conf.h>

int main(int argc,char *argv[])
{
	cluster_conf_t my_cluster;
	my_node_conf_t *my_node_conf;
	char nodename[256];
	char ear_path[256];
	strcpy(nodename,"");
	if (argc>1){
		strcpy(ear_path,argv[1]);
        if (argc > 2)
            strcpy(nodename, argv[2]);
	}else{
		if (get_ear_conf_path(ear_path)==EAR_ERROR){
			printf("Error getting ear.conf path\n");
			exit(0);
		}
	}
	read_cluster_conf(ear_path,&my_cluster);
	print_cluster_conf(&my_cluster);
    //gethostname(nodename,sizeof(nodename));
	//strtok(nodename,".");
    if (strlen(nodename) > 0)
    {
        my_node_conf = get_my_node_conf(&my_cluster, nodename);
        if (my_node_conf==NULL) {
          	fprintf(stderr,"get_my_node_conf for node %s returns NULL\n",nodename);
    	} else {
            printf("\nNODE_CONF FOR NODE: %s\n\n", nodename);
            print_my_node_conf(my_node_conf);
            free(my_node_conf->policies);
            free(my_node_conf);
        }
    }
    printf("releasing cluster_conf\n");
    free_cluster_conf(&my_cluster);
    printf("freed cluster_conf\n");
    printf("reading cluster_conf again\n");
	read_cluster_conf(ear_path,&my_cluster);
    printf("freeing cluster_conf again\n");
    free_cluster_conf(&my_cluster);
    printf("freed cluster_conf\n");

	return 0;
}

