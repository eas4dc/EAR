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

#include <common/system/sockets.h>
#include <common/types/types.h>

typedef struct edb_state {
    state_t server;
    state_t mirror;
} edb_state_t;

#define edb_state_init(state, val) \
        state.server = state.mirror = val;

#define edb_state_return(state, val) \
        state.server = val; \
        state.mirror = val; \
        return state;

#define edb_state_return_msg(state, val, msg) \
        intern_error_num = 0;   \
        intern_error_str = msg; \
        edb_state_return(state, val)

#define edb_state_fail(state) \
        state_fail(state.server) || state_fail(state.mirror)

edb_state_t eardbd_connect(cluster_conf_t *conf, my_node_conf_t *node);
edb_state_t eardbd_reconnect(cluster_conf_t *conf, my_node_conf_t *node, edb_state_t state);
edb_state_t eardbd_disconnect();

edb_state_t eardbd_send_ping();
edb_state_t eardbd_send_periodic_metric(periodic_metric_t *met);
edb_state_t eardbd_send_application(application_t *app);
edb_state_t eardbd_send_loop(loop_t *loop);
edb_state_t eardbd_send_event(ear_event_t *eve);

#endif //EAR_EARDBD_API_H
