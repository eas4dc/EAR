/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _NODE_CONF_H
#define _NODE_CONF_H

#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/configuration/policy_conf.h>
//#include <common/types/configuration/cluster_conf.h>

/**
* @file node_conf.h
* @brief This file includes basic types and functions headers to manipulate nodes and nodes settings
*
*/


/*
 *
 * Types
 *
 */
#define SHORT_NAME 64
#define NODE_PREFIX 64
#define GENERIC_NODE_NAME 256
typedef struct node_range
{
	char prefix[NODE_PREFIX];
	int start;
	int end;
    int db_ip;
    int sec_ip;
    int *specific_tags;
    int num_tags;
    int eargm_id;
} node_range_t;


typedef struct node_conf
{
	char name[GENERIC_NODE_NAME];
	node_range_t *range;
	uint range_count;
	uint cpus;
	char *coef_file;
} node_conf_t;

typedef struct my_node_conf
{
    uint           cpus;
    uint           island;
    ulong          max_pstate;
    char           db_ip[NODE_PREFIX];
    char           db_sec_ip[NODE_PREFIX];
    char          *coef_file;
    char          *energy_plugin;
    char          *energy_model;
    char          *idle_governor;
    char          *powercap_plugin;
    char          *powercap_gpu_plugin;
    uint           num_policies;
    policy_conf_t *policies;
    double         min_sig_power;
    double         max_sig_power;
    double         max_error_power;
    ulong          max_temp;
    ulong          max_avx512_freq;
    ulong          max_avx2_freq;
    long           max_powercap;
    long           powercap;
    char           powercap_type;
	ulong          gpu_def_freq;
    int            cpu_max_pstate; /* Used by policies as lower limit */
    int            imc_max_pstate; /* Used by policies as lower limit */
    int            idle_pstate; 
	ulong          imc_max_freq;   /* Used to create the imcf list */
	ulong          imc_min_freq;   /* Used to create the imcf list */
    uint           use_log;
	char           tag[GENERIC_NAME];
} my_node_conf_t;


typedef struct node_island
{
	uint id;
	uint num_ranges;
	node_range_t *ranges;
	char **db_ips;
    uint num_ips;
	char **backup_ips;
    uint num_backups;
    double min_sig_power;
    double max_sig_power;
    double max_error_power;
    double max_power_cap;
    ulong  max_temp;
    char power_cap_type[SHORT_NAME];	
    char **tags;
    char **specific_tags;
    uint num_tags;
    uint num_specific_tags;
} node_island_t;


/*
 *
 * Functions
 *
 */

int range_conf_contains_node(node_conf_t *node, char *nodename);
int island_range_conf_contains_node(node_island_t *node, char *nodename);

/** Copies dest=src */
void copy_my_node_conf(my_node_conf_t *dest,my_node_conf_t *src);


/** prints in the stderr the node configuration */
void print_node_conf(node_conf_t *node_conf);
void report_my_node_conf(my_node_conf_t *my_node_conf);

/** prints in the stderr the specific node configuration */
void print_my_node_conf(my_node_conf_t *my_node_conf);
int policy_name_to_nodeid(char *my_policy, my_node_conf_t *conf);


void print_my_node_conf_fd_binary(int fd,my_node_conf_t *myconf);
void read_my_node_conf_fd_binary(int fd,my_node_conf_t *myconf);

/** Given a  node and policy, returns the policy configuration for that cluser,node,policy */
policy_conf_t *get_my_policy_conf(my_node_conf_t *my_node,uint p_id);
int is_smaller_unit(int small, int big);

/** TODO: Declared here because it's used by cluster_conf module. */
int is_smaller_unit(int small, int big);


#else
#endif
