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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef _DYN_CONF_PARALLEL_H_
#define _DYN_CONF_PARALLEL_H_

#include <common/config.h>
#include <common/states.h>

state_t init_active_connections_list();
state_t notify_new_connection(int fd);
state_t add_new_connection();
state_t remove_remote_connection(int fd);
void * process_remote_req_th(void * arg);

#endif
