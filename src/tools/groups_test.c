/*
 *
 * This program is part of the EAR software.
 *
 * EAR provides a dynamic, transparent and ligth-weigth solution for
 * Energy management. It has been developed in the context of the
 * Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
 *
 *
 * BSC Contact   mailto:ear-support@bsc.es
 *
 *
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
 * and COPYING.EPL files.
 */

#include <common/config.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <stdio.h>
#include <stdlib.h>

/* Usage: groups_test <ear.conf file> f */

int main(int argc, char *argv[])
{
    cluster_conf_t my_cluster;
    my_node_conf_t *my_node_conf;
    char nodename[256];
    char ear_path[256];
    strcpy(nodename, "");
    verb_channel = STDOUT_FILENO;
    if (argc > 1) {
        strcpy(ear_path, argv[1]);
        if (argc > 2)
            strcpy(nodename, argv[2]);
    } else {
        if (get_ear_conf_path(ear_path) == EAR_ERROR) {
            printf("Error getting ear.conf path\n");
            exit(0);
        }
    }
    read_cluster_conf(ear_path, &my_cluster);
    printf("******************************* GENERIC CONFIGURATION **********+\n");
    print_cluster_conf(&my_cluster);

    // 1. TEST FOR MY USER, WHICH IS IN THE USERLIST
    // 2. TEST FOR MY USER, WHICH IS NOT IN THE USER LIST, BUT IN AN AUTHORIZED GROUP
    // 3. TEST FOR MY USER, WHICH IS NOT ANYWHERE

    int res = is_authorized_usr_grp_acc(&my_cluster, "pmorillas", NULL, NULL);
    if (!res)
        printf("user not authorized\n");
    else
        printf("user auhorized!\n");

    return 0;
}
