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

