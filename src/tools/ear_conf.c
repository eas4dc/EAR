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
    verb_channel = STDOUT_FILENO;
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
    printf("******************************* GENERIC CONFIGURATION **********+\n");
    print_cluster_conf(&my_cluster);
    //gethostname(nodename,sizeof(nodename));
    //strtok(nodename,".");
    int mode = 0;
    if (strlen(nodename) > 0)
    {
        eargm_def_t *edf;
        printf("Looking for %s in the list of EARGM\n", nodename);
        edf = get_eargm_conf(&my_cluster, nodename);
        if (edf != NULL) {
            printf("Found EARGM conf for node %s\n", nodename);
            remove_extra_islands(&my_cluster, edf);
            printf("******************************* EARGM CONFIGURATION **********+\n");
            print_eargm_def(edf, 0);
            printf("*********************** EARGM-CLUSTER CONFIGURATION **********+\n");
            print_cluster_conf(&my_cluster);
        }
        my_node_conf = get_my_node_conf(&my_cluster, nodename);
        if (my_node_conf==NULL) {
            fprintf(stderr,"get_my_node_conf for node %s returns NULL\n",nodename);
        } else {
            printf("\nNODE_CONF FOR NODE: %s\n\n", nodename);
            print_my_node_conf(my_node_conf);
            free(my_node_conf->policies);
            free(my_node_conf);
        }
        printf("Looking for %s in the list of EARDBDs\n", nodename);
        char dummy[SZ_NAME_MEDIUM];
        mode = get_node_server_mirror(&my_cluster, nodename, dummy);
        if (mode && 1) {
            printf("\nEARDBD server found in node %s.........\n", nodename);
        } if (mode && 2) {
            printf("\nEARDBD mirror found in node %s.........\n", nodename);
        }
        if (mode) print_db_manager(&my_cluster.db_manager);
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


