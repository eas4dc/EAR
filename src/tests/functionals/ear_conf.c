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
	if (argc>1){
		strcpy(ear_path,argv[1]);
	}else{
		if (get_ear_conf_path(ear_path)==EAR_ERROR){
			printf("Error getting ear.conf path\n");
			exit(0);
		}
	}
	read_cluster_conf(ear_path,&my_cluster);
	print_cluster_conf(&my_cluster);
	gethostname(nodename,sizeof(nodename));
	strtok(nodename,".");
    my_node_conf = get_my_node_conf(&my_cluster, nodename);
    if (my_node_conf==NULL) {
		fprintf(stderr,"get_my_node_conf for node %s returns NULL\n",nodename);
	}else print_my_node_conf(my_node_conf);

    free_cluster_conf(&my_cluster);
    free(my_node_conf);
    printf("freed cluster_conf\n");
    printf("reading cluster_conf again\n");
	read_cluster_conf(ear_path,&my_cluster);
    printf("freeing cluster_conf again\n");
    free_cluster_conf(&my_cluster);
    printf("freed cluster_conf\n");

	return 0;
}
