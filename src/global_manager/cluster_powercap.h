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

#ifndef CLUSTER_POWERCAP_H
#define CLUSTER_POWERCAP_H
#include <daemon/remote_api/eard_rapi.h>

#define POWER_TYPE 0
#define ENERGY_TYPE 1

#define MONITOR       0
#define HARD_POWERCAP 1
#define SOFT_POWERCAP 2

typedef struct shared_powercap_data {
    ulong status;
    ulong def_power;        // default power for this eargm
    ulong current_power;    // current power usage
    ulong current_powercap; // current maximum sub-cluster power 
    ulong extra_power;      // extra power already allocated to this eargm
    ulong requested;        // extra power requested (>0 means that the eargm is in greedy mode)
    ulong available_power;  // power that can be freed
} shared_powercap_data_t;


#define cluster_powercap_status_t powercap_status_t

//void aggregate_powercap_status(powercap_status_t *my_cluster_power_status,int num_st,cluster_powercap_status_t *cluster_status);
void allocate_free_power_to_greedy_nodes(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint *total_free);
void reduce_allocation(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint min_reduction);
uint powercap_reallocation(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options);
void send_powercap_options_to_cluster(powercap_opt_t *cluster_options);
void print_cluster_power_status(cluster_powercap_status_t *my_cluster_power_status);
uint cluster_get_min_power(cluster_conf_t *conf, char type);
void cluster_powercap_init();
int cluster_power_limited();
void cluster_check_powercap();
void cluster_power_monitor();

void cluster_powercap_red_pc(uint red);
void cluster_powercap_inc_pc(uint inc);
void cluster_powercap_set_pc(uint limit);
void cluster_powercap_reset_pc();
#endif
