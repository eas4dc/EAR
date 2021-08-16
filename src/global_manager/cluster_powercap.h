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

#ifndef CLUSTER_POWERCAP_H
#define CLUSTER_POWERCAP_H
#include <daemon/remote_api/eard_rapi.h>


#define cluster_powercap_status_t powercap_status_t

//void aggregate_powercap_status(powercap_status_t *my_cluster_power_status,int num_st,cluster_powercap_status_t *cluster_status);
void allocate_free_power_to_greedy_nodes(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint *total_free);
void reduce_allocation(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint min_reduction);
uint powercap_reallocation(cluster_powercap_status_t *cluster_status,powercap_opt_t *cluster_options,uint released);
void send_powercap_options_to_cluster(powercap_opt_t *cluster_options);
void print_cluster_power_status(cluster_powercap_status_t *my_cluster_power_status);
void cluster_powercap_init();
int cluster_power_limited();
void cluster_check_powercap();
void cluster_power_monitor();
#endif
