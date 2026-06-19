/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#pragma once

#include <daemon/remote_api/eard_rapi.h>
#include <stdint.h>

#define POWER_TYPE    0
#define ENERGY_TYPE   1

#define MONITOR       0
#define HARD_POWERCAP 1
#define SOFT_POWERCAP 2

typedef struct shared_powercap_data {
    uint64_t status;
    uint64_t def_power;        // default power for this eargm
    uint64_t current_power;    // current power usage
    uint64_t current_powercap; // current maximum sub-cluster power
    uint64_t extra_power;      // extra power already allocated to this eargm
    uint64_t requested;        // extra power requested (>0 means that the eargm is in greedy mode)
    uint64_t available_power;  // power that can be freed
} shared_powercap_data_t;

typedef struct cluster_powercap_ctx {
    uint32_t total_req_new;
    uint32_t total_req_greedy;
    uint32_t req_no_extra;
    uint32_t num_no_extra;
    uint32_t num_greedy;
    uint32_t num_extra;
    uint32_t extra_power_alloc;
    uint32_t total_extra_power;
    uint32_t greedy_allocated;
    uint32_t total_free;
    uint32_t current_cluster_powercap;
} cluster_powercap_ctx_t;

#define cluster_powercap_status_t powercap_status_t

// void aggregate_powercap_status(powercap_status_t *my_cluster_power_status,int num_st,cluster_powercap_status_t
// *cluster_status);
// void allocate_free_power_to_greedy_nodes(cluster_powercap_status_t *cluster_status, powercap_opt_t *cluster_options,
// uint *total_free); void reduce_allocation(cluster_powercap_status_t *cluster_status, powercap_opt_t *cluster_options,
// uint min_reduction); uint powercap_reallocation(cluster_powercap_status_t *cluster_status, powercap_opt_t
// *cluster_options);
void send_powercap_options_to_cluster(powercap_opt_t *cluster_options);
void print_cluster_power_status(cluster_powercap_status_t *my_cluster_power_status);
uint cluster_get_min_power(cluster_conf_t *conf, char type);
void cluster_powercap_init(cluster_conf_t *cc);
int cluster_power_limited();
void cluster_hard_powercap();
void cluster_power_monitor();

void cluster_powercap_red_pc(uint red);
void cluster_powercap_inc_pc(uint inc);
void cluster_powercap_set_pc(uint limit);
void cluster_powercap_reset_pc();
