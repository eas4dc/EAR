/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
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
