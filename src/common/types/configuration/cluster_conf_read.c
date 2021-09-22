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

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_generic.h>
#include <common/types/configuration/cluster_conf_db.h>
#include <common/types/configuration/cluster_conf_etag.h>

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
    char *buffer_ptr;
    char *second_ptr;
    char *token;
    char *start;
    char *next_token;

    start = strtok_r(namelist, "[", &buffer_ptr);
    token = strtok_r(NULL, ",", &buffer_ptr);

    conf->nodes = realloc(conf->nodes, sizeof(node_conf_t)*(conf->num_nodes+1));
    if (conf->nodes==NULL){
        error("NULL pointer in set_nodes_conf");
        return 0;
    }
    memset(&conf->nodes[conf->num_nodes], 0, sizeof(node_conf_t));

    //in this case, only one node is specified in the line
    if (token == NULL)
    {

        conf->nodes[conf->num_nodes].range = realloc(conf->nodes[conf->num_nodes].range, sizeof(node_range_t)*(conf->nodes[conf->num_nodes].range_count+1));
        if (conf->nodes[conf->num_nodes].range==NULL){
            error("NULL pointer in set_nodes_conf for nodes range");
            return 0;
        }
        memset(&conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count], 0, sizeof(node_range_t));
        sprintf(conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].prefix, "%s", start);
        conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].start =
            conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].end = -1;
        conf->nodes[conf->num_nodes].range_count++;

        return 1;
    }
    //at least one node if we reach this point

    while (token != NULL)
    {

        conf->nodes[conf->num_nodes].range = realloc(conf->nodes[conf->num_nodes].range, sizeof(node_range_t)*(conf->nodes[conf->num_nodes].range_count+1));
        if (conf->nodes[conf->num_nodes].range==NULL){
            error("NULL pointer in set_nodes_conf for nodes range");
            return 0;
        }
        memset(&conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count], 0, sizeof(node_range_t));
        sprintf(conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].prefix, "%s", start);

        if (strchr(token, ']'))
        {
            next_token = strtok_r(NULL, "[", &buffer_ptr);
            strclean(token, ']');
        }
        else next_token = NULL;

        if (strchr(token, '-'))
        {
            char *first_token = strtok_r(token, "-", &second_ptr);
            char *second_token = strtok_r(NULL, "-", &second_ptr);
            conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].start = atoi(first_token);
            conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].end = atoi(second_token);

            if (atoi(first_token) < 10 && atoi(second_token) < 10 && (strchr(first_token, '0') || strchr(second_token, '0')))
                strcat(conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].prefix, "0");
        }
        else
        {
            conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].end =
                conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].start =
                atoi(token);
            if (atoi(token) < 10 && strchr(token, '0'))
                strcat(conf->nodes[conf->num_nodes].range[conf->nodes[conf->num_nodes].range_count].prefix, "0");
        }
        token = strtok_r(NULL, ",", &buffer_ptr);
        if (next_token != NULL) start = next_token;
        conf->nodes[conf->num_nodes].range_count++;
    }
    return 1;
}
#if 0
static int get_default_pstate(policy_conf_t *pow_pol, int num_pol, int policy)
{
    int i;
    for (i = 0; i < num_pol; i++)
    {
        if (pow_pol[i].policy == policy)
            return pow_pol[i].p_state;
    }
    return 0;
}
#endif

static void fill_policies(cluster_conf_t *conf)
{
    int i;
    for (i = 0; i < TOTAL_POLICIES; i++){
        conf->power_policies[i].policy = i;
        conf->power_policies[i].is_available=0;
        memset(conf->power_policies[i].settings, 0, sizeof(double)*MAX_POLICY_SETTINGS);
    }
}

static void generate_node_ranges(node_island_t *island, char *nodelist)
{
    char *buffer_ptr;
    char *second_ptr;
    char *token;
    char *start;
    char *next_token;

    strclean(nodelist, '\n');
    start = strtok_r(nodelist, "[", &buffer_ptr);
    token = strtok_r(NULL, ",", &buffer_ptr);
    //in this case, no node ranges are specified in the line
    if (token == NULL)
    {
        token = strtok_r(start, ",", &buffer_ptr);
        while (token != NULL)
        {
            island->ranges = realloc(island->ranges, sizeof(node_range_t)*(island->num_ranges+1));
            if (island->ranges==NULL){
                error("NULL pointer in generate_node_ranges");
                return;
            }
            memset(&island->ranges[island->num_ranges], 0, sizeof(node_range_t));
            sprintf(island->ranges[island->num_ranges].prefix, "%s", token);
            island->ranges[island->num_ranges].start = island->ranges[island->num_ranges].end = -1;
            island->ranges[island->num_ranges].db_ip = island->ranges[island->num_ranges].sec_ip = -1;
            island->num_ranges++;
            token = strtok_r(NULL, ",", &buffer_ptr);
        }
    }
    //at least one range if we reach this point
    int range_count = 0;
    while (token != NULL)
    {
        island->ranges = realloc(island->ranges, sizeof(node_range_t)*(island->num_ranges+range_count+1));
        if (island->ranges==NULL){
            error("NULL pointer in generate_node_ranges");
            return;
        }
        memset(&island->ranges[island->num_ranges+range_count], 0, sizeof(node_range_t));
        strcpy(island->ranges[island->num_ranges+range_count].prefix, start);
        if (strchr(token, ']'))
        {
            next_token = strtok_r(NULL, "[", &buffer_ptr);
            strclean(token, ']');
        }
        else next_token = NULL;

        if (strchr(token, '-'))
        {
            char *first_token = strtok_r(token, "-", &second_ptr);
            char *second_token = strtok_r(NULL, "-", &second_ptr);
            island->ranges[island->num_ranges+range_count].start = atoi(first_token);
            island->ranges[island->num_ranges+range_count].end = atoi(second_token);
            if (atoi(first_token) < 10 && atoi(second_token) < 10 && (strchr(first_token, '0') || strchr(second_token, '0')))
                strcat(island->ranges[island->num_ranges+range_count].prefix, "0");

            range_count++;
        }
        else
        {
            island->ranges[island->num_ranges+range_count].start = island->ranges[island->num_ranges+range_count].end =
                atoi(token);
            if (atoi(token) < 10 && strchr(token, '0'))
                strcat(island->ranges[island->num_ranges+range_count].prefix, "0");
            range_count++;
        }
        token = strtok_r(NULL, ",", &buffer_ptr);
        if (next_token != NULL) start = next_token;
    }

    //
    int i;

    for (i = island->num_ranges; i < island->num_ranges + range_count; i++) {
        island->ranges[i].db_ip = island->ranges[i].sec_ip = -1;
    }

    island->num_ranges += range_count;
}

void parse_island(cluster_conf_t *conf, char *line)
{
    int idx = -1, i = 0;
    int contains_ip = 0;
    int contains_sec_ip = 0;
    char tag_parsing = 0;
    char *token;

    if (conf->num_islands < 1)
        conf->islands = NULL;

    int current_ranges = 0;
    //token = strtok_r(line, " ", &primary_ptr);
    token = strtok(line, "=");
    while (token != NULL)
    {
        strtoup(token);
        if (!strcmp(token, "ISLAND"))
        {
            token = strtok(NULL, " ");
            int aux = atoi(token);
            idx = -1;
            for (i = 0; i < conf->num_islands; i++)
                if (conf->islands[i].id == aux) idx = i;

            if (idx < 0)
            {
                conf->islands = realloc(conf->islands, sizeof(node_island_t)*(conf->num_islands+1));
                if (conf->islands==NULL){
                    error("NULL pointer in allocating islands");
                    return;
                }
                set_default_island_conf(&conf->islands[conf->num_islands],atoi(token));
            }
        }
        else if (!strcmp(token, "MAX_POWER")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_sig_power=(double)atoi(token);
            verbose(0, "Parsing for this option (MAX_POWER) in Island configuration will be deprecated in the future, please change it to a TAG structure\n");
        }
        else if (!strcmp(token, "MIN_POWER")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].min_sig_power=(double)atoi(token);
            verbose(0, "Parsing for this option (MIN_POWER) in Island configuration will be deprecated in the future, please change it to a TAG structure\n");
        }
        else if (!strcmp(token, "ERROR_POWER")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_error_power=(double)atoi(token);
            verbose(0, "Parsing for this option (ERROR_POWER) in Island configuration will be deprecated in the future, please change it to a TAG structure\n");
        }
        else if (!strcmp(token, "MAX_TEMP")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_temp=(double)atoi(token);
            verbose(0, "Parsing for this option (MAX_TEMP) in Island configuration will be deprecated in the future, please change it to a TAG structure\n");
        }
        else if (!strcmp(token, "POWER_CAP")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_power_cap=(double)atoi(token);
            verbose(0, "Parsing for this option (POWER_CAP) in Island configuration will be deprecated in the future, please change it to a TAG structure\n");
        }
        else if (!strcmp(token, "POWER_CAP_TYPE")){
            token = strtok(NULL, " ");
            strclean(token, '\n');
            remove_chars(token, ' ');
            strcpy(conf->islands[conf->num_islands].power_cap_type,token);
            verbose(0, "Parsing for this option (POWER_CAP_TYPE) in Island configuration will be deprecated in the future, please change it to a TAG structure\n");
        }
        else if (!strcmp(token, "DBIP"))
        {
            contains_ip = 1;
            token = strtok(NULL, " ");
            strclean(token, '\n');
            int ip_id = -1;
            int id_f = idx < 0 ? conf->num_islands: idx;
            if (conf->islands[id_f].num_ips < 1)
                conf->islands[id_f].db_ips = NULL;
            for (i = 0; i < conf->islands[id_f].num_ips; i++) //search if the ip already exists
                if (!strcmp(conf->islands[id_f].db_ips[i], token)) ip_id = i;
            if (ip_id < 0)
            {
                conf->islands[id_f].db_ips = realloc(conf->islands[id_f].db_ips, (conf->islands[id_f].num_ips+1)*sizeof(char *));
                if (conf->islands[id_f].db_ips==NULL){
                    error("NULL pointer in DB IPS definition");
                    return;
                }
                conf->islands[id_f].db_ips[conf->islands[id_f].num_ips] = malloc(strlen(token)+1);
                remove_chars(token, ' ');
                strcpy(conf->islands[id_f].db_ips[conf->islands[id_f].num_ips], token);

                for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                    conf->islands[id_f].ranges[i].db_ip = conf->islands[id_f].num_ips;

                conf->islands[id_f].num_ips++;
            }
            else
            {
                for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                    conf->islands[id_f].ranges[i].db_ip = ip_id;
            }
        }
        else if (!strcmp(token, "DBSECIP"))
        {
            contains_sec_ip = 1;
            token = strtok(NULL, " ");
            strclean(token, '\n');
            int ip_id = -1;
            int id_f = idx < 0 ? conf->num_islands: idx;
            if (conf->islands[id_f].num_backups < 1)
                conf->islands[id_f].backup_ips = NULL;
            for (i = 0; i < conf->islands[id_f].num_backups; i++)
                if (!strcmp(conf->islands[id_f].backup_ips[i], token)) ip_id = i;
            if (ip_id < 0)
            {
                remove_chars(token, ' ');
                if (strlen(token) > 0)
                {
                    conf->islands[id_f].backup_ips = realloc(conf->islands[id_f].backup_ips,
                            (conf->islands[id_f].num_backups+1)*sizeof(char *));
                    if (conf->islands[id_f].backup_ips==NULL){
                        error("NULL pointer in DB backup ip");
                        return;
                    }
                    conf->islands[id_f].backup_ips[conf->islands[id_f].num_backups] = malloc(strlen(token)+1);
                    strcpy(conf->islands[id_f].backup_ips[conf->islands[id_f].num_backups], token);
                    for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                        conf->islands[id_f].ranges[i].sec_ip = conf->islands[id_f].num_backups;
                    conf->islands[id_f].num_backups++;
                }
            }
            else
            {
                for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
                    conf->islands[id_f].ranges[i].sec_ip = ip_id;
            }
        }
        else if (!strcmp(token, "NODES"))
        {
            contains_ip = contains_sec_ip = 0;
            token = strtok(NULL, " ");
            int id_f = idx < 0 ? conf->num_islands: idx;
            current_ranges = conf->islands[id_f].num_ranges;
            generate_node_ranges(&conf->islands[id_f], token);
        }
        else if (!strcmp(token, "ISLAND_TAGS"))
        {
            tag_parsing = 1;
            int i, found = 0;
            char aux_token[256], *next_token = NULL;
            token = strtok(NULL, " ");
            strcpy(aux_token, token);
            token = strtok(NULL, " ");
            if (token != NULL && strlen(token) > 0)
                next_token = token;
            token = aux_token;
            token = strtok(token, ",");
            //this is an alternative to the if(idx<0) system
            int id_f = idx < 0 ? conf->num_islands: idx;
            while (token)
            {
                found = 0;
                strclean(token, '\n');
                //prevent repeats in multi-line island definitions
                for (i = 0; i < conf->islands[id_f].num_tags && !found; i++)
                {
                    if (!strcmp(token, conf->islands[id_f].tags[i]))
                        found = 1;
                }
                if (!found)
                {
                    conf->islands[id_f].tags = realloc(conf->islands[id_f].tags, sizeof(char *));
                    conf->islands[id_f].tags[conf->islands[id_f].num_tags] = calloc(strlen(token), sizeof(char));
                    strcpy(conf->islands[id_f].tags[conf->islands[id_f].num_tags], token);
                    conf->islands[id_f].num_tags++;
                }
                token = strtok(NULL, ",");
            }
            token = next_token;
        }
        else if (!strcmp(token, "TAGS") || !strcmp(token, "TAG") )
        {
            tag_parsing = 1;
            char aux_token[256], *next_token = NULL;
            token = strtok(NULL, " ");
            strcpy(aux_token, token);
            token = strtok(NULL, " ");

            if (token != NULL && strlen(token) > 0)
                next_token = token;

            token = aux_token;
            token = strtok(token, ",");
            int id_f = idx < 0 ? conf->num_islands: idx;
            int current_num_tags = 0;
            int *current_tags = NULL;

            if (conf->islands[id_f].num_specific_tags < 1)
                conf->islands[id_f].specific_tags = NULL;
            char found = 0;
            while (token)
            {
                strclean(token, '\n');
                current_tags = realloc(current_tags, sizeof(int)*(current_num_tags+1));
                for (i = 0; i < conf->islands[id_f].num_specific_tags; i++)
                {
                    if (!strcmp(token, conf->islands[id_f].specific_tags[i]))
                    {
                        current_tags[current_num_tags] = i;
                        found = 1;
                    }
                }
                if (!found)
                {
                    conf->islands[id_f].specific_tags = realloc(conf->islands[id_f].specific_tags, sizeof(char *)*(conf->islands[id_f].num_specific_tags+1));
                    conf->islands[id_f].specific_tags[conf->islands[id_f].num_specific_tags] = calloc(strlen(token), sizeof(char));
                    strcpy(conf->islands[id_f].specific_tags[conf->islands[id_f].num_specific_tags], token);
                    current_tags[current_num_tags] = conf->islands[id_f].num_specific_tags;
                    conf->islands[id_f].num_specific_tags++;
                }
                current_num_tags++;
                token = strtok(NULL, ",");
            }

            for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            {
                conf->islands[id_f].ranges[i].specific_tags = calloc(current_num_tags, sizeof(int));
                memcpy(conf->islands[id_f].ranges[i].specific_tags, current_tags, sizeof(int)*current_num_tags); 
                conf->islands[id_f].ranges[i].num_tags = current_num_tags; 
            }
            token = next_token;
            free(current_tags);
        }

        //this is a hack, and the entire function should be rewritten using strtok_r
        if (tag_parsing) token = strtok(token, "=");
        else token = strtok(NULL, "=");
        tag_parsing = 0;
    }
    int id_f = idx < 0 ? conf->num_islands: idx;
    if (!contains_ip)
    {
        int id_db = conf->islands[id_f].num_ips > 0 ? 0 : -1;
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].db_ip = id_db;
    }
    if (!contains_sec_ip)
    {
        int id_sec_db = conf->islands[id_f].num_backups > 0 ? 0 : -1;
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].sec_ip = id_sec_db;
    }
    if (idx < 0)
        conf->num_islands++;
}

void get_cluster_config(FILE *conf_file, cluster_conf_t *conf)
{
    char line[256];
    char def_policy[128];
    char *token;

    //filling the default policies before starting
    conf->num_policies=0;
    conf->num_etags=0;
    conf->power_policies = calloc(TOTAL_POLICIES, sizeof(policy_conf_t));
    fill_policies(conf);
    while (fgets(line, 256, conf_file) != NULL)
    {
        if (line[0] == '#') continue;
        remove_chars(line, 13); //carriage return (\n or \r)
        token = strtok(line, "=");
        strtoup(token);


        if (EARGM_token(token) == EAR_SUCCESS){
            if (EARGM_parse_token(&conf->eargm,token) == EAR_SUCCESS) continue;
        }
        if (EARD_token(token) == EAR_SUCCESS){
            if (EARD_parse_token(&conf->eard,token) == EAR_SUCCESS) continue;
        }
        if (EARDBD_token(token) == EAR_SUCCESS){
            if (EARDBD_parse_token(&conf->db_manager,token) == EAR_SUCCESS) continue;
        }
        if (!strcmp(token, "POLICY"))
        {
            if (POLICY_token(&conf->num_policies,&conf->power_policies,line) == EAR_SUCCESS) continue;
        }
        if (DB_token(token) == EAR_SUCCESS){
            if (DB_parse_token(&conf->database,token) == EAR_SUCCESS) continue;
        }
        if (EARLIB_token(token) == EAR_SUCCESS){
            if (EARLIB_parse_token(&conf->earlib,token) == EAR_SUCCESS) continue;
        }
        if (AUTH_token(token) == EAR_SUCCESS){
            if (!strcmp(token, "AUTHORIZEDUSERS")){
                AUTH_parse_token(token,&conf->num_priv_users,&conf->priv_users);
                continue;
            }else if (!strcmp(token, "AUTHORIZEDGROUPS")){
                AUTH_parse_token(token,&conf->num_priv_groups,&conf->priv_groups);
                continue;
            }else if (!strcmp(token, "AUTHORIZEDACCOUNTS")){	
                AUTH_parse_token(token,&conf->num_acc,&conf->priv_acc);
                continue;
            }
        }
        if (ETAG_token(token) == EAR_SUCCESS){
            if (ETAG_parse_token(&conf->num_etags,&conf->e_tags,line) == EAR_SUCCESS) continue;
        }

        //HARDWARE NODE CONFIG
        else if (!strcmp(token, "NODENAME"))
        {
            int i = 0;
            int num_nodes = 0;
            int num_cpus = 0;
            //fully restore the line
            line[strlen(line)] = '=';
            char *primary_ptr;
            char *secondary_ptr;
            char *coef_file = NULL;
            token = strtok_r(line, " ", &primary_ptr);
            while (token != NULL)
            {

                //fetches the first half of the pair =
                token = strtok_r(token, "=", &secondary_ptr);
                strtoup(token);
                if (!strcmp(token, "NODENAME"))
                {
                    //fetches the second half of the pair =
                    token = strtok_r(NULL, "=", &secondary_ptr);
                    num_nodes = set_nodes_conf(conf, token);
                }
                else if (!strcmp(token, "CPUS"))
                {
                    num_cpus = atoi(strtok_r(NULL, "=", &secondary_ptr));
                }

                else if (!strcmp(token, "DEFCOEFFICIENTSFILE"))
                {
                    token = strtok_r(NULL, "=", &secondary_ptr);
                    strclean(token, '\n');
                    coef_file = malloc(sizeof(char)*strlen(token)+1);
                    remove_chars(token, ' ');
                    strcpy(coef_file, token);
                }

                //fetches the second half of the pair =
                token = strtok_r(NULL, "=", &secondary_ptr);
                token = strtok_r(NULL, " ", &primary_ptr);
            }

            //global data for all nodes in that line
            for (i = 0; i < num_nodes; i++)
            {
                conf->nodes[conf->num_nodes+i].cpus = num_cpus;
            }
            if (coef_file != NULL)
                for (i = 0; i < num_nodes; i++)
                    conf->nodes[conf->num_nodes+i].coef_file = coef_file;
            conf->num_nodes += num_nodes;
        } // NODENAME END



        //TAGS definition
        else if (!strcmp(token, "TAG"))
        {
            line[strlen(line)] = '=';
            TAG_parse_token(&conf->tags, &conf->num_tags, line);
        }

        //ISLES config
        else if (!strcmp(token, "ISLAND"))
        {
            //fully restore the line
            line[strlen(line)] = '=';
            parse_island(conf, line);
        }
        if (token != NULL) GENERIC_parse_token(conf,token,def_policy);
    }

    int i;
    conf->default_policy = 0;
    for (i = 0; i < conf->num_policies; i++)
    {
        if (!strcmp(def_policy, conf->power_policies[i].name)) {
            conf->default_policy = i;
            break;
        }
    }
}


void set_ear_conf_default(cluster_conf_t *my_conf)
{
    if (my_conf==NULL) return;
    my_conf->default_policy = -1; //set to -1 so that it throws an error if it is not set on ear.conf
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

int read_cluster_conf(char *conf_path,cluster_conf_t *my_conf)
{
    FILE *conf_file = fopen(conf_path, "r");
    if (conf_file == NULL)
    {
        error("ERROR opening file: %s\n", conf_path);
        return EAR_ERROR;
    }
    memset(my_conf, 0, sizeof(cluster_conf_t));
    set_ear_conf_default(my_conf);
    get_cluster_config(conf_file, my_conf);
    if ((my_conf->num_policies < 1) || (my_conf->num_islands < 1) || (my_conf->default_policy >TOTAL_POLICIES ))
    {
        error( "Error: ear.conf does not contain any island or policy definition or there is no default policy specified.\n");
        return EAR_ERROR;
    }
    my_conf->cluster_num_nodes=get_num_nodes(my_conf);
    fclose(conf_file);
    //print_cluster_conf(my_conf);
    return EAR_SUCCESS;
}

int read_eardbd_conf(char *conf_path,char *username,char *pass)
{
    FILE *conf_file = fopen(conf_path, "r");
    char line[256];
    if (conf_file == NULL)
    {
        error("ERROR opening file: %s\n", conf_path);
        return EAR_ERROR;
    }
    int current_line=0;
    while ((fgets(line, 256, conf_file) != NULL) && (current_line<2))
    {
        strclean(line, '\n');
        if (current_line==0) 	strcpy(username,line);
        else 					strcpy(pass,line);
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

    for (i = 0; i < conf->num_priv_users; i++)
        free(conf->priv_users[i]);
    free(conf->priv_users);

    for (i = 0; i < conf->num_priv_groups; i++)
        free(conf->priv_groups[i]);
    free(conf->priv_groups);

    for (i = 0; i < conf->num_acc; i++)
        free(conf->priv_acc[i]);
    free(conf->priv_acc);


    char *prev_file = NULL;
    for (i = 0; i < conf->num_nodes; i++) {
        if (conf->nodes[i].coef_file != NULL && conf->nodes[i].coef_file != prev_file)  {
            prev_file = conf->nodes[i].coef_file;
            free(conf->nodes[i].coef_file);
        }
        free(conf->nodes[i].range);
    }

    free(conf->nodes);

    int j;
    for (i = 0; i < conf->num_islands; i++)
    {
        for (j = 0; j < conf->islands[i].num_ips; j++)
            free(conf->islands[i].db_ips[j]);
        for (j = 0; j < conf->islands[i].num_backups; j++)
            free(conf->islands[i].backup_ips[j]);

        for (j = 0; j < conf->islands[i].num_tags; j++)
            free(conf->islands[i].tags[j]);

        if (conf->islands[i].num_tags > 0) free(conf->islands[i].tags);
        conf->islands[i].num_tags = 0;

        for (j = 0; j < conf->islands[i].num_specific_tags; j++)
            free(conf->islands[i].specific_tags[j]);
        if (conf->islands[i].num_specific_tags > 0) free(conf->islands[i].specific_tags);
        conf->islands[i].num_specific_tags = 0;


        for (j = 0; j < conf->islands[i].num_ranges; j++)
        {
            if (conf->islands[i].ranges[j].specific_tags != NULL && conf->islands[i].ranges[j].num_tags > 0)
            {
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
    for (i = 0; i < conf->num_etags; i++)
    {
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

    memset(conf, 0, sizeof(cluster_conf_t));
}
