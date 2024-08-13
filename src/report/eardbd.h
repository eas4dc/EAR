/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_EARDBD_API_H
#define EAR_EARDBD_API_H

#include <common/types/types.h>
#include <common/system/sockets.h>
#include <database_cache/eardbd.h>

/** Tests the state of an EARDBD process in a specific node. */
state_t eardbd_status(char *node, uint port, eardbd_status_t *status);

state_t eardbd_connect(cluster_conf_t *conf, my_node_conf_t *node);

state_t eardbd_reconnect(cluster_conf_t *conf, my_node_conf_t *node);

state_t eardbd_disconnect();

state_t eardbd_send_periodic_metric(periodic_metric_t *met);

state_t eardbd_send_application(application_t *app);

state_t eardbd_send_loop(loop_t *loop);

state_t eardbd_send_event(ear_event_t *eve);

#endif //EAR_EARDBD_API_H
