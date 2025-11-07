/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _XOPEN_SOURCE 700 // to get rid of the warning
#define _GNU_SOURCE

#ifndef META_EARGM
#define META_EARGM

#include <common/config.h>
#include <common/states.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
// #define SHOW_DEBUGS 0
#include <common/messaging/msg_internals.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <daemon/remote_api/eard_rapi.h>

extern uint total_nodes;
extern uint my_port;

typedef struct eargm_table {
    uint num_eargms;
    int *ids;
    int *eargm_ips;
    int *eargm_ports;
    int *actions;
    uint *action_values;

} eargm_table_t;

void *meta_eargm(void *config);

#endif
