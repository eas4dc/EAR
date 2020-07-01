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
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/environment.h>


//#define __OLD__CONF__

/** IP functions */
int get_ip(char *nodename, cluster_conf_t *conf)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char buff[256];
    int s;
    int ip1 = -1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    strcpy(buff, nodename);
    /*#if USE_EXT
    strcat(buff, NW_EXT);
    #endif*/

	if (strlen(conf->net_ext) > 0) {
        	strcat(buff, conf->net_ext);
	}

   	s = getaddrinfo(buff, NULL, &hints, &result);

	if (s != 0) {
		verbose(0, "getaddrinfo fails for port %s (%s)", buff, strerror(errno));
		return EAR_ERROR;
	}


   	for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in*) (rp->ai_addr);
            ip1 = saddr->sin_addr.s_addr;
        }
    }
    freeaddrinfo(result);
    return ip1;
}

int get_ip_ranges(cluster_conf_t *my_conf, int **num_ips, int ***ips)
{
    
    int i, j, k;
    int **aux_ips;
    int *sec_aux_ips;
    int *ip_counter;
    char aux_name[256];
    int total_ranges = 0;
    int current_range = 0;
    //get total number of ranges
    for (i = 0; i < my_conf->num_islands; i++)
        total_ranges += my_conf->islands[i].num_ranges;
    
    if (total_ranges < 1)
    {
        error("No IP ranges found.");
        return EAR_ERROR;
    }
    //allocate memory for each range of IPs as well as each range's IP counter
    ip_counter = calloc(total_ranges, sizeof(int));
    aux_ips = calloc(total_ranges, sizeof(int*));

    for (i = 0; i < my_conf->num_islands; i++)
    {
        for (j = 0; j < my_conf->islands[i].num_ranges; j++)
        {
            node_range_t range = my_conf->islands[i].ranges[j];
            if (range.end == -1)
            {
                sprintf(aux_name, "%s", range.prefix);
                sec_aux_ips = calloc(1, sizeof(int));
                sec_aux_ips[0] = get_ip(aux_name, my_conf);
                aux_ips[current_range] = sec_aux_ips;
                ip_counter[current_range] = 1;
                current_range++;
                continue;
            }
            if (range.end == range.start)
            {
                sprintf(aux_name, "%s%u", range.prefix, range.start);
                sec_aux_ips = calloc(1, sizeof(int));
                sec_aux_ips[0] = get_ip(aux_name, my_conf);
                aux_ips[current_range] = sec_aux_ips;
                ip_counter[current_range] = 1;
                current_range++;
                continue;
            }

            int total_ips = range.end-range.start + 1;
            int it = 0;
            sec_aux_ips = calloc(total_ips, sizeof(int));
            for (k = range.start; k <= range.end ; k++)
            {
                if (k < 10 && range.end > 10)
                    sprintf(aux_name, "%s0%u", range.prefix, k);
                else
                    sprintf(aux_name, "%s%u", range.prefix, k);

                sec_aux_ips[it] = get_ip(aux_name, my_conf);
                it ++;
            }
            aux_ips[current_range] = sec_aux_ips;
            ip_counter[current_range] = it;
            current_range++;
        }
    }
    *ips = aux_ips;
    *num_ips = ip_counter;
    
    return total_ranges;
        
}

int get_range_ips(cluster_conf_t *my_conf, char *nodename, int **ips)
{
    int i, j, range_idx;
    int *aux_ips;
    char aux_name[256];
    for (i = 0; i < my_conf->num_islands; i++)
    {
        range_idx = island_range_conf_contains_node(&my_conf->islands[i], nodename);
        if (range_idx < 0) continue;

        node_range_t range = my_conf->islands[i].ranges[range_idx];
        if (range.end == -1)
        {
            sprintf(aux_name, "%s", range.prefix);
            aux_ips = calloc(1, sizeof(int));
            aux_ips[0] = get_ip(aux_name, my_conf);
            *ips = aux_ips;
            return 1;
        }
        if (range.end == range.start)
        {
            sprintf(aux_name, "%s%u", range.prefix, range.start);
            aux_ips = calloc(1, sizeof(int));
            aux_ips[0] = get_ip(aux_name, my_conf);
            *ips = aux_ips;
            return 1;
        }

        int total_ips = range.end-range.start + 1;
        int it = 0;
        aux_ips = calloc(total_ips, sizeof(int));
        for (j = range.start; j <= range.end ; j++)
        {
            if (j < 10 && range.end > 10)
                sprintf(aux_name, "%s0%u", range.prefix, j);
            else
                sprintf(aux_name, "%s%u", range.prefix, j);

            aux_ips[it] = get_ip(aux_name, my_conf);
            it ++;
        }
        *ips = aux_ips;
        return total_ips;
    }

    return EAR_ERROR;
}

/*
* NODE FUNCTIONS
*/
node_conf_t *get_node_conf(cluster_conf_t *my_conf,char *nodename)
{
	int i=0;
	node_conf_t *n=NULL;
	do{ // At least one node is assumed
		//if (strcmp(my_conf->nodes[i].name,nodename)==0){
		if (range_conf_contains_node(&my_conf->nodes[i], nodename)) {
        	n=&my_conf->nodes[i];
		}
		i++;
	}while((n==NULL) && (i<my_conf->num_nodes));
	return n;
}


my_node_conf_t *get_my_node_conf(cluster_conf_t *my_conf,char *nodename)
{
	int i=0, j=0, range_found=0;
	my_node_conf_t *n=calloc(1, sizeof(my_node_conf_t));
    n->num_policies = my_conf->num_policies;
    n->policies=malloc(sizeof(policy_conf_t)*n->num_policies);
    int num_spec_nodes = 0;
    int range_id = -1;
    while(i<my_conf->num_nodes)
    {
		if (range_conf_contains_node(&my_conf->nodes[i], nodename)) {
            n->cpus = my_conf->nodes[i].cpus;
            n->coef_file = my_conf->nodes[i].coef_file;
		}
		i++;
    }

    i = 0;
    do{ // At least one node is assumed
		if ((range_id = island_range_conf_contains_node(&my_conf->islands[i], nodename)) >= 0) {
            range_found = 1;
            n->island = my_conf->islands[i].id;
            int num_ips = my_conf->islands[i].ranges[range_id].db_ip;
            if (my_conf->islands[i].num_ips > num_ips && num_ips >= 0)
                strcpy(n->db_ip, my_conf->islands[i].db_ips[my_conf->islands[i].ranges[range_id].db_ip]);
            else if (my_conf->islands[i].num_ips > 0)
                strcpy(n->db_ip, my_conf->islands[i].db_ips[0]);
            else
                strcpy(n->db_ip, "");
			if ((my_conf->islands[i].ranges[range_id].sec_ip>=0) && (my_conf->islands[i].num_backups)){
            	strcpy(n->db_sec_ip, my_conf->islands[i].backup_ips[my_conf->islands[i].ranges[range_id].sec_ip]);
			}
			n->max_sig_power=my_conf->islands[i].max_sig_power;
			n->min_sig_power=my_conf->islands[i].min_sig_power;
			n->max_error_power=my_conf->islands[i].max_error_power;
			n->max_temp=my_conf->islands[i].max_temp;
			n->max_power_cap=my_conf->islands[i].max_power_cap;
			strcpy(n->power_cap_type,my_conf->islands[i].power_cap_type);
			n->use_log=my_conf->eard.use_log;
		}
		i++;
	}while(i<my_conf->num_islands);
    
    if (!range_found)
    {
        free(n);
        return NULL;
    }


    //pending checks for policies
		memcpy(n->policies,my_conf->power_policies,sizeof(policy_conf_t)*my_conf->num_policies);
		n->max_pstate=my_conf->eard.max_pstate;

	return n;
}



/** returns the ear.conf path. It checks at $EAR_ETC/ear/ear.conf  */
int get_ear_conf_path(char *ear_conf_path)
{
	char my_path[GENERIC_NAME];
	char *my_etc;
	int fd;
	my_etc=getenv("EAR_ETC");
	if (my_etc==NULL) return EAR_ERROR;
	sprintf(my_path,"%s/ear/ear.conf",my_etc);
	fd=open(my_path,O_RDONLY);
    if (fd>0){
        strcpy(ear_conf_path,my_path);
		close(fd);
        return EAR_SUCCESS;
    }
	return EAR_ERROR;
}

/** returns the eardbd.conf path. It checks at $EAR_ETC/ear/eardbd.conf  */
int get_eardbd_conf_path(char *ear_conf_path)
{
    char my_path[GENERIC_NAME];
    char *my_etc;
    int fd;
    my_etc=getenv("EAR_ETC");
    if (my_etc==NULL) return EAR_ERROR;
    sprintf(my_path,"%s/ear/eardbd.conf",my_etc);
    fd=open(my_path,O_RDONLY);
    if (fd>0){
        strcpy(ear_conf_path,my_path);
		close(fd);
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}


/** CHECKING USER TYPE */
/* returns true if the username, group and/or accounts is presents in the list of authorized users/groups/accounts */
int is_privileged(cluster_conf_t *my_conf, char *user,char *group, char *acc)
{
	int i;
	int found=0;
	if (user!=NULL){
		i=0;
		while((i<my_conf->num_priv_users) && (!found)){
			if (strcmp(user,my_conf->priv_users[i])==0) found=1;
			else if (strcmp(my_conf->priv_users[i],"all")==0) found=1;
			else i++;
		}
	}
	if (found)	return found;
	if (group!=NULL){
		i=0;
		while((i<my_conf->num_priv_groups) && (!found)){
			if (strcmp(group,my_conf->priv_groups[i])==0) found=1;
			else i++;
		}
	}
	if (found)	return found;
	if (acc!=NULL){
		i=0;
		while((i<my_conf->num_acc) && (!found)){
        	if (strcmp(acc,my_conf->priv_acc[i])==0) found=1;
			else i++;
    	}
	}
	return found;
}

/* returns true if the username, group and/or accounts can use the given energy_tag_t */
energy_tag_t * can_use_energy_tag(char *user,char *group, char *acc,energy_tag_t *my_tag)
{
	int i;
    int found=0;
    if (user!=NULL){
		i=0;
        while((i<my_tag->num_users) && (!found)){
			debug("Can_use_energy_tag user %s vs %s\n",user,my_tag->users[i]);
            if (strcmp(user,my_tag->users[i])==0) found=1;
			else if (strcmp(my_tag->users[i],"all")==0) found=1;
			else i++;
        }
    }
    if (found)  return my_tag;
    if (group!=NULL){
		i=0;
        while((i<my_tag->num_groups) && (!found))
		{
			debug("Can_use_energy_tag group %s (%lu) vs %s (%lu)\n",
					group, strlen(group), my_tag->groups[i], strlen(my_tag->groups[i]));

            if (strcmp(group,my_tag->groups[i])==0) found=1;
			else i++;
        }
    }
    if (found)  return my_tag;
    if (acc!=NULL){
		i=0;
        while((i<my_tag->num_accounts) && (!found)){
			debug("Can_use_energy_tag acc %s vs %s\n",acc,my_tag->accounts[i]);
            if (strcmp(acc,my_tag->accounts[i])==0) found=1;
			else i++;
        }
    }
	if (found) return my_tag;
	return NULL;
}

/* returns  the energy tag entry if the username, group and/or accounts is in the list of the users/groups/acc authorized to use the given energy-tag, NULL otherwise */
energy_tag_t * is_energy_tag_privileged(cluster_conf_t *my_conf, char *user,char *group, char *acc,char *energy_tag)
{
	int i;
	int found=0;
	energy_tag_t *my_tag;
	if (energy_tag==NULL) return NULL;
	i=0;
	while((i<my_conf->num_tags) && (!found)){
		if (strcmp(energy_tag,my_conf->e_tags[i].tag)==0){
			found=1;
			my_tag=&my_conf->e_tags[i];
		}else i++;
	}
	if (!found)	return NULL;
	return can_use_energy_tag(user,group,acc,my_tag);
}

/** Returns true if the energy tag exists */
energy_tag_t * energy_tag_exists(cluster_conf_t *my_conf,char *etag)
{
	int i;
	int found=0;
	if (etag==NULL)	return NULL;
	i=0;
	while ((i<my_conf->num_tags) && (!found)){
		if (strcmp(etag,my_conf->e_tags[i].tag)==0)	found=1;
		else i++;
	}
	if (found) return &my_conf->e_tags[i];
	return NULL;
}

/** returns the user type: NORMAL, AUTHORIZED, ENERGY_TAG */
uint get_user_type(cluster_conf_t *my_conf, char *energy_tag, char *user,char *group, char *acc,energy_tag_t **my_tag)
{
	energy_tag_t *is_tag;
	int flag;
	*my_tag=NULL;
	verbose(VPRIV,"Checking user %s group %s acc %s etag %s\n",user,group,acc,energy_tag);
	/* We first check if it is authorized user */
	flag=is_privileged(my_conf,user,group,acc);
	if (flag){
		if (energy_tag!=NULL){
			is_tag=energy_tag_exists(my_conf,energy_tag);
			if (is_tag!=NULL){ *my_tag=is_tag;return ENERGY_TAG;}
			else return AUTHORIZED;
		}else return AUTHORIZED;
	}
	/* It is an energy tag user ? */
	is_tag=is_energy_tag_privileged(my_conf, user,group,acc,energy_tag);
	if (is_tag!=NULL){
		*my_tag=is_tag;
		return ENERGY_TAG;
	} else return NORMAL;
}

/* Copy src in dest */
void copy_ear_lib_conf(earlib_conf_t *dest,earlib_conf_t *src)
{
	if ((dest!=NULL) && (src!=NULL)){
		strcpy(dest->coefficients_pathname,src->coefficients_pathname);
		dest->dynais_levels=src->dynais_levels;
		dest->dynais_window=src->dynais_window;
		dest->dynais_timeout=src->dynais_timeout;
		dest->lib_period=src->lib_period;
		dest->check_every=src->check_every;
	}
}

/* Prints the given library conf */
void print_ear_lib_conf(earlib_conf_t *libc)
{
	if (libc!=NULL){
		verbose(VCCONF,"coeffs %s dynais level %u dynais window_size %u\n",
		libc->coefficients_pathname,libc->dynais_levels,libc->dynais_window);
		verbose(VCCONF,"dynais timeout %u lib_period %u check_every %u\n",
		libc->dynais_timeout,libc->lib_period,libc->check_every);
	}
}

void copy_eard_conf(eard_conf_t *dest,eard_conf_t *src)
{
	memcpy(dest,src,sizeof(eard_conf_t));
}
void copy_eargmd_conf(eargm_conf_t *dest,eargm_conf_t *src)
{
	memcpy(dest,src,sizeof(eargm_conf_t));
}
void copy_eardb_conf(db_conf_t *dest,db_conf_t *src)
{
	memcpy(dest,src,sizeof(db_conf_t));
}
void copy_eardbd_conf(eardb_conf_t *dest,eardb_conf_t *src)
{
	memcpy(dest,src,sizeof(eardb_conf_t));
}

/*** DEFAULT VALUES ****/
void set_default_eardbd_conf(eardb_conf_t *eardbdc)
{
	eardbdc->aggr_time		= DEF_DBD_AGGREGATION_TIME;
	eardbdc->insr_time		= DEF_DBD_INSERTION_TIME;
	eardbdc->tcp_port		= DEF_DBD_SERVER_PORT;
	eardbdc->sec_tcp_port	= DEF_DBD_MIRROR_PORT;
	eardbdc->sync_tcp_port	= DEF_DBD_SYNCHR_PORT;
	eardbdc->mem_size		= DEF_DBD_ALLOC_MBS;
	eardbdc->use_log		= DEF_DBD_FILE_LOG;
	eardbdc->mem_size_types[0] = 60;
	eardbdc->mem_size_types[1] = 22;
	eardbdc->mem_size_types[2] = 5;
	eardbdc->mem_size_types[3] = 0;
	eardbdc->mem_size_types[4] = 7;
	eardbdc->mem_size_types[5] = 5;
	eardbdc->mem_size_types[6] = 1;
}

void set_default_eard_conf(eard_conf_t *eardc)
{
	eardc->verbose=1;           /* default 1 */
    eardc->period_powermon=POWERMON_FREQ;  /* default 30000000 (30secs) */
    eardc->max_pstate=1;       /* default 1 */
    eardc->turbo=USE_TURBO;             /* Fixed to 0 by the moment */
    eardc->port=DAEMON_PORT_NUMBER;              /* mandatory */
    eardc->use_mysql=1;         /* Must EARD report to DB */
    eardc->use_eardbd=1;        /* Must EARD report to DB using EARDBD */
	eardc->force_frequencies=1; /* EARD will force frequencies */
	eardc->use_log=EARD_FILE_LOG;
}

void set_default_earlib_conf(earlib_conf_t *earlibc)
{
	strcpy(earlibc->coefficients_pathname,DEFAULT_COEFF_PATHNAME);
	earlibc->dynais_levels=DEFAULT_DYNAIS_LEVELS;
	earlibc->dynais_window=DEFAULT_DYNAIS_WINDOW_SIZE;
	earlibc->dynais_timeout=MAX_TIME_DYNAIS_WITHOUT_SIGNATURE;
	earlibc->lib_period=PERIOD;
	earlibc->check_every=MPI_CALLS_TO_CHECK_PERIODIC;
}

void set_default_eargm_conf(eargm_conf_t *eargmc)
{
	eargmc->verbose=1;
	eargmc->use_aggregation=1;
	eargmc->t1=DEFAULT_T1;
	eargmc->t2=DEFAULT_T2;
	eargmc->energy=DEFAULT_T2*DEFAULT_POWER;
	eargmc->units=1;
	eargmc->policy=MAXPOWER;
	eargmc->port=EARGM_PORT_NUMBER;
	eargmc->mode=0;
	eargmc->defcon_limits[0]=85;
	eargmc->defcon_limits[1]=90;
	eargmc->defcon_limits[2]=95;
	eargmc->grace_periods=GRACE_T1;
	strcpy(eargmc->mail,"nomail");
	eargmc->use_log=EARGMD_FILE_LOG;
}

void set_default_db_conf(db_conf_t *db_conf)
{
    strcpy(db_conf->user, "ear_daemon");
    strcpy(db_conf->user_commands, "ear_daemon");
    strcpy(db_conf->ip, "127.0.0.1");
    db_conf->port = 0;
	db_conf->max_connections=MAX_DB_CONNECTIONS;
	db_conf->report_node_detail=DEMO;
	db_conf->report_sig_detail=!DB_SIMPLE;
	db_conf->report_loops=!LARGE_CLUSTER;
}

void set_default_island_conf(node_island_t *isl_conf, uint id)
{
	isl_conf->id=id;
	isl_conf->num_ranges=0;
	isl_conf->ranges=NULL;
	isl_conf->db_ips=NULL;
	isl_conf->num_ips=0;	
	isl_conf->backup_ips=NULL;
	isl_conf->num_backups=0;
	isl_conf->num_tags=0;
	isl_conf->min_sig_power=MIN_SIG_POWER;
	isl_conf->max_sig_power=MAX_SIG_POWER;
	isl_conf->max_error_power=MAX_ERROR_POWER;
	isl_conf->max_temp=MAX_TEMP;
	isl_conf->max_power_cap=MAX_POWER_CAP;
	strcpy(isl_conf->power_cap_type,POWER_CAP_TYPE);
}

void set_default_conf_install(conf_install_t *inst)
{
	sprintf(inst->obj_ener, "default");
	sprintf(inst->obj_power_model, "default");
}

/*
 *
 *
 *
 */

int get_node_island(cluster_conf_t *conf, char *hostname)
{
	my_node_conf_t *node;
	int island;

	node = get_my_node_conf(conf, hostname);

	if (node == NULL) {
		return EAR_ERROR;
	}

	island = node->island;
	free(node);

	return island;
}

int get_node_server_mirror(cluster_conf_t *conf, const char *hostname, char *mirror_of)
{
	char hostalias[SZ_NAME_MEDIUM];
	node_island_t *is;
	const char *a, *n;
	int found_server;
	int found_mirror;
	int found_both;
	int i, k;
	char *p;

	//
	strncpy(hostalias, hostname, SZ_NAME_MEDIUM);
	found_both   = 0;
	found_server = 0;
	found_mirror = 0;
	a = hostalias;
	n = hostname;
	
	// Finding a possible short form
	if ((p = strchr(a, '.')) != NULL) {
		p[0] = '\0';
	}

	for (i = 0; i < conf->num_islands && !found_both; ++i)
	{
		is = &conf->islands[i];

		for (k = 0; k < is->num_ranges && !found_both; k++)
		{
			p = is->db_ips[is->ranges[k].db_ip];

			if (!found_server && p != NULL &&
				((strncmp(p, n, strlen(n)) == 0) || (strncmp(p, a, strlen(a)) == 0)))
			{
				found_server = 1;
			}

			if (!found_mirror && is->ranges[k].sec_ip >= 0)
			{
				p = is->backup_ips[is->ranges[k].sec_ip];

				if (p != NULL &&
					((strncmp(p, n, strlen(n)) == 0) || (strncmp(p, a, strlen(a)) == 0)))
				{
					strcpy(mirror_of, is->db_ips[is->ranges[k].db_ip]);
					found_mirror = 1;
				}
			}

			found_both = found_server && found_mirror;
		}
	}

	//Codes:
	//	- 0000 (0x00): nothing
	//	- 0001 (0x01): server
	//	- 0010 (0x02): mirror
	//	- 0011 (0x03): both
	found_mirror = (found_mirror << 1);
	return (found_server | found_mirror);
}

/** Tries to get a short policy name*/
void get_short_policy(char *buf, char *policy, cluster_conf_t *conf)
{
    //each policy could have a short name stored in the cluster_conf section and we could just return that here
    int pol = policy_name_to_id(policy, conf);
    strtolow(policy);
    if (!strcmp(policy, "min_energy_to_solution") || !strcmp(policy, "min_energy"))
    {
        strcpy(buf, "ME");
        return;
    }
    else if (!strcmp(policy, "min_time_to_solution") || !strcmp(policy, "min_time"))
    {
        strcpy(buf, "MT");
        return;
    }
    else if (!strcmp(policy, "monitoring_only") || !strcmp(policy, "monitoring"))
    {
        strcpy(buf, "MO");
        return;
    }
    else if (pol == EAR_ERROR)
    {
        strcpy(buf, "NP");
        return;
    }
    else
        strcpy(buf, policy);
}

/** Converts from policy name to policy_id . Returns EAR_ERROR if error*/
int policy_name_to_id(char *my_policy, cluster_conf_t *conf)
{
    int i;
    for (i = 0; i < conf->num_policies; i++)
    {
        if (strcmp(my_policy, conf->power_policies[i].name) == 0) return i;
    }
	return EAR_ERROR;
}


/** Converts from policy_id to policy name. Returns error if policy_id is not valid*/
int policy_id_to_name(int policy_id, char *my_policy, cluster_conf_t *conf)
{
    int i;
    for (i = 0; i < conf->num_policies; i++)
    {
        if (conf->power_policies[i].policy == policy_id)
        {
		    strcpy(my_policy, conf->power_policies[i].name);;
            return EAR_SUCCESS;
        }
    }
    strcpy(my_policy,"UNKNOWN");;
	return EAR_ERROR;

}

int is_valid_policy(unsigned int p_id, cluster_conf_t *conf)
{
    int i;
    for (i = 0; i < conf->num_policies; i++)
        if (conf->power_policies[i].policy == p_id) return 1;

    return 0;
}


int validate_configuration(cluster_conf_t *conf)
{
    if (conf->num_policies < 1) return EAR_ERROR;
    if (conf->num_islands < 1) return EAR_ERROR;

    int i;
    for (i = 0; i < conf->num_islands; i++)
        if (conf->islands[i].num_ips < 1 || conf->islands[i].num_backups < 1) return EAR_ERROR;
    for (i = 0; i < conf->num_tags; i++)
        if (conf->e_tags[i].num_users < 1 && conf->e_tags[i].num_groups < 1 && conf->e_tags[i].num_accounts < 1) return EAR_ERROR;


    return EAR_SUCCESS;
}
