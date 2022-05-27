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

#ifndef _CLUSTER_CONF_H
#define _CLUSTER_CONF_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <common/sizes.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>
#include <common/string_enhanced.h>
#include <common/types/configuration/node_conf.h>
#include <common/types/configuration/policy_conf.h>

#include <common/types/configuration/cluster_conf_db.h>
#include <common/types/configuration/cluster_conf_tag.h>
#include <common/types/configuration/cluster_conf_etag.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_earlib.h>

#define NORMAL 		0
#define AUTHORIZED	1
#define ENERGY_TAG	2

/*
 *
 * Types
 *
 */

#define POWERCAP_TYPE_NODE  0 
#define POWERCAP_TYPE_APP   1
#define TAG_TYPE_ARCH       0 

typedef struct communication_node
{
    char name[GENERIC_NAME];
    int  distance;
} communication_node_t;

typedef struct conf_install {
	char dir_temp[SZ_PATH_INCOMPLETE];
	char dir_conf[SZ_PATH_INCOMPLETE];
	char dir_inst[SZ_PATH_INCOMPLETE];
	char dir_plug[SZ_PATH_INCOMPLETE];
	char obj_ener[SZ_PATH_SHORT];
	char obj_power_model[SZ_PATH_SHORT];
} conf_install_t;

/** Stores information about EAR installation. */
typedef struct cluster_conf
{
    /**@{ Library & common configuration. */
    char         DB_pathname[GENERIC_NAME];
    char         net_ext[ID_SIZE];
    uint         verbose;
    uint         cluster_num_nodes;
    eard_conf_t	 eard;
    eargm_conf_t eargm;
    /**@}*/
    /**@{ List of policies. */
    uint           num_policies;
    policy_conf_t *power_policies;
    uint           default_policy;
    /**@}*/
    /**@{ List of authorized users. */
    uint   num_priv_users;
    char **priv_users;
    uint   num_priv_groups;
    char **priv_groups;
    uint   num_acc;
    char **priv_acc;
    /**@}*/
    /**@{ Special cases. */
    uint min_time_perf_acc;
    /**@}*/
    /**@{ List of nodes. */
    uint         num_nodes;
    node_conf_t *nodes;
    /**@}*/
    /**@{ Installation (other). */
    db_conf_t    database;
    eardb_conf_t db_manager;
    uint         num_tags;
    tag_t       *tags;
    uint         num_etags;
    energy_tag_t *e_tags;
    uint num_islands;
    node_island_t *islands;
    earlib_conf_t earlib;
    conf_install_t install;
} cluster_conf_t;

/*
 *
 * Functions
 *
 */

/** returns the ear.conf path. It checks first at /etc/ear/ear.conf and, it is not available, checks at $EAR_INSTALL_PATH/etc/sysconf/ear.conf */
int get_ear_conf_path(char *ear_conf_path);

/** returns the eardbd.conf path. It checks first at /etc/ear/eardbd.conf and, it is not available, checks at $EAR_INSTALL_PATH/etc/sysconf/eardbd.conf */
int get_eardbd_conf_path(char *ear_conf_path);

/** returns the pointer to the information of nodename */
node_conf_t *get_node_conf(cluster_conf_t *my_conf,char *nodename);


/** returns the configuration for your node with any specific setting specified at ear.conf */
my_node_conf_t *get_my_node_conf(cluster_conf_t *my_conf,char *nodename);

// Cluster configuration read

/** read the cluster configuration from the ear_cluster.conf pointed by conf path */
int read_cluster_conf(char *conf_path,cluster_conf_t *my_conf);
/** Reads data for EAR DB connection for eardbd and eard */
int read_eardbd_conf(char *conf_path,char *username,char *pass);

/** frees a cluster_conf_t */
void free_cluster_conf(cluster_conf_t *conf);


void set_ear_conf_default(cluster_conf_t *my_conf);


// Cluster configuration verbose



/** Prints in the stdout the whole cluster configuration */
void print_cluster_conf(cluster_conf_t *conf);

/** Prints in the stdout the energy_tag settings */
void print_energy_tag(energy_tag_t *etag);

/** Prints the given library conf */
void print_ear_lib_conf(earlib_conf_t *libc);

// User functions

/** returns  the energy tag entry if the username, group and/or accounts is in the list of the users/groups/acc authorized to use the given energy-tag, NULL otherwise */
energy_tag_t * is_energy_tag_privileged(cluster_conf_t *my_conf, char *user,char *group, char *acc,char *energy_tag);

/** returns true if the username, group and/or accounts is presents in the list of authorized users/groups/accounts */
int is_privileged(cluster_conf_t *my_conf, char *user,char *group, char *acc);

/** returns the user type: NORMAL, AUTHORIZED, ENERGY_TAG */
uint get_user_type(cluster_conf_t *my_conf, char *energy_tag, char *user,char *group, char *acc,energy_tag_t **my_tag);

// Policy functions

/** Check if a given policy if is valid */
int is_valid_policy(unsigned int p_id, cluster_conf_t *conf);


// Copy functions

/** Copy src in dest */
void copy_ear_lib_conf(earlib_conf_t *dest,earlib_conf_t *src);



// Default functions




/** Initializes the default values for earlib conf */
void set_default_earlib_conf(earlib_conf_t *earlibc);

/** Initializes the default values for a given island id conf. This function doesn't allocate memory */
void set_default_island_conf(node_island_t *isl_conf,uint id);

/** Initializes the installation configuration values */
void set_default_conf_install(conf_install_t *inst);

/** Initializes the default values for TAGS */
void set_default_tag_values(tag_t *tag);

/** Returns the default tag id or -1 if no default tag is found */
int get_default_tag_id(cluster_conf_t *conf);
// Concrete data functions
int get_node_island(cluster_conf_t *conf, char *hostname);

int get_node_server_mirror(cluster_conf_t *conf, const char *hostname, char *mirror_of);

/** given a node name, get all ips of its range*/
int get_range_ips(cluster_conf_t *my_conf, char *nodename, int **ips);
int get_ip_ranges(cluster_conf_t *my_conf, int **num_ips, int ***ips);
void get_ip_nodelist(cluster_conf_t *conf, char **nodes, int num_nodes, int **ips);


/** returns the number of nodes defined in cluster_conf_t */
int get_num_nodes(cluster_conf_t *my_conf);

/** returns the ip of the nodename specified */
int get_ip(char *nodename, cluster_conf_t *conf);

/** Returns the short name for a given policy. To be used in eacct etc */
void get_short_policy(char *buf, char *policy, cluster_conf_t *conf);

/** Converts from policy name to policy_id . Returns EAR_ERROR if error*/
int policy_name_to_id(char *my_policy, cluster_conf_t *conf);

/** Converts from policy_id to policy name. Returns error if policy_id is not valid*/
int policy_id_to_name(int policy_id,char *my_policy, cluster_conf_t *conf);

/** Given a default tag id, returns the number of default policies and allocates them in the policies vector.
 *  The function implementation can be found in policy_conf.c*/
int get_default_policies(cluster_conf_t *conf, policy_conf_t **policies, int tag_id);

/** Given a hostname, returns the eargm definition of that nodename. If no configuration is found, returns NULL */
eargm_def_t *get_eargm_conf(cluster_conf_t *conf, char *host);

/** Given a EARGM definition, removes all islands that do not have said EARGM id */
void remove_extra_islands(cluster_conf_t *conf, eargm_def_t *e_def);

/** Given a cluster_conf, checks if all node tags have their powercap set to 1 (unlimited) */
uint tags_are_unlimited(cluster_conf_t *conf);

/** It serializes conf and stores in ear_conf_buf. Memory is internally allocated */
state_t serialize_cluster_conf(cluster_conf_t *conf, char **ear_conf_buf, size_t *conf_size);

/** ear_conf_buf points to a serialized region. conf is the output. Memory is not internally allocated for it */
state_t deserialize_cluster_conf(cluster_conf_t *conf, char *ear_conf_buf, size_t conf_size);

/** Checks if a tag with a given name is defined in ear.conf and returns its id if it exists */
int tag_id_exists(cluster_conf_t *conf, char *tag);


void generate_node_ranges(node_island_t *island, char *nodelist);
#endif
