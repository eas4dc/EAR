/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>
#include <common/output/verbose.h>
#include <common/messaging/msg_conf.h>

#include <daemon/remote_api/eard_server_api.h>
#include <daemon/remote_api/eard_rapi_internals.h>
#include <daemon/remote_api/eard_rapi.h>

void ear_set_powerlimit(cluster_conf_t *my_cluster_conf, unsigned long limit, char **nodes, int num_nodes)
{
	if (nodes == NULL || num_nodes < 1)
		ear_cluster_set_powerlimit(my_cluster_conf, limit);
	else
		ear_nodelist_set_powerlimit(my_cluster_conf, limit, nodes, num_nodes);
}

int ear_get_status(cluster_conf_t *conf, status_t **status, char **nodes, int num_nodes)
{
	if (nodes == NULL || num_nodes < 1)
		return ear_cluster_get_status(conf, status);
	else
		return ear_nodelist_get_status(conf, status, nodes, num_nodes);
}

int ear_get_power(cluster_conf_t *my_cluster_conf, power_check_t *power, char **nodes, int num_nodes) 
{
	if (nodes == NULL || num_nodes < 1)
		return ear_cluster_get_power(my_cluster_conf, power);
	else
		return ear_nodelist_get_power(my_cluster_conf, power, nodes, num_nodes);
}

int ear_get_powercap_status(cluster_conf_t *conf, powercap_status_t **pc_status, int release_power, char **nodes, int num_nodes)
{
	if (nodes == NULL || num_nodes < 1)
		return ear_cluster_get_powercap_status(conf, pc_status, release_power);
	else
		return ear_nodelist_get_powercap_status(conf, pc_status, release_power, nodes, num_nodes);
}

int ear_get_app_master_status(cluster_conf_t *conf, app_status_t **app_status, char **nodes, int num_nodes)
{
	if (nodes == NULL || num_nodes < 1)
		return ear_cluster_get_app_master_status(conf, app_status);
	else
		return ear_nodelist_get_app_master_status(conf, app_status, nodes, num_nodes);
}

int ear_get_app_node_status(cluster_conf_t *conf, app_status_t **app_status, char **nodes, int num_nodes)
{
	if (nodes == NULL || num_nodes < 1)
		return ear_cluster_get_app_node_status(conf, app_status);
	else
		return ear_nodelist_get_app_node_status(conf, app_status, nodes, num_nodes);
}

int ear_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt, char **nodes, int num_nodes)
{
	if (nodes == NULL || num_nodes < 1)
		return ear_cluster_set_powercap_opt(my_cluster_conf, pc_opt);
	else
		return ear_nodelist_set_powercap_opt(my_cluster_conf, pc_opt, nodes, num_nodes);
}

