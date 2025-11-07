/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _DB_CONF_H
#define _DB_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef struct db_conf {
    char ip[USER];
    char sec_ip[USER];
    char user[USER];
    char pass[USER];
    char user_commands[USER];
    char pass_commands[USER];
    char database[USER];
    uint port;
    uint max_connections;
    uint report_node_detail;
    uint report_sig_detail;
    uint report_loops;
} db_conf_t;

state_t DB_token(char *token);
state_t DB_parse_token(db_conf_t *conf, char *token);
void print_database_conf(db_conf_t *conf);
void set_default_db_conf(db_conf_t *db_conf);
void copy_eardb_conf(db_conf_t *dest, db_conf_t *src);

#endif
