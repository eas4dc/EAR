/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _DYN_CONF_PARALLEL_H_
#define _DYN_CONF_PARALLEL_H_

#include <common/config.h>
#include <common/states.h>

state_t init_active_connections_list();
state_t notify_new_connection(int fd);
state_t add_new_connection();
state_t remove_remote_connection(int fd);
void *process_remote_req_th(void *arg);

#endif
