/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <stdio.h>
#include <stdlib.h>

static char *buffer_serial;

static void serialize_deserialize_test(char *ear_path)
{
    cluster_conf_t my_cluster_ser, my_cluster_deser;
    size_t size_serial = { 0 };
    printf("reading ear.conf for serialization test\n");
    read_cluster_conf(ear_path, &my_cluster_ser);
    if (state_fail(serialize_cluster_conf(&my_cluster_ser, &buffer_serial, &size_serial))) {
        printf("Seralization fails\n");
        exit(1);
    }
    printf("Serialization ok: Serial size %u\n", size_serial);
    printf("Deserialization\n");

    version_t conf_version;
    version_set(&conf_version, 6, 0);

    if (state_fail(deserialize_cluster_conf(&my_cluster_deser, buffer_serial, size_serial, &conf_version))) {
        printf("Deserialization fails\n");
        exit(1);
    }
    printf("Deserialization ok\n");
    print_cluster_conf(&my_cluster_deser);
    return;
}

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
    // gethostname(nodename,sizeof(nodename));
    // strtok(nodename,".");
    int mode = 0;
    if (strlen(nodename) > 0) {
        eargm_def_t *edf;
        printf("Looking for %s in the list of EARGM\n", nodename);
        edf = get_eargm_conf(&my_cluster, nodename);
        if (edf != NULL) {
            printf("Found EARGM conf for node %s\n", nodename);
            remove_islands_by_eargm(&my_cluster, edf);
            printf("******************************* EARGM CONFIGURATION **********+\n");
            print_eargm_def(edf, 0);
            printf("*********************** EARGM-CLUSTER CONFIGURATION **********+\n");
            print_cluster_conf(&my_cluster);
        }
        my_node_conf = get_my_node_conf(&my_cluster, nodename);
        if (my_node_conf == NULL) {
            fprintf(stderr, "get_my_node_conf for node %s returns NULL\n", nodename);
        } else {
            printf("\nNODE_CONF FOR NODE: %s\n\n", nodename);
            print_my_node_conf(my_node_conf);
            printf("Default policy after filter is %u\n", my_cluster.default_policy);
            free(my_node_conf->policies);
            free(my_node_conf);
        }
        printf("Looking for %s in the list of EARDBDs\n", nodename);
        char dummy[SZ_NAME_MEDIUM];
        mode = get_node_server_mirror(&my_cluster, nodename, dummy);
        if (mode & 1) {
            printf("\nEARDBD server found in node %s.........\n", nodename);
        }
        if (mode & 2) {
            printf("\nEARDBD mirror found in node %s.........\n", nodename);
        }
        if (mode)
            print_db_manager(&my_cluster.db_manager);
    }
    printf("releasing cluster_conf\n");
    free_cluster_conf(&my_cluster);
    printf("freed cluster_conf\n");
    printf("reading cluster_conf again\n");
    read_cluster_conf(ear_path, &my_cluster);
    printf("freeing cluster_conf again\n");
    free_cluster_conf(&my_cluster);
    printf("freed cluster_conf\n");

    serialize_deserialize_test(ear_path);

    return 0;
}
