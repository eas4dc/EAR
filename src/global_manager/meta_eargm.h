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

#define _XOPEN_SOURCE 700 //to get rid of the warning
#define _GNU_SOURCE 

#ifndef META_EARGM
#define META_EARGM

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <common/states.h>
#include <common/config.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/messaging/msg_internals.h>
#include <daemon/remote_api/eard_rapi.h>


extern uint total_nodes;
extern uint my_port;

typedef struct eargm_table
{
    uint num_eargms;
    int  *ids;
    int  *eargm_ips;
    int  *eargm_ports;
    int  *actions;
    uint *action_values;

} eargm_table_t;

void *meta_eargm(void *config);

#endif
