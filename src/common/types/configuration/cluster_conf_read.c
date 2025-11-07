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
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_db.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_etag.h>
#include <common/types/configuration/cluster_conf_generic.h>
#include <common/utils/sched_support.h>

#ifndef INCLUDE_CONF
#define INCLUDE_CONF 0
#endif

#if 0
static void insert_th_policy(cluster_conf_t *conf, char *token, int policy, int main)
{
    int i;
    for (i = 0; i < conf->num_policies; i++)
    {
        if (conf->power_policies[i].policy == policy)
        {
            //We assume the policies will only have one setting in this case, so we change that one
            if (main)
                conf->power_policies[i].settings[0] = atof(token);
        }
    }

}
#endif

static int set_nodes_conf(cluster_conf_t *conf, char *namelist)
{

    conf->nodes = realloc(conf->nodes, sizeof(node_conf_t) * (conf->num_nodes + 1));
    if (conf->nodes == NULL) {
        error("NULL pointer in set_nodes_conf");
        return 0;
    }
    memset(&conf->nodes[conf->num_nodes], 0, sizeof(node_conf_t));

    range_def_t *aux_r_def = NULL;
    int32_t range_count    = 0;

    range_count = parse_range_list(namelist, &aux_r_def);

    for (int32_t i = 0; i < range_count; i++) {
        conf->nodes[conf->num_nodes].range = realloc(
            conf->nodes[conf->num_nodes].range, sizeof(node_range_t) * (conf->nodes[conf->num_nodes].range_count + 1));
        if (conf->nodes[conf->num_nodes].range == NULL) {
            error("NULL pointer in set_nodes_conf for nodes range");
            return 0;
        }
        memset(&conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count], 0, sizeof(node_range_t));
        memcpy(&conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].r_def, &aux_r_def[i],
               sizeof(range_def_t));
        conf->nodes[conf->num_nodes].range_count++;
    }

    return 1;
}

static void fill_policies(cluster_conf_t *conf)
{
    int i;
    for (i = 0; i < TOTAL_POLICIES; i++) {
        conf->power_policies[i].policy       = i;
        conf->power_policies[i].is_available = 0;
        memset(conf->power_policies[i].settings, 0, sizeof(double) * MAX_POLICY_SETTINGS);
    }
}

void generate_node_ranges(node_island_t *island, char *nodelist)
{

    range_def_t *aux_r_def = NULL;
    int32_t range_count    = 0;

    range_count = parse_range_list(nodelist, &aux_r_def);

    for (int32_t i = 0; i < range_count; i++) {
        island->ranges = realloc(island->ranges, sizeof(node_range_t) * (island->num_ranges + 1));
        if (island->ranges == NULL) {
            error("NULL pointer in generate_node_ranges");
            return;
        }
        memset(&island->ranges[island->num_ranges], 0, sizeof(node_range_t));
        memcpy(&island->ranges[island->num_ranges], &aux_r_def[i], sizeof(range_def_t));
        island->num_ranges++;
    }
}

void clean_newlines(char *str)
{
    remove_chars(str, '\n');
    remove_chars(str, '\r');
    /* the following are hidden characters that may not be seen in a text editor but may exist nonetheless */
    remove_chars(str, '\a');
    remove_chars(str, '\b');
    remove_chars(str, '\f');
}

void parse_island(cluster_conf_t *conf, char *line)
{
    int idx = -1, i = 0;
    int contains_ip     = 0;
    int contains_sec_ip = 0;
    int contains_eargm  = 0;
    char *token;
    char *first_ptr, *sec_ptr;

    if (conf->num_islands < 1)
        conf->islands = NULL;

    int current_ranges = 0;
    clean_newlines(line);
    token = strtok_r(line, "=", &first_ptr);
    while (token != NULL) {
        strtoup(token);
        if (!strcmp(token, "ISLAND")) {
            token   = strtok_r(NULL, " ", &first_ptr);
            int aux = atoi(token);
            idx     = -1;
            for (i = 0; i < conf->num_islands; i++)
                if (conf->islands[i].id == aux)
                    idx = i;

            if (idx < 0) {
                conf->islands = realloc(conf->islands, sizeof(node_island_t) * (conf->num_islands + 1));
                if (conf->islands == NULL) {
                    error("NULL pointer in allocating islands");
                    return;
                }
                set_default_island_conf(&conf->islands[conf->num_islands], atoi(token));
            }
        } else if (!strcmp(token, "MAX_POWER")) {
            token                                          = strtok_r(NULL, " ", &first_ptr);
            conf->islands[conf->num_islands].max_sig_power = (double) atoi(token);
            verbose(0, "Parsing for this option (MAX_POWER) in Island configuration will be deprecated in the future, "
                       "please change it to a TAG structure\n");
        } else if (!strcmp(token, "MIN_POWER")) {
            token                                          = strtok_r(NULL, " ", &first_ptr);
            conf->islands[conf->num_islands].min_sig_power = (double) atoi(token);
            verbose(0, "Parsing for this option (MIN_POWER) in Island configuration will be deprecated in the future, "
                       "please change it to a TAG structure\n");
        } else if (!strcmp(token, "ERROR_POWER")) {
            token                                            = strtok_r(NULL, " ", &first_ptr);
            conf->islands[conf->num_islands].max_error_power = (double) atoi(token);
            verbose(0, "Parsing for this option (ERROR_POWER) in Island configuration will be deprecated in the "
                       "future, please change it to a TAG structure\n");
        } else if (!strcmp(token, "MAX_TEMP")) {
            token                                     = strtok_r(NULL, " ", &first_ptr);
            conf->islands[conf->num_islands].max_temp = (double) atoi(token);
            verbose(0, "Parsing for this option (MAX_TEMP) in Island configuration will be deprecated in the future, "
                       "please change it to a TAG structure\n");
        } else if (!strcmp(token, "POWER_CAP")) {
            token                                          = strtok_r(NULL, " ", &first_ptr);
            conf->islands[conf->num_islands].max_power_cap = (double) atoi(token);
            verbose(0, "Parsing for this option (POWER_CAP) in Island configuration will be deprecated in the future, "
                       "please change it to a TAG structure\n");
        } else if (!strcmp(token, "POWER_CAP_TYPE")) {
            token = strtok_r(NULL, " ", &first_ptr);
            remove_chars(token, ' ');
            strcpy(conf->islands[conf->num_islands].power_cap_type, token);
            verbose(0, "Parsing for this option (POWER_CAP_TYPE) in Island configuration will be deprecated in the "
                       "future, please change it to a TAG structure\n");
        } else if (!strcmp(token, "DBIP") || !strcasecmp(token, "EARDBD_IP")) {
            if (!strcmp(token, "DBIP"))
                warning("DBIP as an EARDBD specifier for node definitions is deprecated, please use EARDBD_IP");
            contains_ip = 1;
            token       = strtok_r(NULL, " ", &first_ptr);
            int ip_id   = -1;
            int id_f    = idx < 0 ? conf->num_islands : idx;
            if (conf->islands[id_f].num_ips < 1)
                conf->islands[id_f].db_ips = NULL;
            for (i = 0; i < conf->islands[id_f].num_ips; i++) // search if the ip already exists
                if (!strcmp(conf->islands[id_f].db_ips[i], token))
                    ip_id = i;
            if (ip_id < 0) {
                conf->islands[id_f].db_ips =
                    realloc(conf->islands[id_f].db_ips, (conf->islands[id_f].num_ips + 1) * sizeof(char *));
                if (conf->islands[id_f].db_ips == NULL) {
                    error("NULL pointer in DB IPS definition");
                    return;
                }
                conf->islands[id_f].db_ips[conf->islands[id_f].num_ips] = malloc(strlen(token) + 1);
                remove_chars(token, ' ');
                strcpy(conf->islands[id_f].db_ips[conf->islands[id_f].num_ips], token);

                for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                    conf->islands[id_f].ranges[i].db_ip = conf->islands[id_f].num_ips;

                conf->islands[id_f].num_ips++;
            } else {
                for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                    conf->islands[id_f].ranges[i].db_ip = ip_id;
            }
        } else if (!strcmp(token, "DBSECIP") || !strcasecmp(token, "EARDBD_MIRROR_IP")) {
            if (!strcmp(token, "DBSECIP"))
                warning(
                    "DBSECIP as an EARDBD specifier for node definitions is deprecated, please use EARDBD_MIRROR_IP");
            contains_sec_ip = 1;
            token           = strtok_r(NULL, " ", &first_ptr);
            debug("entering DBSECIP with token %s\n", token);
            int ip_id = -1;
            int id_f  = idx < 0 ? conf->num_islands : idx;
            if (conf->islands[id_f].num_backups < 1)
                conf->islands[id_f].backup_ips = NULL;
            for (i = 0; i < conf->islands[id_f].num_backups; i++)
                if (!strcmp(conf->islands[id_f].backup_ips[i], token))
                    ip_id = i;
            if (ip_id < 0) {
                remove_chars(token, ' ');
                if (strlen(token) > 0) {
                    conf->islands[id_f].backup_ips =
                        realloc(conf->islands[id_f].backup_ips, (conf->islands[id_f].num_backups + 1) * sizeof(char *));
                    if (conf->islands[id_f].backup_ips == NULL) {
                        error("NULL pointer in DB backup ip");
                        return;
                    }
                    conf->islands[id_f].backup_ips[conf->islands[id_f].num_backups] = malloc(strlen(token) + 1);
                    strcpy(conf->islands[id_f].backup_ips[conf->islands[id_f].num_backups], token);
                    for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                        conf->islands[id_f].ranges[i].sec_ip = conf->islands[id_f].num_backups;
                    conf->islands[id_f].num_backups++;
                }
            } else {
                for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                    conf->islands[id_f].ranges[i].sec_ip = ip_id;
            }
        } else if (!strcmp(token, "NODES")) {
            contains_ip = contains_sec_ip = contains_eargm = 0;
            token                                          = strtok_r(NULL, " ", &first_ptr);
            int id_f                                       = idx < 0 ? conf->num_islands : idx;
            current_ranges                                 = conf->islands[id_f].num_ranges;
            generate_node_ranges(&conf->islands[id_f], token);
        } else if (!strcmp(token, "EARGMID")) {
            contains_eargm = 1;
            int id_f       = idx < 0 ? conf->num_islands : idx;
            token          = strtok_r(NULL, " ", &first_ptr);
            int egm_id     = atoi(token);
            for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                conf->islands[id_f].ranges[i].eargm_id = egm_id;
        } else if (!strcmp(token, "ISLAND_TAGS")) {
            int i, found = 0;
            token = strtok_r(NULL, " ", &first_ptr);
            token = strtok_r(token, ",", &sec_ptr);
            // this is an alternative to the if(idx<0) system
            int id_f = idx < 0 ? conf->num_islands : idx;
            while (token) {
                found = 0;
                // prevent repeats in multi-line island definitions
                for (i = 0; i < conf->islands[id_f].num_tags && !found; i++) {
                    if (!strcmp(token, conf->islands[id_f].tags[i]))
                        found = 1;
                }
                if (!found) {
                    conf->islands[id_f].tags = realloc(conf->islands[id_f].tags, sizeof(char *));
                    conf->islands[id_f].tags[conf->islands[id_f].num_tags] = calloc(strlen(token), sizeof(char));
                    strcpy(conf->islands[id_f].tags[conf->islands[id_f].num_tags], token);
                    conf->islands[id_f].num_tags++;
                }
                token = strtok_r(NULL, ",", &sec_ptr);
            }
        } else if (!strcmp(token, "TAGS") || !strcmp(token, "TAG")) {
            token = strtok_r(NULL, " ", &first_ptr);

            token                = strtok_r(token, ",", &sec_ptr);
            int id_f             = idx < 0 ? conf->num_islands : idx;
            int current_num_tags = 0;
            int *current_tags    = NULL;

            if (conf->islands[id_f].num_specific_tags < 1)
                conf->islands[id_f].specific_tags = NULL;
            char found = 0;
            while (token) {
                current_tags = realloc(current_tags, sizeof(int) * (current_num_tags + 1));
                for (i = 0; i < conf->islands[id_f].num_specific_tags; i++) {
                    if (!strcmp(token, conf->islands[id_f].specific_tags[i])) {
                        current_tags[current_num_tags] = i;
                        found                          = 1;
                    }
                }
                if (!found) {
                    conf->islands[id_f].specific_tags =
                        realloc(conf->islands[id_f].specific_tags,
                                sizeof(char *) * (conf->islands[id_f].num_specific_tags + 1));
                    conf->islands[id_f].specific_tags[conf->islands[id_f].num_specific_tags] =
                        calloc(strlen(token) + 1, sizeof(char));
                    strcpy(conf->islands[id_f].specific_tags[conf->islands[id_f].num_specific_tags], token);
                    current_tags[current_num_tags] = conf->islands[id_f].num_specific_tags;
                    conf->islands[id_f].num_specific_tags++;
                }
                current_num_tags++;
                token = strtok_r(NULL, ",", &sec_ptr);
            }

            for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++) {
                conf->islands[id_f].ranges[i].specific_tags = calloc(current_num_tags, sizeof(int));
                memcpy(conf->islands[id_f].ranges[i].specific_tags, current_tags, sizeof(int) * current_num_tags);
                conf->islands[id_f].ranges[i].num_tags = current_num_tags;
            }
            free(current_tags);
        }

        token = strtok_r(NULL, "=", &first_ptr);
        debug("parsing %s token\n", token);
    }
    int id_f = idx < 0 ? conf->num_islands : idx;
    if (!contains_ip) {
        int id_db = conf->islands[id_f].num_ips > 0 ? 0 : -1;
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].db_ip = id_db;
    }
    if (!contains_sec_ip) {
        int id_sec_db = conf->islands[id_f].num_backups > 0 ? 0 : -1;
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].sec_ip = id_sec_db;
    }
    if (!contains_eargm) {
        int last_eargm = current_ranges > 0 ? conf->islands[id_f].ranges[current_ranges - 1].eargm_id : 0;
        if (conf->eargm.num_eargms > 0 && last_eargm == 0) {
            last_eargm = conf->eargm.eargms[0].id;
        }
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].eargm_id = last_eargm;
    }
    if (idx < 0)
        conf->num_islands++;
}

state_t EAR_include(char *token)
{
    if (strcasestr(token, "include") != NULL) {
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}

#define ADD_CONTENT    0
#define CREATE_CONTENT 1

void get_cluster_config(FILE *conf_file, cluster_conf_t *conf, uint action)
{
    char line[512];
    char def_policy[128];
    char *token;

    // filling the default policies before starting
    if (action != ADD_CONTENT) {
        conf->num_policies   = 0;
        conf->num_etags      = 0;
        conf->power_policies = calloc(TOTAL_POLICIES, sizeof(policy_conf_t));
        fill_policies(conf);
    }
    while (fgets(line, 512, conf_file) != NULL) {
        if (line[0] == '#')
            continue;
        remove_chars(line, 13); // carriage return (\n or \r)
        token = strtok(line, "=");
        strtoup(token);
#if INCLUDE_CONF
        if (EAR_include(token) == EAR_SUCCESS) {
            char *file_to_include = strtok(NULL, "=");
            clean_newlines(file_to_include);
            verbose(VCCONF, "Include file: '%s'", file_to_include);
            FILE *fd_include = fopen(file_to_include, "r");
            if (fd_include != NULL)
                get_cluster_config(fd_include, conf, ADD_CONTENT);
            else
                verbose(VCCONF, "Include file cannot be open %s", strerror(errno));
            continue;
        }
#endif

        if (EARGM_token(token) == EAR_SUCCESS) {
            if (EARGM_parse_token(&conf->eargm, token) == EAR_SUCCESS)
                continue;
        }
        if (EARD_token(token) == EAR_SUCCESS) {
            if (EARD_parse_token(&conf->eard, token) == EAR_SUCCESS)
                continue;
        }
        if (EARDBD_token(token) == EAR_SUCCESS) {
            if (EARDBD_parse_token(&conf->db_manager, token) == EAR_SUCCESS)
                continue;
        }
        if (!strcmp(token, "POLICY")) {
            if (POLICY_token(&conf->num_policies, &conf->power_policies, line) == EAR_SUCCESS)
                continue;
        }
        if (DB_token(token) == EAR_SUCCESS) {
            if (DB_parse_token(&conf->database, token) == EAR_SUCCESS)
                continue;
        }
        if (EARLIB_token(token) == EAR_SUCCESS) {
            if (EARLIB_parse_token(&conf->earlib, token) == EAR_SUCCESS)
                continue;
        }
        if (AUTH_token(token) == EAR_SUCCESS) {
            if (!strcmp(token, "AUTHORIZEDUSERS")) {
                AUTH_parse_token(token, &conf->auth_users_count, &conf->auth_users);
                continue;
            } else if (!strcmp(token, "AUTHORIZEDGROUPS")) {
                AUTH_parse_token(token, &conf->auth_groups_count, &conf->auth_groups);
                continue;
            } else if (!strcmp(token, "AUTHORIZEDACCOUNTS")) {
                AUTH_parse_token(token, &conf->auth_acc_count, &conf->auth_accounts);
                continue;
            }
        }
        if (ADMIN_token(token) == EAR_SUCCESS) {
            if (!strcmp(token, "ADMINUSERS")) {
                AUTH_parse_token(token, &conf->admin_users_count, &conf->admin_users);
                continue;
            }
        }
        if (ETAG_token(token) == EAR_SUCCESS) {
            if (ETAG_parse_token(&conf->num_etags, &conf->e_tags, line) == EAR_SUCCESS)
                continue;
        }

        if (EDCMON_token(token) == EAR_SUCCESS) {
            line[strlen(line)] = '=';
            if (EDCMON_parse_token(&conf->edcmon_tags, &conf->num_edcmons_tags, line) == EAR_SUCCESS)
                continue;
        }
        // HARDWARE NODE CONFIG
        else if (!strcmp(token, "NODENAME")) {
            int i         = 0;
            int num_nodes = 0;
            int num_cpus  = 0;
            // fully restore the line
            line[strlen(line)] = '=';
            char *primary_ptr;
            char *secondary_ptr;
            char *coef_file = NULL;
            token           = strtok_r(line, " ", &primary_ptr);
            while (token != NULL) {

                // fetches the first half of the pair =
                token = strtok_r(token, "=", &secondary_ptr);
                strtoup(token);
                if (!strcmp(token, "NODENAME")) {
                    // fetches the second half of the pair =
                    token     = strtok_r(NULL, "=", &secondary_ptr);
                    num_nodes = set_nodes_conf(conf, token);
                } else if (!strcmp(token, "CPUS")) {
                    num_cpus = atoi(strtok_r(NULL, "=", &secondary_ptr));
                }

                else if (!strcmp(token, "DEFCOEFFICIENTSFILE")) {
                    token = strtok_r(NULL, "=", &secondary_ptr);
                    strclean(token, '\n');
                    coef_file = malloc(sizeof(char) * strlen(token) + 1);
                    remove_chars(token, ' ');
                    strcpy(coef_file, token);
                }
                // fetches the second half of the pair =
                token = strtok_r(NULL, "=", &secondary_ptr);
                token = strtok_r(NULL, " ", &primary_ptr);
            }

            // global data for all nodes in that line
            for (i = 0; i < num_nodes; i++) {
                conf->nodes[conf->num_nodes + i].cpus = num_cpus;
            }
            if (coef_file != NULL)
                for (i = 0; i < num_nodes; i++)
                    conf->nodes[conf->num_nodes + i].coef_file = coef_file;
            conf->num_nodes += num_nodes;
        } // NODENAME END

        // TAGS definition
        else if (!strcmp(token, "TAG")) {
            line[strlen(line)] = '=';
            TAG_parse_token(&conf->tags, &conf->num_tags, line);
        }
        // ISLES config
        else if (!strcmp(token, "ISLAND")) {
            // fully restore the line
            line[strlen(line)] = '=';
            parse_island(conf, line);
        }
        if (token != NULL)
            GENERIC_parse_token(conf, token, def_policy);
    }

    int i;
    conf->default_policy = 0;
    for (i = 0; i < conf->num_policies; i++) {
        if (!strcmp(def_policy, conf->power_policies[i].name)) {
            conf->default_policy = i;
            break;
        }
    }

    /* Plugins */
    if (strlen(conf->eard.plugins) == 0) {
        strcpy(conf->eard.plugins, "eardbd.so");
    }
    if (strlen(conf->db_manager.plugins) == 0) {
        strcpy(conf->db_manager.plugins, "mysql.so");
    }
    if (strlen(conf->eargm.plugins) == 0) {
        strcpy(conf->eargm.plugins, "mysql.so");
    }
    if (strlen(conf->earlib.plugins) == 0) {
        strcpy(conf->earlib.plugins, "eard.so");
    }

    /* EARGM checks */

    check_cluster_conf_eargm(&conf->eargm);
}

void set_ear_conf_default(cluster_conf_t *my_conf)
{
    if (my_conf == NULL)
        return;
    my_conf->default_policy = -1; // set to -1 so that it throws an error if it is not set on ear.conf
#if USE_EXT
    strcpy(my_conf->net_ext, NW_EXT);
#else
    strcpy(my_conf->net_ext, "");
#endif
    set_default_eard_conf(&my_conf->eard);
    set_default_eargm_conf(&my_conf->eargm);
    set_default_db_conf(&my_conf->database);
    set_default_eardbd_conf(&my_conf->db_manager);
    set_default_earlib_conf(&my_conf->earlib);
    set_default_conf_install(&my_conf->install);
}

int read_cluster_conf(char *conf_path, cluster_conf_t *my_conf)
{
    FILE *conf_file = fopen(conf_path, "r");
    if (conf_file == NULL) {
        error("ERROR opening file: %s\n", conf_path);
        return EAR_ERROR;
    }
    memset(my_conf, 0, sizeof(cluster_conf_t));
    set_ear_conf_default(my_conf);
    get_cluster_config(conf_file, my_conf, CREATE_CONTENT);
    if ((my_conf->num_policies < 1) || (my_conf->num_islands < 1) || (my_conf->default_policy > TOTAL_POLICIES)) {
        error("Error: ear.conf does not contain any island or policy definition or there is no default policy "
              "specified.\n");
        return EAR_ERROR;
    }
    my_conf->cluster_num_nodes = get_num_nodes(my_conf);
    fclose(conf_file);
    // print_cluster_conf(my_conf);
    return EAR_SUCCESS;
}

int read_eardbd_conf(char *conf_path, char *username, char *pass)
{
    FILE *conf_file = fopen(conf_path, "r");
    char line[512];
    if (conf_file == NULL) {
        error("ERROR opening file: %s\n", conf_path);
        return EAR_ERROR;
    }
    int current_line = 0;
    while ((fgets(line, 512, conf_file) != NULL) && (current_line < 2)) {
        strclean(line, '\n');
        if (current_line == 0)
            strcpy(username, line);
        else
            strcpy(pass, line);
        current_line++;
    }
    fclose(conf_file);
    return EAR_SUCCESS;
}

void free_cluster_conf(cluster_conf_t *conf)
{
    int i;

    if (conf == NULL) {
        return;
    }

    for (i = 0; i < conf->auth_users_count; i++)
        free(conf->auth_users[i]);
    free(conf->auth_users);

    for (i = 0; i < conf->auth_groups_count; i++)
        free(conf->auth_groups[i]);
    free(conf->auth_groups);

    for (i = 0; i < conf->auth_acc_count; i++)
        free(conf->auth_accounts[i]);
    free(conf->auth_accounts);

    for (i = 0; i < conf->admin_users_count; i++) {
        free(conf->admin_users[i]);
    }
    free(conf->admin_users);

    char *prev_file = NULL;
    for (i = 0; i < conf->num_nodes; i++) {
        if (conf->nodes[i].coef_file != NULL && conf->nodes[i].coef_file != prev_file) {
            prev_file = conf->nodes[i].coef_file;
            free(conf->nodes[i].coef_file);
        }
        free(conf->nodes[i].range);
    }

    free(conf->nodes);

    int j;
    for (i = 0; i < conf->num_islands; i++) {
        for (j = 0; j < conf->islands[i].num_ips; j++)
            free(conf->islands[i].db_ips[j]);
        for (j = 0; j < conf->islands[i].num_backups; j++)
            free(conf->islands[i].backup_ips[j]);

        for (j = 0; j < conf->islands[i].num_tags; j++)
            free(conf->islands[i].tags[j]);

        if (conf->islands[i].num_tags > 0)
            free(conf->islands[i].tags);
        conf->islands[i].num_tags = 0;

        for (j = 0; j < conf->islands[i].num_specific_tags; j++)
            free(conf->islands[i].specific_tags[j]);
        if (conf->islands[i].num_specific_tags > 0)
            free(conf->islands[i].specific_tags);
        conf->islands[i].num_specific_tags = 0;

        for (j = 0; j < conf->islands[i].num_ranges; j++) {
            if (conf->islands[i].ranges[j].specific_tags != NULL && conf->islands[i].ranges[j].num_tags > 0) {
                free(conf->islands[i].ranges[j].specific_tags);
                conf->islands[i].ranges[j].specific_tags = NULL;
            }
        }
        free(conf->islands[i].ranges);
        free(conf->islands[i].db_ips);
        free(conf->islands[i].backup_ips);
    }

    free(conf->islands);

    free(conf->tags);
    for (i = 0; i < conf->num_etags; i++) {
        for (j = 0; j < conf->e_tags[i].num_users; j++)
            free(conf->e_tags[i].users[j]);

        for (j = 0; j < conf->e_tags[i].num_groups; j++)
            free(conf->e_tags[i].groups[j]);

        for (j = 0; j < conf->e_tags[i].num_accounts; j++)
            free(conf->e_tags[i].accounts[j]);

        free(conf->e_tags[i].users);
        free(conf->e_tags[i].groups);
        free(conf->e_tags[i].accounts);
    }
    free(conf->e_tags);
    free(conf->power_policies);
    for (i = 0; i < conf->eargm.num_eargms; i++) {
        if (conf->eargm.eargms[i].num_subs > 0)
            free(conf->eargm.eargms[i].subs);
    }
    if (conf->eargm.num_eargms > 0)
        free(conf->eargm.eargms);

    memset(conf, 0, sizeof(cluster_conf_t));
}
