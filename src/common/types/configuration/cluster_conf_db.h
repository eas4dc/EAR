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

#ifndef _DB_CONF_H
#define _DB_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>


typedef struct db_conf
{
    char ip[USER];
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
state_t DB_parse_token(db_conf_t *conf,char *token);
void print_database_conf(db_conf_t *conf);
void set_default_db_conf(db_conf_t *db_conf);
void copy_eardb_conf(db_conf_t *dest,db_conf_t *src);



#endif
