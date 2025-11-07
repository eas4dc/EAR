/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <arpa/inet.h>
#include <common/config.h>
#include <common/messaging/msg_internals.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_rapi_internals.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>

void seq_ear_cluster_ping(cluster_conf_t *my_cluster_conf);

int main(int argc, char *argv[])
{
    cluster_conf_t my_cluster;
    char ear_path[256];
    if (get_ear_conf_path(ear_path) == EAR_ERROR) {
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path, &my_cluster);
    seq_ear_cluster_ping(&my_cluster);
}

void seq_ear_cluster_ping(cluster_conf_t *my_cluster_conf)
{
    int32_t rc = 0;
    char node_name[256];
    printf("Sendind ping to all nodes");
    // it is always secuential as it is only used for debugging purposes

    int32_t *ip_counts      = NULL;
    int32_t **ips           = NULL;
    char ***names           = NULL;
    struct sockaddr_in temp = {0};

    int32_t total_ranges = get_ip_and_names_from_ranges(my_cluster_conf, &ip_counts, &ips, &names);
    for (int32_t i = 0; i < total_ranges; i++) {
        for (int32_t j = 0; j < ip_counts[i]; j++) {
            temp.sin_addr.s_addr = ips[i][j];
            strcpy(node_name, inet_ntoa(temp.sin_addr));
            rc = remote_connect(node_name, my_cluster_conf->eard.port);

            if (rc < 0) {
                printf("Error connecting with node %s (IP: %s)\n", names[i][j], node_name);
            } else {
                debug("Node %s ping!", node_name);
                if (!ear_node_ping())
                    printf("Error sending ping to node %s (IP: %s)\n", names[i][j], node_name);
                remote_disconnect();
            }
        }
    }
}
