/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_NODE_SERVICES_H_
#define _EARD_NODE_SERVICES_H_

#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>

#define CLOSE_EARD -100
#define CLOSE_PIPE -101

void clean_global_connector();

void eard_close_comm(int req_fd, int ack_fd);

state_t eard_local_api();

state_t service_close_by_id(ulong jid, ulong sid);

void services_init(topology_t *tp);

void services_print(topology_t *tp, cluster_conf_t *cluster_conf);

void services_dispose();

#endif
