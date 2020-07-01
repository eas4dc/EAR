/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
*/

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>

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

    if (conf->num_islands == 0)
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
        }
        else if (!strcmp(token, "MIN_POWER")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].min_sig_power=(double)atoi(token);
        }
        else if (!strcmp(token, "ERROR_POWER")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_error_power=(double)atoi(token);
        }
        else if (!strcmp(token, "MAX_TEMP")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_temp=(double)atoi(token);
        }
        else if (!strcmp(token, "POWER_CAP")){
            token = strtok(NULL, " ");
            conf->islands[conf->num_islands].max_power_cap=(double)atoi(token);
        }
        else if (!strcmp(token, "POWER_CAP_TYPE")){
            token = strtok(NULL, " ");
            strclean(token, '\n');
            remove_chars(token, ' ');
            strcpy(conf->islands[conf->num_islands].power_cap_type,token);
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
            for (i = 0; i < conf->islands[id_f].num_ips; i++)
                if (!strcmp(conf->islands[id_f].db_ips[i], token)) ip_id = i;
            if (ip_id < 0)
            {
                conf->islands[id_f].db_ips = realloc(conf->islands[id_f].db_ips,
                                                                    (conf->islands[id_f].num_ips+1)*sizeof(char *));
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
            token = strtok(NULL, " ");
            int id_f = idx < 0 ? conf->num_islands: idx;
            current_ranges = conf->islands[id_f].num_ranges;
            generate_node_ranges(&conf->islands[id_f], token);
        }
        /*else if (!strcmp(token, "ISLAND_TAGS"))
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
        else if (!strcmp(token, "TAGS"))
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
        }*/
        
        //this is a hack, and the entire function should be rewritten using strtok_r
        if (tag_parsing) token = strtok(token, "=");
        else token = strtok(NULL, "=");
        tag_parsing = 0;
    }
    int id_f = idx < 0 ? conf->num_islands: idx;
    if (!contains_ip && conf->islands[id_f].num_ips > 0)
    {
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].db_ip = 0;
    }
    if (!contains_sec_ip && conf->islands[id_f].num_backups > 0)
    {
        for (i = current_ranges; i < conf->islands[id_f].num_ranges; i++)
            conf->islands[id_f].ranges[i].sec_ip = 0;
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
	conf->num_tags=0;
    conf->power_policies = calloc(TOTAL_POLICIES, sizeof(policy_conf_t));
	fill_policies(conf);
	while (fgets(line, 256, conf_file) != NULL)
	{
		if (line[0] == '#') continue;
        remove_chars(line, 13);
		token = strtok(line, "=");
		strtoup(token);

		if (!strcmp(token, "DEFAULTPOWERPOLICY"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
            remove_chars(token, ' ');
            strcpy(def_policy, token);
		}
		else if (!strcmp(token, "DATABASEPATHNAME"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
            remove_chars(token, ' ');
			strcpy(conf->DB_pathname, token);
		}

            //EARLIB CONF
		else if (!strcmp(token, "COEFFICIENTSDIR"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
            remove_chars(token, ' ');
			strcpy(conf->earlib.coefficients_pathname, token);
		}
        else if (!strcmp(token, "DYNAISLEVELS"))
        {
            token = strtok(NULL, "=");
            conf->earlib.dynais_levels = atoi(token);
        }
        else if (!strcmp(token, "DYNAISWINDOWSIZE"))
        {
            token = strtok(NULL, "=");
            conf->earlib.dynais_window = atoi(token);
        }
        else if (!strcmp(token, "DYNAISTIMEOUT"))
        {
            token = strtok(NULL, "=");
            conf->earlib.dynais_timeout = atoi(token);
        }
        else if (!strcmp(token, "LIBRARYPERIOD"))
        {
            token = strtok(NULL, "=");
            conf->earlib.lib_period = atoi(token);
        }
        else if (!strcmp(token, "CHECKEARMODEEVERY"))
        {
            token = strtok(NULL, "=");
            conf->earlib.check_every = atoi(token);
        }
		else if (!strcmp(token, "VERBOSE"))
		{
			token = strtok(NULL, "=");
			conf->verbose = atoi(token);
		}
        else if (!strcmp(token, "POLICY"))
        {
			char *primary_ptr;
			char *secondary_ptr;
            char *key, *value;
            policy_conf_t *curr_policy;
			line[strlen(line)] = '=';
			token = strtok_r(line, " ", &primary_ptr);
			if (conf->num_policies>=TOTAL_POLICIES){
      	    conf->power_policies = realloc(conf->power_policies, sizeof(policy_conf_t)*(conf->num_policies + 1));
				if (conf->power_policies==NULL){
					error("NULL pointer in get_cluster_config");
					return ;
				}
			}
            curr_policy = &conf->power_policies[conf->num_policies];
            init_policy_conf(curr_policy);
      /* POLICY DEFINITION */
			while (token != NULL)
			{
				key = strtok_r(token, "=", &secondary_ptr);
				strtoup(key);
				value = strtok_r(NULL, "=", &secondary_ptr);
                if (!strcmp(key, "POLICY"))
                {
                    strcpy(curr_policy->name, value);
                }
                else if (!strcmp(key, "SETTINGS"))
                {
                    value = strtok(value, ",");
                    int i;
                    for (i = 0; (i < MAX_POLICY_SETTINGS) && (value != NULL); i++)
                    {
                        curr_policy->settings[i] = atof(value);
                        value = strtok(NULL, ",");
                    }
                }
                else if (!strcmp(key, "DEFAULTFREQ"))
                {
                    curr_policy->def_freq= atof(value);
                }
								else if (!strcmp(key, "DEFAULTPSTATE"))
								{
									curr_policy->p_state=atoi(value);
								}
                else if (!strcmp(key, "PRIVILEGED"))
                {
									curr_policy->is_available=(atoi(value)==0);
                }
                                
			    token = strtok_r(NULL, " ", &primary_ptr);
            }
            curr_policy->policy = conf->num_policies;
            conf->num_policies++;
        }
		else if (!strcmp(token, "MINTIMEPERFORMANCEACCURACY"))
		{
			token = strtok(NULL, "=");
			conf->min_time_perf_acc = atoi(token);
		}
		else if (!strcmp(token, "AUTHORIZEDUSERS"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, ",");
			while (token != NULL)
			{
				conf->num_priv_users++;
				conf->priv_users = realloc(conf->priv_users, sizeof(char *)*conf->num_priv_users);
				if (conf->priv_users==NULL){
					error("NULL pointer reading authorized users");
					return;
				}
				strclean(token, '\n');
				conf->priv_users[conf->num_priv_users-1] = malloc(strlen(token)+1);
                remove_chars(token, ' ');
				strcpy(conf->priv_users[conf->num_priv_users-1], token);
				token = strtok(NULL, ",");
			}
		}
        else if (!strcmp(token, "AUTHORIZEDGROUPS"))
        {
            token = strtok(NULL, "=");
            token = strtok(token, ",");
            while (token != NULL)
            {
                conf->num_priv_groups++;
                conf->priv_groups = realloc(conf->priv_groups, sizeof(char *)*conf->num_priv_groups);
								if (conf->priv_groups==NULL){
									error("NULL pointer reading authorized groups");
									return;
								}
                strclean(token, '\n');
                conf->priv_groups[conf->num_priv_groups-1] = malloc(strlen(token)+1);
                remove_chars(token, ' ');
                strcpy(conf->priv_groups[conf->num_priv_groups-1], token);
                token = strtok(NULL, ",");
            }
        }
		else if (!strcmp(token, "AUTHORIZEDACCOUNTS"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, ",");
			while (token != NULL)
			{
				conf->num_acc++;
				conf->priv_acc = realloc(conf->priv_acc, sizeof(char *)*conf->num_acc);
				if (conf->priv_acc==NULL){
					error("NULLL pointer when reading privileges accounts");
					return;
				}
				strclean(token, '\n');
				conf->priv_acc[conf->num_acc-1] = malloc(strlen(token)+1);
                remove_chars(token, ' ');
				strcpy(conf->priv_acc[conf->num_acc-1], token);
				token = strtok(NULL, ",");
			}
		}
		else if (!strcmp(token, "ENERGYTAG"))
		{
			line[strlen(line)] = '=';
			char *primary_ptr;
			char *secondary_ptr;
			token = strtok_r(line, " ", &primary_ptr);
			while (token != NULL)
			{
				token = strtok_r(token, "=", &secondary_ptr);
				strtoup(token);

				//this must always be the first one
				if (!strcmp(token, "ENERGYTAG"))
				{
					conf->num_tags++;
					if (conf->num_tags==1) conf->e_tags=NULL;
					conf->e_tags = realloc(conf->e_tags, sizeof(energy_tag_t) * (conf->num_tags));
					if (conf->e_tags==NULL){
						error("NULL pointer reading energy tags");
						return;
					}
					token = strtok_r(NULL, "=", &secondary_ptr);
					memset(&conf->e_tags[conf->num_tags-1], 0, sizeof(energy_tag_t));
          remove_chars(token, ' ');
					strcpy(conf->e_tags[conf->num_tags-1].tag, token);
					conf->e_tags[conf->num_tags-1].users = NULL;
					conf->e_tags[conf->num_tags-1].groups = NULL;
					conf->e_tags[conf->num_tags-1].accounts = NULL;
				}
				else if (!strcmp(token, "PSTATE"))
				{
					token = strtok_r(NULL, "=", &secondary_ptr);
					conf->e_tags[conf->num_tags-1].p_state = atoi(token);
				}
				else if (!strcmp(token, "USERS"))
				{
					token = strtok_r(NULL, "=", &secondary_ptr);
					token = strtok_r(token, ",", &secondary_ptr);
					while (token != NULL)
					{
						conf->e_tags[conf->num_tags-1].users = realloc(conf->e_tags[conf->num_tags-1].users,
																	   sizeof(char *)*(conf->e_tags[conf->num_tags-1].num_users+1));
						if (conf->e_tags[conf->num_tags-1].users==NULL){
							error("NULL pointer in allocating etags");
							return;
						}
						conf->e_tags[conf->num_tags-1].users[conf->e_tags[conf->num_tags-1].num_users] = malloc(strlen(token)+1);
                        remove_chars(token, ' ');
						remove_chars(token, '\n');
						strcpy(conf->e_tags[conf->num_tags-1].users[conf->e_tags[conf->num_tags-1].num_users], token);
						conf->e_tags[conf->num_tags-1].num_users++;
                        token = strtok_r(NULL, ",", &secondary_ptr);
					}
				}
				else if (!strcmp(token, "GROUPS"))
				{
					token = strtok_r(NULL, "=", &secondary_ptr);
					token = strtok_r(token, ",", &secondary_ptr);
					while (token != NULL)
					{
						conf->e_tags[conf->num_tags-1].groups = realloc(conf->e_tags[conf->num_tags-1].groups,
																		sizeof(char *)*(conf->e_tags[conf->num_tags-1].num_groups+1));
						if (conf->e_tags[conf->num_tags-1].groups==NULL){
							error("NULL pointer in allocating etags");
							return;
						}
						conf->e_tags[conf->num_tags-1].groups[conf->e_tags[conf->num_tags-1].num_groups] = malloc(strlen(token)+1);
                        remove_chars(token, ' ');
						remove_chars(token, '\n');
						strcpy(conf->e_tags[conf->num_tags-1].groups[conf->e_tags[conf->num_tags-1].num_groups], token);
						conf->e_tags[conf->num_tags-1].num_groups++;
                        token = strtok_r(NULL, ",", &secondary_ptr);
					}
				}
				else if (!strcmp(token, "ACCOUNTS"))
				{
					token = strtok_r(NULL, "=", &secondary_ptr);
					token = strtok_r(token, ",", &secondary_ptr);
					while (token != NULL)
					{
						conf->e_tags[conf->num_tags-1].accounts = realloc(conf->e_tags[conf->num_tags-1].accounts,
																		  sizeof(char *)*(conf->e_tags[conf->num_tags-1].num_accounts+1));
						if (conf->e_tags[conf->num_tags-1].accounts==NULL){
							error("NULL pointer in allocating etags");
							return;
						}
						conf->e_tags[conf->num_tags-1].accounts[conf->e_tags[conf->num_tags-1].num_accounts] = malloc(strlen(token)+1);
                        remove_chars(token, ' ');
						remove_chars(token, '\n');
						strcpy(conf->e_tags[conf->num_tags-1].accounts[conf->e_tags[conf->num_tags-1].num_accounts], token);
						conf->e_tags[conf->num_tags-1].num_accounts++;
                        token = strtok_r(NULL, ",", &secondary_ptr);
					}
				}
				token = strtok_r(NULL, " ", &primary_ptr);
			}
		}

			//HARDWARE NODE CONFIG
		else if (!strcmp(token, "NODENAME"))
		{
			int i = 0;
			int island = 0;
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

			//EARD conf
		else if (!strcmp(token, "NODEDAEMONVERBOSE"))
		{
			token = strtok(NULL, "=");
			conf->eard.verbose = atoi(token);
		}
		else if (!strcmp(token, "NODEDAEMONPOWERMONFREQ"))
		{
			token = strtok(NULL, "=");
			conf->eard.period_powermon = atoi(token);
		}
		else if (!strcmp(token, "NODEDAEMONMAXPSTATE"))
		{
			token = strtok(NULL, "=");
			conf->eard.max_pstate = atoi(token);
		}
		else if (!strcmp(token, "NODEDAEMONTURBO"))
		{
			token = strtok(NULL, "=");
			conf->eard.turbo = atoi(token);
		}
		else if (!strcmp(token, "NODEDAEMONPORT"))
		{
			token = strtok(NULL, "=");
			conf->eard.port = atoi(token);
		}
		else if (!strcmp(token, "NODEUSEDB"))
        {
            token = strtok(NULL, "=");
            conf->eard.use_mysql = atoi(token);
        }
        else if (!strcmp(token, "NODEUSEEARDBD"))
        {
            token = strtok(NULL, "=");
            conf->eard.use_eardbd = atoi(token);
        }
        else if (!strcmp(token, "NODEDAEMONFORCEFREQUENCIES"))
        {
            token = strtok(NULL, "=");
            conf->eard.force_frequencies = atoi(token);
        }
		else if (!strcmp(token, "NODEUSELOG"))
        {
            token = strtok(NULL, "=");
            conf->eard.use_log = atoi(token);
        }


		//DB MANAGER
		else if (!strcmp(token, "DBDAEMONAGGREGATIONTIME"))
		{
			token = strtok(NULL, "=");
			conf->db_manager.aggr_time = atoi(token);
		}
		else if (!strcmp(token, "DBDAEMONINSERTIONTIME"))
		{
			token = strtok(NULL, "=");
			conf->db_manager.insr_time = atoi(token);
		}
		else if (!strcmp(token, "DBDAEMONPORTTCP"))
		{
			token = strtok(NULL, "=");
			conf->db_manager.tcp_port = atoi(token);
		}
		else if (!strcmp(token, "DBDAEMONPORTSECTCP"))
		{
			token = strtok(NULL, "=");
			conf->db_manager.sec_tcp_port = atoi(token);
		}
	    else if (!strcmp(token, "DBDAEMONSYNCPORT"))
        {
            token = strtok(NULL, "=");
            conf->db_manager.sync_tcp_port = atoi(token);
        }
        else if (!strcmp(token, "DBDAEMONMEMORYSIZE"))
        {
            token = strtok(NULL, "=");
            conf->db_manager.mem_size = atoi(token);
        }
		else if (!strcmp(token, "DBDAEMONMEMORYSIZEPERTYPE"))
		{
			int i = 0;

			token = strtok(NULL, "=");
			token = strtok(token, ",");

			while (token != NULL && i < EARDBD_TYPES)
			{
				conf->db_manager.mem_size_types[i] = atoi(token);
				token = strtok(NULL, ",");
				i++;
			}
		}
		else if (!strcmp(token, "DBDAEMONUSELOG"))
		{
            token = strtok(NULL, "=");
            conf->db_manager.use_log = atoi(token);
		}

		//GLOBAL MANAGER
		else if (!strcmp(token, "GLOBALMANAGERVERBOSE"))
		{
			token = strtok(NULL, "=");
			conf->eargm.verbose = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERUSEAGGREGATED"))
		{
			token = strtok(NULL, "=");
			conf->eargm.use_aggregation = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERPERIODT1"))
		{
			token = strtok(NULL, "=");
			conf->eargm.t1 = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERPERIODT2"))
		{
			token = strtok(NULL, "=");
			conf->eargm.t2 = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERUNITS"))
		{
			token = strtok(NULL, "=");
		
			//
			strclean(token, '\n');
			remove_chars(token, ' ');
			strtoup(token);

			if (!strcmp(token,"-"))	conf->eargm.units=BASIC;
			else if (!strcmp(token,"K")) conf->eargm.units=KILO;
			else if (!strcmp(token,"M")) conf->eargm.units=MEGA;
			else conf->eargm.units=KILO;
		}
		else if (!strcmp(token, "GLOBALMANAGERPOLICY"))
		{
			token = strtok(NULL, "=");
	               
			// 
			strclean(token, '\n');
                        remove_chars(token, ' ');
                        strtoup(token);

			if (strcmp(token,"MAXENERGY") == 0) {
				conf->eargm.policy=MAXENERGY;
			} else {
				conf->eargm.policy=MAXPOWER;
			}
		}
		else if (!strcmp(token, "GLOBALMANAGERENERGYLIMIT"))
		{
			token = strtok(NULL, "=");
			conf->eargm.energy = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERPOWERLIMIT"))
		{
			token = strtok(NULL, "=");
			/* It mas be included in power */
			conf->eargm.energy = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERWARNINGSPERC"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, ",");
			int perc=0;

			while (token != NULL)
			{
				conf->eargm.defcon_limits[perc++] = atoi(token);
				token = strtok(NULL, ",");
			}
		}
		else if (!strcmp(token, "GLOBALMANAGERGRACEPERIODS"))
		{
			token = strtok(NULL, "=");
			conf->eargm.grace_periods = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERPORT"))
		{
			token = strtok(NULL, "=");
			conf->eargm.port = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERMODE"))
		{
			token = strtok(NULL, "=");
			conf->eargm.mode = atoi(token);
		}
		else if (!strcmp(token, "GLOBALMANAGERMAIL"))
		{
			token = strtok(NULL, "=");
			strclean(token, '\n');
			remove_chars(token, ' ');
			strcpy(conf->eargm.mail, token);
		}
		else if (!strcmp(token, "GLOBALMANAGERHOST"))
		{
			token = strtok(NULL, "=");
			strclean(token, '\n');
			remove_chars(token, ' ');
			strcpy(conf->eargm.host, token);
		}
        else if (!strcmp(token, "GLOBALMANAGERUSELOG"))
        {
			token = strtok(NULL, "=");
			conf->eargm.use_log = atoi(token);
        }
        //COMMON CONFIG
        else if (!strcmp(token, "NETWORKEXTENSION"))
        {
            token = strtok(NULL, "=");
            strcpy(conf->net_ext, token);
        }

		//MARIADB/MYSQL config
		else if (!strcmp(token, "MARIADBIP"))
		{
			token = strtok(NULL, "=");
			strclean(token, '\n');
            remove_chars(token, ' ');
			strcpy(conf->database.ip, token);
		}
		else if (!strcmp(token, "MARIADBUSER"))
		{
			token = strtok(NULL, "=");
			strclean(token, '\n');
            remove_chars(token, ' ');
			strcpy(conf->database.user, token);
		}
		else if (!strcmp(token, "MARIADBPASSW"))
		{
			token = strtok(NULL, "=");
			strclean(token, '\n');
            remove_chars(token, ' ');
			strcpy(conf->database.pass, token);
		}
        else if (!strcmp(token, "MARIADBCOMMANDSUSER"))
        {
            token = strtok(NULL, "=");
            strclean(token, '\n');
            remove_chars(token, ' ');
            strcpy(conf->database.user_commands, token);
        }
        else if (!strcmp(token, "MARIADBCOMMANDSPASSW"))
        {
            token = strtok(NULL, "=");
            strclean(token, '\n');
            remove_chars(token, ' ');
            strcpy(conf->database.pass_commands, token);
        }
		else if (!strcmp(token, "MARIADBDATABASE"))
		{
			token = strtok(NULL, "=");
			strclean(token, '\n');
            remove_chars(token, ' ');
			strcpy(conf->database.database, token);
		}
		else if (!strcmp(token, "MARIADBPORT"))
		{
			token = strtok(NULL, "=");
			conf->database.port = atoi(token);
		}
		else if (!strcmp(token, "MAXCONNECTIONS"))
        {
            token = strtok(NULL, "=");
            conf->database.max_connections = atoi(token);
        }
		else if (!strcmp(token, "REPORTNODEDETAIL"))
        {
            token = strtok(NULL, "=");
            conf->database.report_node_detail = atoi(token);
		}
		else if (!strcmp(token, "REPORTSIGDETAIL"))
        {
            token = strtok(NULL, "=");
            conf->database.report_sig_detail = atoi(token);
        }
		else if (!strcmp(token, "REPORTLOOPS"))
        {
            token = strtok(NULL, "=");
            conf->database.report_loops = atoi(token);
        }

		    //ISLES config
		else if (!strcmp(token, "ISLAND"))
		{
			//fully restore the line
			line[strlen(line)] = '=';
            parse_island(conf, line);
		}
		// INSTALLATION AREA
		else if (!strcmp(token, "TMPDIR"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
			remove_chars(token, ' ');
			strcpy(conf->install.dir_temp, token);
		}
		else if (!strcmp(token, "ETCDIR"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
			remove_chars(token, ' ');
			strcpy(conf->install.dir_conf, token);
		}
		else if (!strcmp(token, "INSTDIR"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
			remove_chars(token, ' ');
			strcpy(conf->install.dir_inst, token);

			// Plugin path
			sprintf(conf->install.dir_plug, "%s/lib/plugins", token);
		}
		else if (!strcmp(token, "PLUGINENERGY"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
			remove_chars(token, ' ');
			strcpy(conf->install.obj_ener, token);
		}
		else if (!strcmp(token, "PLUGINPOWERMODEL"))
		{
			token = strtok(NULL, "=");
			token = strtok(token, "\n");
			remove_chars(token, ' ');
			strcpy(conf->install.obj_power_model, token);
		}
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
        /*
        for (j = 0; j < conf->islands[i].num_tags; j++)
            free(conf->islands[i].tags[j]);
        free(conf->islands[i].tags);
        for (j = 0; j < conf->islands[i].num_specific_tags; j++)
            free(conf->islands[i].specific_tags[j]);
        free(conf->islands[i].specific_tags);

        for (j = 0; j < conf->islands[i].num_ranges; j++)
        {
            if (conf->islands[i].ranges[j].specific_tags != NULL && conf->islands[i].ranges[j].num_tags > 0)
            {
                free(conf->islands[i].ranges[j].specific_tags);
                conf->islands[i].ranges[j].specific_tags = NULL;
            }
        } */
		free(conf->islands[i].ranges);
        free(conf->islands[i].db_ips);
        free(conf->islands[i].backup_ips);
    }

	free(conf->islands);

	for (i = 0; i < conf->num_tags; i++)
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
