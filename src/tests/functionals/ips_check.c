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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <daemon/eard_server_api.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>

int self_id = -1, total_ips = -1, *ips = NULL;
char nodename[256];
char custom_node = 0;

int init_ips(cluster_conf_t *my_conf)
{
    printf("Initialising ips\n");
    int ret, i, temp_ip;
    char buff[64];
    if (custom_node)
        strcpy(buff, nodename);
    else
    {
        gethostname(buff, 64);
        strtok(buff,".");
    }
    printf("Searching for %s in the list\n", buff);
    ret = get_range_ips(my_conf, buff, &ips);
    if (ret < 1) {
        ips = NULL;
        self_id = -1;
        printf("Hostname wasn't found in any range of the configuration\n");
        return EAR_ERROR;
    }
    if (strlen(my_conf->net_ext) > 0)
    {
        strcat(buff, my_conf->net_ext);
        printf("Using network extension to find the ip, new hostname: %s\n", buff);
    }
    temp_ip = get_ip(buff, my_conf);
    struct in_addr addr =  {temp_ip};
    printf("Checking current ip (%s) against the list of ips from the configuration file.\n",inet_ntoa(addr));
    for (i = 0; i < ret; i++)
    {
        if (ips[i] == temp_ip)
        {
            self_id = i;
            addr.s_addr = ips[i]; 
            printf("Found IP, position %d on the list, ip: %d (%s)\n", self_id, ips[i], inet_ntoa(addr)); 
            break;
        }
    }
    if (self_id < 0)
    {
        free(ips);
        ips = NULL;
        printf("Couldn't find node in IP list\n");
        return EAR_ERROR;
    }
    total_ips = ret;
    return EAR_SUCCESS;
  
}

int main(int argc,char *argv[])
{
	cluster_conf_t my_cluster;
	my_node_conf_t *my_node_conf;
	char ear_path[256];
    if (argc > 1)
    {
        strcpy(nodename, argv[1]);
        custom_node = 1;
    }
    else
        gethostname(nodename, sizeof(nodename)); 
	if (get_ear_conf_path(ear_path)==EAR_ERROR){
		printf("Error getting ear.conf path\n");
		exit(0);
    }
	read_cluster_conf(ear_path,&my_cluster);

    int is_node_present = init_ips(&my_cluster);
    if (is_node_present == EAR_ERROR)
        printf("Node %s could not be found in the configuration file\n", nodename);


    free_cluster_conf(&my_cluster);
	return 0;
}
