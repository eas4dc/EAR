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

#ifndef _NODE_CONF_H
#define _NODE_CONF_H

#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/configuration/cluster_conf.h>

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
    uint cpus;
    uint island;
    ulong max_pstate;
    char db_ip[NODE_PREFIX];
    char db_sec_ip[NODE_PREFIX];
    char *coef_file;
    char *energy_plugin;
    char *energy_model;
    char *powercap_plugin;
    char *powercap_gpu_plugin;
    uint num_policies;
    policy_conf_t *policies;
    double  min_sig_power;
    double  max_sig_power;
    double  max_error_power;
    ulong   max_temp;
    ulong   max_avx512_freq;
    ulong   max_avx2_freq;
    long    max_powercap;
    long    powercap;
    char    powercap_type;
		ulong   gpu_def_freq;
    uint 	use_log;
}my_node_conf_t;


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

char range_conf_contains_node(node_conf_t *node, char *nodename);
char island_range_conf_contains_node(node_island_t *node, char *nodename);


/** Copies dest=src */
void copy_my_node_conf(my_node_conf_t *dest,my_node_conf_t *src);


/** prints in the stderr the node configuration */
void print_node_conf(node_conf_t *node_conf);

/** prints in the stderr the specific node configuration */
void print_my_node_conf(my_node_conf_t *my_node_conf);
int policy_name_to_nodeid(char *my_policy, my_node_conf_t *conf);


void print_my_node_conf_fd_binary(int fd,my_node_conf_t *myconf);
void read_my_node_conf_fd_binary(int fd,my_node_conf_t *myconf);

/** Given a  node and policy, returns the policy configuration for that cluser,node,policy */
policy_conf_t *get_my_policy_conf(my_node_conf_t *my_node,uint p_id);


#else
#endif
