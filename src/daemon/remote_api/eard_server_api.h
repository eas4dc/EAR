/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_SERVER_API_H
#define _EARD_SERVER_API_H
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <common/messaging/msg_conf.h>
#include <common/types/configuration/cluster_conf.h>

void propagate_req(request_t *command, uint port);
int propagate_and_cat_data(request_t *command, uint port, void **status, size_t size, uint type, int num_items);
int propagate_status(request_t *command, uint port, status_t **status);
int propagate_app_status(request_t *command, uint port, app_status_t **status, int num_apps);
int propagate_release_idle(request_t *command, uint port, pc_release_data_t *release);
int propagate_powercap_status(request_t *command, uint port, powercap_status_t **status);
int propagate_get_power(request_t *command, uint port, power_check_t **power);
int propagate_policy_status(request_t *command, uint port, policy_status_t **status);

powercap_status_t *mem_alloc_powercap_status(char *final_data);
char *mem_alloc_char_powercap_status(powercap_status_t *status);

int init_ips(cluster_conf_t *my_conf);
void close_ips();

#endif
