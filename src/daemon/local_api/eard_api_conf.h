/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EARD_LOCAL_API_INTERNS_H
#define EARD_LOCAL_API_INTERNS_H

#define _GNU_SOURCE
#include <sched.h>
#include <common/types/job.h>
#include <common/types/loop.h>
#include <common/types/generic.h>
#include <common/types/event_type.h>
#include <common/types/application.h>
#include <common/config/config_install.h>

// This class is intended to bring intern functions to connect to EARD.
// Including this class is only required for connection managing modules. If you
// want easy-to-read functions to ask for EARD request, look at eard_api.h class.

#define WAIT_DATA    0
#define NO_WAIT_DATA 1

typedef struct req_padding_s {
    uint  padding1;
    ulong padding2[MAX_CPUS_SUPPORTED];
} req_padding_t;

// Data type to send the requests
union daemon_req_opt {
    unsigned long   req_value;
    application_t   app;
	loop_t		    loop;
	ear_event_t     event;
	req_padding_t   padding; // Padding is just for compatibility
};

typedef struct app_id{
    ulong jid;
    ulong sid;
    ulong lid;
} app_id_t;

/* new_services : This section is eard request messages headers */
typedef struct eard_head_s {
    ulong    req_service;
    ulong    sec;
    size_t   size;
    state_t  state;
    app_id_t con_id;
} eard_head_t;

struct daemon_req {
    /* For new_services : This section must include same size than eard_head_t */
    ulong    req_service;
    ulong    sec;
    size_t   size;
    state_t  state;
    app_id_t con_id;
    union daemon_req_opt req_data;
};

int eards_read(int fd,char *buff, int size, uint type);

int eards_write(int fd,char *buff, int size);

// This function is to be used for external commands or applications not using the EARL.
int eards_connection();

// Tries to connect with the daemon. Returns 0 on success and -1 otherwise.
int eards_connect(application_t *my_app, ulong lid);

// True if the binary is connected with EARD.
int eards_connected();

// Closes the connection with EARD.
void eards_disconnect();

// Closes the FDs and sets the status to not connected. To be used only with multiprocess
void eards_new_process_disconnect();

#endif //EARD_LOCAL_API_INTERNS_H
