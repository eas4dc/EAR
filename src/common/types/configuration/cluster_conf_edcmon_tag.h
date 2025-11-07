/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EDCMONTAG_CONF_H
#define _EDCMONTAG_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef enum {
    storage    = 0,
    network    = 1,
    management = 2,
    others     = 3,
} pdu_t;

#define pdu_type_to_str(t)                                                                                             \
    ((t == storage) ? "storage" : ((t == network) ? "network" : ((t == management) ? "management" : "others")))

typedef struct edcmon {
    char id[GENERIC_NAME];
    char host[GENERIC_NAME];
    pdu_t type;
    char sensors_list[1024];
    char pdu_list[1024];
} edcmon_t;

state_t EDCMON_token(char *token);
state_t EDCMON_parse_token(edcmon_t **edcmon_i, unsigned int *num_edcmon_i, char *line);
void print_edcmon_tags_conf(edcmon_t *edcmon_tag, uint i);

#endif
