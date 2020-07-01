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

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <common/config.h>
#include <common/states.h>
#include <daemon/eard_rapi.h>
#include <common/system/user.h>
#include <common/output/verbose.h>
#include <common/types/version.h>
#include <common/types/application.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/configuration/cluster_conf.h>
                   
#define NUM_LEVELS  4
#define MAX_PSTATE  16
#define IP_LENGTH   24

typedef struct ip_table
{
    int ip_int;
    char ip[IP_LENGTH];
    char name[IP_LENGTH];
    int counter;
		uint power;
    uint max_power;
    int job_id;
    int step_id;
    uint current_freq;
    uint temp;
		eard_policy_info_t policies[TOTAL_POLICIES];
} ip_table_t;

cluster_conf_t my_cluster_conf;

void fill_ip(char *buff, ip_table_t *table)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

   	s = getaddrinfo(buff, NULL, &hints, &result);
    if (s != 0) {
		printf("getaddrinfo fails for port %s (%s)", buff, strerror(errno));
		return;
    }


   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in*) (rp->ai_addr);
            //ina = saddr->sin_addr;    
            table->ip_int = saddr->sin_addr.s_addr;
            strcpy(table->ip, inet_ntoa(saddr->sin_addr));
        }
    }
    freeaddrinfo(result);
}

int generate_node_names(cluster_conf_t my_cluster_conf, ip_table_t **ips)
{
    int k;
    unsigned int i, j;
    char node_name[256];
    int num_ips = 0;
    my_node_conf_t *aux_node_conf;
    ip_table_t *new_ips = NULL;
    for (i = 0; i < my_cluster_conf.num_islands; i++){
        for (j = 0; j < my_cluster_conf.islands[i].num_ranges; j++)
        {   
            for (k = my_cluster_conf.islands[i].ranges[j].start; k <= my_cluster_conf.islands[i].ranges[j].end; k++)
            {   
                if (k == -1) 
                    sprintf(node_name, "%s", my_cluster_conf.islands[i].ranges[j].prefix);
                else if (my_cluster_conf.islands[i].ranges[j].end == my_cluster_conf.islands[i].ranges[j].start)
                    sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                else {
                    if (k < 10 && my_cluster_conf.islands[i].ranges[j].end > 10) 
                        sprintf(node_name, "%s0%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                    else 
                        sprintf(node_name, "%s%u", my_cluster_conf.islands[i].ranges[j].prefix, k); 
                }   
                new_ips = realloc(new_ips, sizeof(ip_table_t)*(num_ips+1));
                strcpy(new_ips[num_ips].name, node_name);
                aux_node_conf = get_my_node_conf(&my_cluster_conf, node_name);
                if (aux_node_conf == NULL) 
                    fprintf(stderr, "Error reading node %s configuration\n", node_name);
                else
                    new_ips[num_ips].max_power = (uint) aux_node_conf->max_error_power;

				/*#if USE_EXT
				strcat(node_name,NW_EXT);
				#endif*/
                if (strlen(my_cluster_conf.net_ext) > 0)
                    strcat(node_name, my_cluster_conf.net_ext);

                fill_ip(node_name, &new_ips[num_ips]);
                num_ips++;
            }
        }
    }
    *ips = new_ips;
    return num_ips;
}

void print_ips(ip_table_t *ips, int num_ips, char error_only)
{
    int i, j, counter = 0;
    if (!error_only)
    {
        printf("%10s\t%5s\t%4s\t%4s\t%6s\t%6s", "hostname", "power", "temp", "freq", "job_id", "stepid");
        printf("  %6s  %5s  %2s\n", "policy", "pfreq", "th");
    }
	char temp[GENERIC_NAME];
    char final[GENERIC_NAME];
    for (i=0; i<num_ips; i++)
	{
        if (ips[i].counter && ips[i].power != 0 )
        {
            if (!error_only) {
                    printf("%10s\t%5d\t%3dC\t%.2lf\t%6d\t%6d", ips[i].name, ips[i].power, ips[i].temp, 
                            (double)ips[i].current_freq/1000000.0, ips[i].job_id, ips[i].step_id);
	    	    for (j = 0; j < TOTAL_POLICIES; j++)
    		    {
			        policy_id_to_name(j, temp, &my_cluster_conf);
                    get_short_policy(final, temp, &my_cluster_conf);
		    	    printf("  %6s  %.2lf  %3u", final, (double)ips[i].policies[j].freq/1000000.0, ips[i].policies[j].th);
	    	    }
                printf("\n");
            }
            if (ips[i].power < ips[i].max_power)
                counter++;
            else if (ips[i].max_power == 0)
                counter++;
        }
	}
    if (counter < num_ips)
    {
        if (!error_only) printf("\n\nINACTIVE NODES\n");
        char first_node = 1;
        for (i = 0; i <num_ips; i++)
        {
            if (error_only)
            {
                if (!ips[i].counter || !ips[i].power || ips[i].power > ips[i].max_power)
                {
                    if (!first_node)
                        printf(",");
                    else first_node = 0;
                    printf("%8s", ips[i].name);
                }
            }
            else {
                if (!ips[i].counter) {
                    printf("%10s\t%10s\n", ips[i].name, ips[i].ip);
                } else if (!ips[i].power || ips[i].power > ips[i].max_power) {
                    printf("%10s\t%10s\t->power error (reported %dW)\n", ips[i].name, ips[i].ip, ips[i].power);
                }
	        }
        }
        printf("\n");
    }
}

void usage(char *app)
{
	printf("Usage: %s [options]"\
            "\n\t--set-freq \tnewfreq\t\t\t->sets the frequency of all nodes to the requested one"\
            "\n\t--set-def-freq \tnewfreq\tpolicy_name\t->sets the default frequency for the selected policy "\
            "\n\t--set-max-freq \tnewfreq\t\t\t->sets the maximum frequency"\
            "\n\t--inc-th \tnew_th\tpolicy_name\t->increases the threshold for all nodes"\
            "\n\t--set-th \tnew_th\tpolicy_name\t->sets the threshold for all nodes"\
            "\n\t--red-def-freq \tn_pstates\t\t->reduces the default and max frequency by n pstates"\
            "\n\t--restore-conf \t\t\t\t->restores the configuration to all node"\
            "\n\t--status \t\t\t\t->requests the current status for all nodes. The ones responding show the current "\
            "\n\t\t\t\t\t\t\tpower, IP address and policy configuration. A list with the ones not"\
            "\n\t\t\t\t\t\t\tresponding is provided with their hostnames and IP address."\
            "\n\t\t\t\t\t\t\t--status=node_name retrieves the status of that node individually."\
            "\n\t--ping	\t\t\t\t->pings all nodes to check wether the nodes are up or not. Additionally,"\
            "\n\t\t\t\t\t\t\t--ping=node_name pings that node individually."\
            "\n\t--version \t\t\t\t->displays current EAR version."\
            "\n\t--help \t\t\t\t\t->displays this message.", app);
    printf("\n\nThis app requires privileged access privileged accesss to execute.\n");
	exit(0);
}

void check_ip(status_t status, ip_table_t *ips, int num_ips)
{
    int i, j;
    for (i = 0; i < num_ips; i++)
        if (htonl(status.ip) == htonl(ips[i].ip_int))
		{
            ips[i].counter |= status.ok;
			ips[i].power = status.node.power;
            ips[i].current_freq = status.node.avg_freq;
            ips[i].temp = status.node.temp;
            ips[i].job_id = status.app.job_id;
            ips[i].step_id = status.app.step_id;
			//refactor
			for (j = 0; j < TOTAL_POLICIES; j++)
			{
				ips[i].policies[j].freq = status.policy_conf[j].freq;
				ips[i].policies[j].th = status.policy_conf[j].th;
			}	
		}
}

void clean_ips(ip_table_t *ips, int num_ips)
{
    int i = 0;
    for (i = 0; i < num_ips; i++)
        ips[i].counter=0;
}

void process_status(int num_status, status_t *status, char error_only)
{
    if (num_status > 0)
    {
        int i, num_ips;
        ip_table_t *ips = NULL;
        num_ips = generate_node_names(my_cluster_conf, &ips);
        clean_ips(ips, num_ips);
        for (i = 0; i < num_status; i++)
            check_ip(status[i], ips, num_ips);
        print_ips(ips, num_ips, error_only);
        free(status);
    }
    else printf("An error retrieving status has occurred.\n");
}

void generate_ip(ip_table_t *ips, char *node_name)
{
    my_node_conf_t *aux_node_conf;
    strcpy(ips[0].name, node_name);
    aux_node_conf = get_my_node_conf(&my_cluster_conf, node_name);
    if (aux_node_conf == NULL) 
        fprintf(stderr, "Error reading node %s configuration\n", node_name);
    else
        ips[0].max_power = (uint) aux_node_conf->max_error_power;
/*#if USE_EXT
    strcat(node_name,NW_EXT);
#endif*/
    if (strlen(my_cluster_conf.net_ext) > 0)
        strcat(node_name, my_cluster_conf.net_ext);
    fill_ip(node_name, &ips[0]);

}

void process_single_status(int num_status, status_t *status, char *node_name)
{
    if (num_status > 0)
    {
        ip_table_t ips;
        generate_ip(&ips, node_name);
        check_ip(*status, &ips, 1);
        print_ips(&ips, 1, 0);
    }
    else printf("An error retrieving status has occurred.\n");
}


int main(int argc, char *argv[])
{
    int c = 0;
    char path_name[128];
    status_t *status;
    int num_status = 0;
    verb_level = -1;
    verb_enabled = 0;
    if (argc < 2) usage(argv[0]);

    if (get_ear_conf_path(path_name)==EAR_ERROR){
        printf("Error getting ear.conf path\n"); //error
        exit(1);
    }

    if (read_cluster_conf(path_name, &my_cluster_conf) != EAR_SUCCESS) printf("ERROR reading cluster configuration\n");
    
    if (getuid() != 0 && !is_privileged_command(&my_cluster_conf))
    {
        printf("This command can only be executed by privileged users. Contact your admin for more info.\n");
        free_cluster_conf(&my_cluster_conf);
        exit(1); //error
    }

    while (1)
    {
        int option_idx = 0;
        static struct option long_options[] = {
            {"set-freq",     	required_argument, 0, 0},
            {"red-def-freq", 	required_argument, 0, 1},
            {"set-max-freq", 	required_argument, 0, 2},
            {"inc-th",       	required_argument, 0, 3},
            {"set-def-freq", 	required_argument, 0, 4},
            {"set-th",          required_argument, 0, 5},
            {"restore-conf", 	optional_argument, 0, 6},
	        {"ping", 	     	optional_argument, 0, 'p'},
            {"status",       	optional_argument, 0, 's'},
            {"error",           no_argument, 0, 'e'},
            {"help",         	no_argument, 0, 'h'},
            {"version",         no_argument, 0, 'v'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "p::shvb::", long_options, &option_idx);

        if (c == -1)
            break;

        int arg;
		int arg2;

        switch(c)
        {
            case 0:
                set_freq_all_nodes(atoi(optarg),my_cluster_conf);
                break;
            case 1:
                arg = atoi(optarg);
                red_def_max_pstate_all_nodes(arg,my_cluster_conf);
                break;
            case 2:
                arg = atoi(optarg);
                set_max_freq_all_nodes(arg,my_cluster_conf);
                break;
            case 3:
                arg = atoi(optarg);
				if (optind+1 > argc)
				{
					printf("Missing policy argument for set-def-freq\n");
                    break;
				}
				arg2 = policy_name_to_id(argv[optind], &my_cluster_conf);
				if (!is_valid_policy(arg2, &my_cluster_conf) || arg2 == EAR_ERROR)
				{
					printf("Invalid policy (%s).\n", argv[optind]);
                    break;
				}
                if (arg > 100)
                {
                    printf("Indicated threshold increase above theoretical maximum (100%%)\n");
                    break;
                }
                increase_th_all_nodes(arg, arg2, my_cluster_conf);
                break;
            case 4:
                arg = atoi(optarg);
				if (optind+1 > argc)
				{
					printf("Missing policy argument for set-def-freq\n");
                    break;
				}
				arg2 = policy_name_to_id(argv[optind], &my_cluster_conf);
				if (!is_valid_policy(arg2, &my_cluster_conf) || arg2 == EAR_ERROR)
				{
					printf("Invalid policy (%s).\n", argv[optind]);
                    break;
				}
                set_def_freq_all_nodes(arg, arg2, my_cluster_conf);
                break;
            case 5:
                arg = atoi(optarg);
                if (arg > 100)
                {
                    printf("Indicated threshold increase above theoretical maximum (100%%)\n");
                    break;
                }
                if (optind+1 > argc)
				{
					printf("Missing policy argument for set-def-freq\n");
                    break;
				}
				arg2 = policy_name_to_id(argv[optind], &my_cluster_conf);
				if (!is_valid_policy(arg2, &my_cluster_conf) || arg2 == EAR_ERROR)
				{
					printf("Invalid policy (%s).\n", argv[optind]);
                    break;
				}
                set_th_all_nodes(arg, arg2, my_cluster_conf);
                break;
            case 6:
                if (optarg)
                {
                    char node_name[256];
                    strcpy(node_name, optarg);
                    strtoup(node_name);
                    if (strcmp(node_name, optarg))
                    {
                        strcpy(node_name, optarg);
                        strcat(node_name, my_cluster_conf.net_ext);
                    }
                    else strcpy(node_name, optarg);
                    int rc=eards_remote_connect(node_name, my_cluster_conf.eard.port);
                    if (rc<0){
                        printf("Error connecting with node %s\n", node_name);
                    }else{
                        if (!eards_restore_conf()) printf("Eroor restoring configuration for node: %s\n", node_name);
                        eards_remote_disconnect();
                    }
                }
                else
                {
                    restore_conf_all_nodes(my_cluster_conf);
                }
                break;
            case 'p':
                if (optarg)
                {
                    char node_name[256];
                    strcpy(node_name, optarg);
                    strtoup(node_name);
                    if (strcmp(node_name, optarg))
                    {
                        strcpy(node_name, optarg);
                        strcat(node_name, my_cluster_conf.net_ext);
                    }
                    else strcpy(node_name, optarg);
                    int rc=eards_remote_connect(node_name, my_cluster_conf.eard.port);
                    if (rc<0){
                        printf("Error connecting with node %s\n", node_name);
                    }else{
                        verbose(1,"Node %s ping!\n", node_name);
                        if (!eards_ping()) printf("Error doing ping for node %s\n", node_name);
                        eards_remote_disconnect();
                    }
                }
                else
                    old_ping_all_nodes(my_cluster_conf);
                break;
            case 's':
                if (optarg)
                {
                    char node_name[256];
                    strcpy(node_name, optarg);
                    strtoup(node_name);
                    if (strcmp(node_name, optarg))
                    {
                        strcpy(node_name, optarg);
                        strcat(node_name, my_cluster_conf.net_ext);
                    }
                    else strcpy(node_name, optarg);
                    int rc=eards_remote_connect(node_name, my_cluster_conf.eard.port);
                    if (rc<0){
                        printf("Error connecting with node %s\n", node_name);
                    }else{
                        request_t command;
                        command.req = EAR_RC_STATUS;
                        command.node_dist = INT_MAX;
                        if ((num_status = send_status(&command, &status)) != 1) printf("Error doing status for node %s, returned (%d)\n", node_name, num_status);
                        process_single_status(num_status, status, node_name);
                        eards_remote_disconnect();
                    }
                }
                else
                {
                    num_status = status_all_nodes(my_cluster_conf, &status);
                    process_status(num_status, status, 0);
                }
                break;
            case 'e':
                num_status = status_all_nodes(my_cluster_conf, &status);
                process_status(num_status, status, 1);
                break;
            case 'h':
                usage(argv[0]);
                break;
            case 'v':
                print_version();
                break;
            case 'b':
                verb_enabled = 1;
                if (optarg) verb_level = atoi(optarg);
                else verb_level = 1;


        }
    }

    free_cluster_conf(&my_cluster_conf);
}
