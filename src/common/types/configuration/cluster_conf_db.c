/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE

#include <common/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/string_enhanced.h>
#include <common/types/configuration/cluster_conf_db.h>
#include <common/types/generic.h>

state_t DB_token(char *token)
{
    if (strcasestr(token, "DB") != NULL)
        return EAR_SUCCESS;
    if (strcasestr(token, "MariaDB") != NULL)
        return EAR_SUCCESS;
    return EAR_ERROR;
}

state_t DB_parse_token(db_conf_t *conf, char *token)
{
    state_t found = EAR_ERROR;
    debug("DB_parse_token %s", token);
    if (!strcmp(token, "MARIADBIP") || !strcmp(token, "DBIP")) {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->ip, token);
        found = EAR_SUCCESS;
    } else if (!strcmp(token, "DBSECIP")) {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->sec_ip, token);
        found = EAR_SUCCESS;
    } else if (!strcmp(token, "MARIADBUSER") || !strcmp(token, "DBUSER")) {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->user, token);
        found = EAR_SUCCESS;
    } else if (!strcmp(token, "MARIADBPASSW") || !strcmp(token, "DBPASSW")) {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->pass, token);
        found = EAR_SUCCESS;
    } else if (!strcmp(token, "MARIADBCOMMANDSUSER") || !strcmp(token, "DBCOMMANDSUSER")) {
        found = EAR_SUCCESS;
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->user_commands, token);
    } else if (!strcmp(token, "MARIADBCOMMANDSPASSW") || !strcmp(token, "DBCOMMANDSPASSW")) {
        found = EAR_SUCCESS;
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->pass_commands, token);
    } else if (!strcmp(token, "MARIADBDATABASE") || !strcmp(token, "DBDATABASE")) {
        found = EAR_SUCCESS;
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->database, token);
    } else if (!strcmp(token, "MARIADBPORT") || !strcmp(token, "DBPORT")) {
        found      = EAR_SUCCESS;
        token      = strtok(NULL, "=");
        conf->port = atoi(token);
    } else if (!strcmp(token, "DBMAXCONNECTIONS")) {
        found                 = EAR_SUCCESS;
        token                 = strtok(NULL, "=");
        conf->max_connections = atoi(token);
    } else if (!strcmp(token, "DBREPORTNODEDETAIL")) {
        found                    = EAR_SUCCESS;
        token                    = strtok(NULL, "=");
        conf->report_node_detail = atoi(token);
    } else if (!strcmp(token, "DBREPORTSIGDETAIL")) {
        found                   = EAR_SUCCESS;
        token                   = strtok(NULL, "=");
        conf->report_sig_detail = atoi(token);
    } else if (!strcmp(token, "DBREPORTLOOPS")) {
        found              = EAR_SUCCESS;
        token              = strtok(NULL, "=");
        conf->report_loops = atoi(token);
    }
    debug("End DB token");
    return found;
}

void print_database_conf(db_conf_t *conf)
{
    verbosen(VCCONF, "\n--> DB configuration\n");
    verbosen(VCCONF, "---> IP: %s sec_ip %s \tUser: %s\tUser commands %s\tPort:%u\tDB:%s\n", conf->ip, conf->sec_ip,
             conf->user, conf->user_commands, conf->port, conf->database);
    verbosen(VCCONF, "-->max_connections %u report_node_details %u report_sig_details %u report_loops %u\n",
             conf->max_connections, conf->report_node_detail, conf->report_sig_detail, conf->report_loops);
}

void set_default_db_conf(db_conf_t *db_conf)
{
    strcpy(db_conf->user, "ear_daemon");
    strcpy(db_conf->database, "EAR");
    strcpy(db_conf->user_commands, "ear_commands");
    strcpy(db_conf->ip, "127.0.0.1");
    strcpy(db_conf->sec_ip, "");
    db_conf->port               = 3306;
    db_conf->max_connections    = MAX_DB_CONNECTIONS;
    db_conf->report_node_detail = 1;
    db_conf->report_sig_detail  = !DB_SIMPLE;
    db_conf->report_loops       = !LARGE_CLUSTER;
}

void copy_eardb_conf(db_conf_t *dest, db_conf_t *src)
{
    memcpy(dest, src, sizeof(db_conf_t));
}
