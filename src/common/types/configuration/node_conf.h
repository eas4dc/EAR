/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#pragma once

#include <common/config.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/generic.h>
#include <stdbool.h>
// #include <common/types/configuration/cluster_conf.h>

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
#define SHORT_NAME        64
#define NODE_PREFIX       64
#define GENERIC_NODE_NAME 256

/*
 * Auxiliary structure for range_def_t
 * The first and (optional) second numbers in a range definition.
 * leading_zeroes is the amount of zeroes to the left that get removed during the conversion to
 * integer but should be there for the nodename.
 *
 * Examples:
 * node10[00-09] would have 0 as first, 9 as second, and 1 as leading_zeroes.
 * cmp25[45-48] would have 45 as first, 48 as second, and 0 as leading_zeroes.
 * node10[000-001] would have 0 as first, 1 as second, and 2 as leading_zeroes.
 */
typedef struct pair {
    int32_t first;
    int32_t second;
    int32_t leading_zeroes;
} pair;

/*
 * Structure to parse SLURM-like nodelists.
 * -prefix is the initial text. For example, "cmp25" in cmp25[45,46]; or "island" and "rack" in "island[0-2]rack[0-2]
 * -numbers is a list of pairs that describe the inside of a [].
 *    the possible pair combinations are in "numbers" are:
 *    1)0-0 if NO numbers had to be set (ideally shouldn't happen and the pointer would be null with no range).
 * cmp2546[] (useless []) 2)0-N or N-M if BOTH numbers have been set. ex: cmp25[45-47] 3)N-0 if only ONE number has been
 * set. ex: cmp25[45] The following example would have types 2 and 3: cmp25[45,47-48]. The following is NOT an example
 * of the type 1: cmp2545,cmp2546 -> The pairs here would be NULL pointers Type 1 is only supported as a happy accident.
 * cmp25[,45,47-48] IS an example of types 1, 2 and 3. This will generate nodes cmp25,cmp2545,cmp2547,cmp2548 -next is
 * the next range_def. in "island[0-2]rack[0-2]", "island[0-2]" would represent a range_def and "rack[0-2]" a second
 *
 * Two ranges defined as "cmp2545,cmp2546" will be defined in two separate entries, and will NOT be linked together
 * by *next
 * */
typedef struct range_def {
    char prefix[32]; // should be enough??
    pair *numbers;
    int32_t numbers_count;
    struct range_def *next;
} range_def_t;

typedef struct node_range {
    range_def_t r_def;
    int db_ip;
    int sec_ip;
    int *specific_tags;
    int num_tags;
    int eargm_id;
} node_range_t;

typedef struct node_conf {
    char name[GENERIC_NODE_NAME];
    node_range_t *range;
    uint range_count;
    uint cpus;
    char *coef_file;
} node_conf_t;

typedef struct my_node_conf {
    uint cpus;
    uint island;
    ulong max_pstate;
    char db_ip[NODE_PREFIX];
    char db_sec_ip[NODE_PREFIX];
    char *coef_file;
    char *energy_plugin;
    char *energy_model;
    char *idle_governor;
    char *powercap_plugin;
    char *powercap_node_plugin;
    char *powercap_gpu_plugin;
    uint num_policies;
    policy_conf_t *policies;
    double min_sig_power;
    double max_sig_power;
    double max_error_power;
    ulong max_temp;
    ulong max_avx512_freq;
    ulong max_avx2_freq;
    long max_powercap;
    long powercap;
    char powercap_type;
    ulong gpu_def_freq;
    int cpu_max_pstate; /* Used by policies as lower limit */
    int imc_max_pstate; /* Used by policies as lower limit */
    int idle_pstate;
    ulong imc_max_freq; /* Used to create the imcf list */
    ulong imc_min_freq; /* Used to create the imcf list */
    uint use_log;
    char tag[GENERIC_NAME];
} my_node_conf_t;

typedef struct node_island {
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
    ulong max_temp;
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

/* Checks if a range_conf contains a node.
 * Returns true if the node is in the range, false if it is not.
 */
bool range_conf_contains_node(node_conf_t *node, char *nodename);

/* Checks if a node_islant_t contains a certain node.
 * Returns the range id on success, -1 if it is not on the island */
int nodeconf_get_island_range_for_node(node_island_t *node, char *nodename);

/** Copies dest=src */
void copy_my_node_conf(my_node_conf_t *dest, my_node_conf_t *src);

/** prints in the stderr the node configuration */
void print_node_conf(node_conf_t *node_conf);
void report_my_node_conf(my_node_conf_t *my_node_conf);

/** prints in the stderr the specific node configuration */
void print_my_node_conf(my_node_conf_t *my_node_conf);
int policy_name_to_nodeid(char *my_policy, my_node_conf_t *conf);

int32_t node_range_get_num_nodes(node_range_t *r);

void print_my_node_conf_fd_binary(int fd, my_node_conf_t *myconf);
void read_my_node_conf_fd_binary(int fd, my_node_conf_t *myconf);

/** Given a  node and policy, returns the policy configuration for that cluser,node,policy */
policy_conf_t *get_my_policy_conf(my_node_conf_t *my_node, uint p_id);
int is_smaller_unit(int small, int big);

/** TODO: Declared here because it's used by cluster_conf module. */
int is_smaller_unit(int small, int big);

int32_t difference_in_units(int32_t first, int32_t second);
