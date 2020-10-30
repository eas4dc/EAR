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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>

void propagate_sim(int target_idx, int *ips, int ip_count, int depth)
{
    int current_dist, off_ip, i, j;
    struct sockaddr_in temp;
    char next_ip[64];

    current_dist = target_idx - target_idx%NUM_PROPS;
    off_ip = target_idx%NUM_PROPS;

    for (i = 1; i <= NUM_PROPS; i++)
    {
        if ((current_dist*NUM_PROPS + i*NUM_PROPS + off_ip) >= ip_count) break;

        temp.sin_addr.s_addr = ips[current_dist*NUM_PROPS + i*NUM_PROPS + off_ip];
        strcpy(next_ip, inet_ntoa(temp.sin_addr));
        
        for (j = 0; j < depth; j++)
            printf("\t");

        printf("[%d][%d][%s] -> depth %d\n", i, j, next_ip, depth);
        propagate_sim(current_dist*NUM_PROPS + i*NUM_PROPS + off_ip, ips, ip_count, depth + 1);
    }

}

int initial_prop_sim(cluster_conf_t *conf)
{
    int i, j, total_ranges;
    int **ips, *ip_counts;

    int distance = 0;
    int depth = 0;
    struct sockaddr_in temp;
    char next_ip[64];

    total_ranges = get_ip_ranges(conf, &ip_counts, &ips);
    printf("\n[%s][%s][IP]\n", "R_ID", "IP_ID");
    for (i = 0; i < total_ranges; i++)
    {
        for (j = 0; j < ip_counts[i] && j < NUM_PROPS; j++)
        {
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(next_ip, inet_ntoa(temp.sin_addr));

            printf("[%d][%d][%s] -> Initial propagation (depth %d)\n", i, j, next_ip, depth);
            propagate_sim(j, ips[i], ip_counts[i], depth + 1);
            printf("\n");
        }
    }

}

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
	//print_cluster_conf(&my_cluster);

    initial_prop_sim(&my_cluster);
    free_cluster_conf(&my_cluster);

	return 0;
}
