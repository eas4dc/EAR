/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/messaging/msg_conf.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/job.h>

#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_rapi_internals.h>
#include <daemon/remote_api/eard_server_api.h>

state_t ear_set_powerlimit(cluster_conf_t *my_cluster_conf, unsigned long limit, char **nodes, int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_set_powerlimit(my_cluster_conf, limit);
    else
        return ear_nodelist_set_powerlimit(my_cluster_conf, limit, nodes, num_nodes);
}

state_t ear_get_policy_status(cluster_conf_t *conf, policy_status_t **status, int32_t *num_status, char **nodes,
                              int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_get_policy_status(conf, status, num_status);
    else
        return ear_nodelist_get_policy_status(conf, status, num_status, nodes, num_nodes);
}

state_t ear_get_status(cluster_conf_t *conf, status_t **status, int32_t *num_status, char **nodes, int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_get_status(conf, status, num_status);
    else
        return ear_nodelist_get_status(conf, status, num_status, nodes, num_nodes);
}

state_t ear_get_power(cluster_conf_t *my_cluster_conf, power_check_t *power, char **nodes, int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_get_power(my_cluster_conf, power);
    else
        return ear_nodelist_get_power(my_cluster_conf, power, nodes, num_nodes);
}

state_t ear_get_powercap_status(cluster_conf_t *conf, powercap_status_t **pc_status, int release_power, char **nodes,
                                int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_get_powercap_status(conf, pc_status, release_power);
    else
        return ear_nodelist_get_powercap_status(conf, pc_status, release_power, nodes, num_nodes);
}

state_t ear_get_app_master_status(cluster_conf_t *conf, app_status_t **app_status, int32_t *num_status, char **nodes,
                                  int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_get_app_master_status(conf, app_status, num_status);
    else
        return ear_nodelist_get_app_master_status(conf, app_status, num_status, nodes, num_nodes);
}

state_t ear_get_app_node_status(cluster_conf_t *conf, app_status_t **app_status, int32_t *num_status, char **nodes,
                                int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_get_app_node_status(conf, app_status, num_status);
    else
        return ear_nodelist_get_app_node_status(conf, app_status, num_status, nodes, num_nodes);
}

state_t ear_set_powercap_opt(cluster_conf_t *my_cluster_conf, powercap_opt_t *pc_opt, char **nodes, int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_set_powercap_opt(my_cluster_conf, pc_opt);
    else
        return ear_nodelist_set_powercap_opt(my_cluster_conf, pc_opt, nodes, num_nodes);
}

state_t ear_send_message(cluster_conf_t *conf, char *message, char **nodes, int num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_send_message(conf, message);
    else
        return ear_nodelist_send_message(conf, message, nodes, num_nodes);
}

state_t ear_restore_conf(cluster_conf_t *conf, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_restore_conf(conf);
    else
        return ear_nodelist_restore_conf(conf, nodes, num_nodes);
}

state_t ear_set_risk(cluster_conf_t *conf, risk_t risk, uint32_t target, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1)
        return ear_cluster_set_risk(conf, risk, target);
    else
        return ear_nodelist_set_risk(conf, risk, target, nodes, num_nodes);
}

state_t ear_set_max_freq(cluster_conf_t *conf, uint32_t freq, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1) {
        return ear_cluster_set_max_freq(conf, freq);
    } else {
        for (int32_t i = 0; i < num_nodes; i++) {
            int32_t fd = remote_connect(nodes[i], conf->eard.port);
            ear_node_set_max_freq(freq);
            remote_disconnect_fd(fd);
        }
    }
    return EAR_SUCCESS;
}

state_t ear_set_freq(cluster_conf_t *conf, uint32_t freq, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1) {
        return ear_cluster_set_freq(conf, freq);
    } else {
        for (int32_t i = 0; i < num_nodes; i++) {
            int32_t fd = remote_connect(nodes[i], conf->eard.port);
            ear_node_set_freq(freq);
            remote_disconnect_fd(fd);
        }
    }
    return EAR_SUCCESS;
}

state_t ear_set_def_freq(cluster_conf_t *conf, uint32_t freq, int32_t policy_id, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1) {
        return ear_cluster_set_def_freq(conf, freq, policy_id);
    } else {
        for (int32_t i = 0; i < num_nodes; i++) {
            int32_t fd = remote_connect(nodes[i], conf->eard.port);
            ear_node_set_def_freq(freq, policy_id);
            remote_disconnect_fd(fd);
        }
    }
    return EAR_SUCCESS;
}

state_t ear_red_max_and_def_freq(cluster_conf_t *conf, uint32_t pstates, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1) {
        return ear_cluster_red_def_max_pstate(conf, pstates);
    } else {
        for (int32_t i = 0; i < num_nodes; i++) {
            int32_t fd = remote_connect(nodes[i], conf->eard.port);
            ear_node_red_max_and_def_freq(pstates);
            remote_disconnect_fd(fd);
        }
    }
    return EAR_SUCCESS;
}

state_t ear_ping(cluster_conf_t *conf, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1) {
        return ear_cluster_ping(conf);
    } else {
        for (int32_t i = 0; i < num_nodes; i++) {
            int32_t fd = remote_connect(nodes[i], conf->eard.port);
            ear_node_ping();
            remote_disconnect_fd(fd);
        }
    }
    return EAR_SUCCESS;
}

state_t ear_release_idle_power(cluster_conf_t *conf, pc_release_data_t *released, char **nodes, int32_t num_nodes)
{
    if (nodes == NULL || num_nodes < 1) {
        return ear_cluster_release_idle_power(conf, released);
    } else {
#warning release_idle_power not implemented for nodelists
    }
    return 0;
}
