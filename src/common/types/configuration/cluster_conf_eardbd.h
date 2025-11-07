/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARDBD_CONF_H
#define _EARDBD_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef struct eardb_conf {
    uint aggr_time;
    uint insr_time;
    uint tcp_port;
    uint sec_tcp_port;
    uint sync_tcp_port;
    uint mem_size;
    uint use_log;
    uchar mem_size_types[EARDBD_TYPES];
    char plugins[SZ_PATH_INCOMPLETE];
} eardb_conf_t;

state_t EARDBD_token(char *token);
state_t EARDBD_parse_token(eardb_conf_t *conf, char *token);
void copy_eardbd_conf(eardb_conf_t *dest, eardb_conf_t *src);
void set_default_eardbd_conf(eardb_conf_t *eargmc);
void print_db_manager(eardb_conf_t *conf);

#endif
