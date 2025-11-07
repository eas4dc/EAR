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
#include <common/string_enhanced.h>
#include <common/types/configuration/cluster_conf_eardbd.h>

state_t EARDBD_token(char *token)
{
    if (strcasestr(token, "EARDBD") != NULL)
        return EAR_SUCCESS;
    if (strcasestr(token, "DBDAEMON") != NULL)
        return EAR_SUCCESS;
    return EAR_ERROR;
}

state_t EARDBD_parse_token(eardb_conf_t *conf, char *token)
{
    state_t found = EAR_ERROR;

    if (!strcmp(token, "DBDAEMONAGGREGATIONTIME")) {
        token           = strtok(NULL, "=");
        conf->aggr_time = atoi(token);
        found           = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONINSERTIONTIME")) {
        token           = strtok(NULL, "=");
        conf->insr_time = atoi(token);
        found           = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONPORTTCP")) {
        token          = strtok(NULL, "=");
        conf->tcp_port = atoi(token);
        found          = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONPORTSECTCP")) {
        token              = strtok(NULL, "=");
        conf->sec_tcp_port = atoi(token);
        found              = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONSYNCPORT")) {
        token               = strtok(NULL, "=");
        conf->sync_tcp_port = atoi(token);
        found               = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONMEMORYSIZE")) {
        token          = strtok(NULL, "=");
        conf->mem_size = atoi(token);
        found          = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONMEMORYSIZEPERTYPE")) {
        int i = 0;

        token = strtok(NULL, "=");
        token = strtok(token, ",");

        while (token != NULL && i < EARDBD_TYPES) {
            conf->mem_size_types[i] = atoi(token);
            token                   = strtok(NULL, ",");
            i++;
        }
        found = EAR_SUCCESS;
    } else if (!strcmp(token, "DBDAEMONUSELOG")) {
        token         = strtok(NULL, "=");
        conf->use_log = atoi(token);
        found         = EAR_SUCCESS;
    } else if (!strcmp(token, "EARDBDREPORTPLUGINS")) {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        if (strlen(token) < sizeof(conf->plugins)) {
            strncpy(conf->plugins, token, sizeof(conf->plugins) - 1);
            found = EAR_SUCCESS;
        } else {
            found = EAR_WARNING;
        }
    }

    return found;
}

void copy_eardbd_conf(eardb_conf_t *dest, eardb_conf_t *src)
{
    memcpy(dest, src, sizeof(eardb_conf_t));
}

void set_default_eardbd_conf(eardb_conf_t *eardbdc)
{
    eardbdc->aggr_time     = DEF_DBD_AGGREGATION_TIME;
    eardbdc->insr_time     = DEF_DBD_INSERTION_TIME;
    eardbdc->tcp_port      = DEF_DBD_SERVER_PORT;
    eardbdc->sec_tcp_port  = DEF_DBD_MIRROR_PORT;
    eardbdc->sync_tcp_port = DEF_DBD_SYNCHR_PORT;
    eardbdc->mem_size      = DEF_DBD_ALLOC_MBS;
    eardbdc->use_log       = DEF_DBD_FILE_LOG;
    strcpy(eardbdc->plugins, "mysql.so");
}

void print_db_manager(eardb_conf_t *conf)
{
    verbosen(VCCONF, "--> EARDBD configuration\n");
    verbosen(
        VCCONF,
        "---> Insertion time %u\tAggregation time: %u\tTCP port: %u\tSec. TCP port: %u\tSync Port: %u\tCacheSize: %u\n",
        conf->insr_time, conf->aggr_time, conf->tcp_port, conf->sec_tcp_port, conf->sync_tcp_port, conf->mem_size);
    verbosen(VCCONF, "--> use_log %ui report plugins %s\n", conf->use_log, conf->plugins);
}
