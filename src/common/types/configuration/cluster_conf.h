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
#include <common/types/configuration/policy_conf.h>
#include <common/types/configuration/node_conf.h>
#include <common/string_enhanced.h>

#define GENERIC_NAME	256
#define USER			64
#define ACC				64
#define SMALL           16

#define NORMAL 		0
#define AUTHORIZED	1
#define ENERGY_TAG	2

/*
 *
 * Types
 *
 */

/*
* EARDVerbose=
* EARDPerdiocPowermon=
* EARDMaxPstate=
* EARDTurbo=
* EARDPort=
*/

typedef struct eard_conf
{
	uint verbose;			/* default 1 */
	ulong period_powermon;	/* default 30000000 (30secs) */
	ulong max_pstate; 		/* default 1 */
	uint turbo;				/* Fixed to 0 by the moment */
	uint port;				/* mandatory */
	uint use_mysql;			/* Must EARD report to DB */
	uint use_eardbd;		/* Must EARD report to DB using EARDBD */
	uint force_frequencies; /* 1=EARD will force pstates specified in policies , 0=will not */
  uint    use_log;
} eard_conf_t;

/*
* EARGMVerbose=
* EARGMT1=
* EARGMT2=
* EARGMEnergy=
* EARGMPort=
*/

#define MAXENERGY   0
#define MAXPOWER    1
#define BASIC 	0
#define KILO 	1
#define MEGA	2

#define DECON_LIMITS	3

typedef struct eargm_conf
{
	uint 	verbose;		/* default 1 */
	uint	use_aggregation; /* Use aggregated metrics.Default 1 */
	ulong	t1;				/* default 60 seconds */
	ulong	t2;				/* default 600 seconds */
	ulong 	energy;			/* mandatory */
	uint	units;			/* 0=J, 1=KJ=default, 2=MJ, or Watts when using Power */	
	uint 	policy;			/* 0=MaxEnergy (default), 1=MaxPower ( not yet implemented) */
	uint 	port;			/* mandatory */
	uint 	mode;
	uint	defcon_limits[3];
	uint	grace_periods;
	char 	mail[GENERIC_NAME];
    char    host[GENERIC_NAME];
	uint  	use_log;
} eargm_conf_t;

typedef struct eardb_conf 
{
	uint aggr_time;
	uint insr_time;
	uint tcp_port;
	uint sec_tcp_port;
	uint sync_tcp_port;
    uint mem_size;
    uchar mem_size_types[EARDBD_TYPES];
    uint    use_log;

} eardb_conf_t;


typedef struct communication_node
{
    char name[GENERIC_NAME];
    int  distance;
} communication_node_t;

typedef struct energy_tag
{
	char tag[USER];
	uint p_state;
	char **users;
	uint num_users;
	char **groups;
	uint num_groups;
	char **accounts;
	uint num_accounts;
} energy_tag_t;

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

typedef struct earlib_conf
{
	char coefficients_pathname[GENERIC_NAME];
    uint dynais_levels;
    uint dynais_window;
	uint dynais_timeout;
	uint lib_period;
	uint check_every;
} earlib_conf_t;

typedef struct conf_install {
	char dir_temp[SZ_PATH_INCOMPLETE];
	char dir_conf[SZ_PATH_INCOMPLETE];
	char dir_inst[SZ_PATH_INCOMPLETE];
	char dir_plug[SZ_PATH_INCOMPLETE];
	char obj_ener[SZ_PATH_SHORT];
	char obj_power_model[SZ_PATH_SHORT];
} conf_install_t;

typedef struct cluster_conf
{
	// Library & common conf
	char DB_pathname[GENERIC_NAME];
    char net_ext[SMALL];
	uint verbose;
	eard_conf_t		eard;
	eargm_conf_t 	eargm;
	// List of policies	
	uint num_policies;
	policy_conf_t *power_policies;
	uint default_policy;			// selecs one of the power_policies
	// Lis of autorized users
	uint num_priv_users;
	char **priv_users;
	uint num_priv_groups;
	char **priv_groups;
	uint num_acc;
	char **priv_acc;
	// Special cases
	uint min_time_perf_acc;
	// List of nodes
	uint num_nodes;
	node_conf_t *nodes;
	db_conf_t database;
	eardb_conf_t db_manager;
	uint num_tags;
	energy_tag_t *e_tags;
	uint num_islands;
	node_island_t *islands;
    earlib_conf_t earlib;
    communication_node_t *comm_nodes;
    uint num_comm_nodes;
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

// Cluster configuration verbose

/** Prints the DB configuration */
void print_database_conf(db_conf_t *conf);


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


/** */
void copy_eard_conf(eard_conf_t *dest,eard_conf_t *src);

/** */
void copy_eargmd_conf(eargm_conf_t *dest,eargm_conf_t *src);

/** */
void copy_eardb_conf(db_conf_t *dest,db_conf_t *src);

/** */
void copy_eardbd_conf(eardb_conf_t *dest,eardb_conf_t *src);

// Default functions

/** Initializes the default values for EARD */
void set_default_eard_conf(eard_conf_t *eardc);

/** Initializes the default values for EAR Global Mamager*/
void set_default_eargm_conf(eargm_conf_t *eardc);

/** Initializes the default values for DB */
void set_default_db_conf(db_conf_t *db_conf);

/** Initializes the default values for EARDBD */
void set_default_eardbd_conf(eardb_conf_t *eardbd_conf);

/** Initializes the default values for earlib conf */
void set_default_earlib_conf(earlib_conf_t *earlibc);

/** Initializes the default values for a given island id conf. This function doesn't allocate memory */
void set_default_island_conf(node_island_t *isl_conf,uint id);

/** Initializes the installation configuration values */
void set_default_conf_install(conf_install_t *inst);

// Concrete data functions
int get_node_island(cluster_conf_t *conf, char *hostname);

int get_node_server_mirror(cluster_conf_t *conf, const char *hostname, char *mirror_of);

/** given a node name, get all ips of its range*/
int get_range_ips(cluster_conf_t *my_conf, char *nodename, int **ips);
int get_ip_ranges(cluster_conf_t *my_conf, int **num_ips, int ***ips);

/** returns the ip of the nodename specified */
int get_ip(char *nodename, cluster_conf_t *conf);

/** Returns the short name for a given policy. To be used in eacct etc */
void get_short_policy(char *buf, char *policy, cluster_conf_t *conf);

/** Converts from policy name to policy_id . Returns EAR_ERROR if error*/
int policy_name_to_id(char *my_policy, cluster_conf_t *conf);

/** Converts from policy_id to policy name. Returns error if policy_id is not valid*/
int policy_id_to_name(int policy_id,char *my_policy, cluster_conf_t *conf);



#endif
