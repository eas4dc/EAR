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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/remote_api/eard_rapi.h>

#ifndef _ENERGY_CLUSTER_STATUS_H
#define _ENERGY_CLUSTER_STATUS_H

#define EARGM_NO_PROBLEM 3
#define EARGM_WARNING1   2
#define EARGM_WARNING2   1
#define EARGM_PANIC      0

typedef struct node_info {
    uint dist_pstate;
    int ip;
    float power_red;
    uint victim;
    uint idle;
} node_info_t;

state_t get_nodes_status(cluster_conf_t my_cluster_conf, uint *nnodes, node_info_t **einfo);
void manage_warning(risk_t *risk, uint level, cluster_conf_t my_cluster_conf, float target, uint mode);
void create_risk(risk_t *my_risk, int wl);

#endif
