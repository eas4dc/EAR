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
