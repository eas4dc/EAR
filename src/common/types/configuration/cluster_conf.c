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
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_db.h>
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

    if (conf != NULL) {
    	if (strlen(conf->net_ext) > 0) {
        	strcat(buff, conf->net_ext);
	    }
    }

   	s = getaddrinfo(buff, NULL, &hints, &result);

	if (s != 0) {
		debug("getaddrinfo fails for node %s (%s)", buff, strerror(errno));
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

void get_ip_nodelist(cluster_conf_t *conf, char **nodes, int num_nodes, int **ips)
{
    int i;
    int *aux_ips = calloc(num_nodes, sizeof(int));
    for (i = 0; i < num_nodes; i++)
    {
        aux_ips[i] = get_ip(nodes[i], conf);
    }

    *ips = aux_ips;
}

int get_num_nodes(cluster_conf_t *my_conf)
{
    int i, j;
    int total_nodes = 0;
    
    if (my_conf->num_islands < 1)
    {
        error("No island ranges found.");
        return EAR_ERROR;
    }

    for (i = 0; i < my_conf->num_islands; i++)
    {
        for (j = 0; j < my_conf->islands[i].num_ranges; j++)
        {
            node_range_t range = my_conf->islands[i].ranges[j];
            if (range.end == -1)
                total_nodes++;
            else if (range.end == range.start)
                total_nodes++;
            else
                total_nodes += (range.end - range.start) + 1; //end - start does not count the starting node
        }
    }
    
    return total_nodes;
        
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

int tag_id_exists(cluster_conf_t *conf, char *tag)
{
    int i;
    if (tag == NULL) return 0;
    for (i = 0; i < conf->num_tags; i++)
        if (!strcmp(tag, conf->tags[i].id)) return i;

    return -1;

}

int get_default_tag_id(cluster_conf_t *conf)
{
    int i, id = -1;
    for (i = 0; i < conf->num_tags; i++)
        if (conf->tags[i].is_default) id = i;

    return id;
}


my_node_conf_t *get_my_node_conf(cluster_conf_t *my_conf, char *nodename)
{
	int i=0, j=0, range_found=0;
    int range_id = -1;
    int tag_id = -1, def_tag_id = -1;

	if (my_conf->num_islands == 0) return NULL;

    def_tag_id = get_default_tag_id(my_conf);

	my_node_conf_t *n = calloc(1, sizeof(my_node_conf_t));

    n->num_policies = get_default_policies(my_conf, &n->policies, def_tag_id);
    //n->num_policies = my_conf->num_policies;
    //n->policies = malloc(sizeof(policy_conf_t)*n->num_policies);

    n->max_pstate=my_conf->eard.max_pstate;

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
			n->use_log=my_conf->eard.use_log;


            j = 0;
            if (my_conf->islands[i].ranges[range_id].num_tags > 0)
            {
                for (j = 0; j < my_conf->islands[i].ranges[range_id].num_tags; j++)
                {
                    // tags for each island are stored in specific_tags
                    // each range of an island stores the ids in the array of specific_tags that they have assigned
                    tag_id = tag_id_exists(my_conf, my_conf->islands[i].specific_tags[my_conf->islands[i].ranges[range_id].specific_tags[j]]);
                    if (tag_id >= 0) {
                        debug("found tag: %s\n", my_conf->islands[i].specific_tags[my_conf->islands[i].ranges[range_id].specific_tags[j]]);
                        break;
                    }
                }
            }
		}
		i++;
	}while(i<my_conf->num_islands);
    
    if (!range_found)
    {
        free(n);
        return NULL;
    }

        n->energy_plugin = my_conf->install.obj_ener;
        n->energy_model = my_conf->install.obj_power_model;

    if (tag_id < 0) tag_id = def_tag_id; // if no tag found, we apply the default tag
    if (tag_id >= 0)
    {
        debug("Found my_node_conf with tag: %s\n", my_conf->tags[tag_id].id);
        n->max_sig_power = (double)my_conf->tags[tag_id].max_power;
        n->min_sig_power = (double)my_conf->tags[tag_id].min_power;
        n->max_error_power = (double)my_conf->tags[tag_id].error_power;
        n->max_temp = my_conf->tags[tag_id].max_temp;
        n->powercap = (double)my_conf->tags[tag_id].powercap;
        n->max_powercap = (double)my_conf->tags[tag_id].max_powercap;
        n->max_avx512_freq = my_conf->tags[tag_id].max_avx512_freq;
        n->max_avx2_freq = my_conf->tags[tag_id].max_avx2_freq;

        n->powercap_type = my_conf->tags[tag_id].powercap_type;
        n->gpu_def_freq = my_conf->tags[tag_id].gpu_def_freq;

        if (my_conf->tags[tag_id].energy_plugin != NULL && strlen(my_conf->tags[tag_id].energy_plugin) > 0)
            n->energy_plugin = my_conf->tags[tag_id].energy_plugin;
        if (my_conf->tags[tag_id].energy_model != NULL && strlen(my_conf->tags[tag_id].energy_model) > 0)
            n->energy_model = my_conf->tags[tag_id].energy_model;

        n->powercap_plugin = my_conf->tags[tag_id].powercap_plugin;
        n->powercap_gpu_plugin = my_conf->tags[tag_id].powercap_gpu_plugin;

        n->coef_file = strlen(my_conf->tags[tag_id].coeffs) > 0 ? my_conf->tags[tag_id].coeffs : "coeffs.default";
        
    }
    else warning("No tag found for current node in ear.conf\n");

    //POLICY management
    char found;
    if (tag_id >= 0 && tag_id != def_tag_id) // policies with the default tag have already been added
    {
        for (i = 0; i < my_conf->num_policies; i++)
        {
            found = 0;
            debug("comparing current tag (%s) with policy tag (%s)\n", my_conf->tags[tag_id].id, my_conf->power_policies[i].tag);
            if (!strcmp(my_conf->tags[tag_id].id, my_conf->power_policies[i].tag)) //we only need the policies for our current tag
            {
                debug("match found!\n");
                for (j = 0; j < n->num_policies; j++)
                {
                    if (!strcmp(my_conf->power_policies[i].name, n->policies[j].name)) //two policies are the same if they are have the same name, so we replace them
                    {
                        memcpy(&n->policies[j], &my_conf->power_policies[i], sizeof(policy_conf_t));
                        found = 1;
                        break;
                    }
                }
                if (!found) // if the policy didn't exist as a default one, we add it for this node
                {
                    debug("new policy found for this tag, adding it to my_node_conf's list\n");
                    n->policies = realloc(n->policies, sizeof(policy_conf_t)*(n->num_policies + 1));
                    memcpy(&n->policies[n->num_policies], &my_conf->power_policies[i], sizeof(policy_conf_t));
                    n->num_policies ++;
                }
            }


        }
    }
    /* Automatic computation of powercap */
		#if POWERCAP
		/* If node powercap is -1, and there is a global powercap, power is equally distributed */
    if (n->powercap == DEF_POWER_CAP){
      if (my_conf->eargm.power > 0){
        n->powercap = my_conf->eargm.power / my_conf->cluster_num_nodes;
      }else{
				/* 0 means no limit */
        n->powercap = 0;
      }
    }
		#endif


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
	while((i<my_conf->num_etags) && (!found)){
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
	while ((i<my_conf->num_etags) && (!found)){
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



/*** DEFAULT VALUES ****/

void set_default_tag_values(tag_t *tag)
{
    tag->type           = TAG_TYPE_ARCH;
    tag->powercap_type  = POWERCAP_TYPE_NODE;

    tag->max_power      = MAX_SIG_POWER;
	tag->error_power    = MAX_ERROR_POWER;
	tag->powercap       = DEF_POWER_CAP;
	tag->min_power      = MIN_SIG_POWER;
	tag->max_temp       = MAX_TEMP;
	tag->gpu_def_freq   = 0;
    
    strcpy(tag->coeffs, "coeffs.default");
    strcpy(tag->energy_model, "");
    strcpy(tag->energy_plugin, "");
    strcpy(tag->powercap_plugin, "");
    strcpy(tag->powercap_gpu_plugin, "");
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
	isl_conf->max_power_cap=DEF_POWER_CAP;
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
		else if (!strncmp(policy, "load_balance",strlen("load_balance")))
		{
			strcpy(buf, "LB");
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
		if (conf == NULL) return EAR_ERROR;
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
    for (i = 0; i < conf->num_etags; i++)
        if (conf->e_tags[i].num_users < 1 && conf->e_tags[i].num_groups < 1 && conf->e_tags[i].num_accounts < 1) return EAR_ERROR;


    return EAR_SUCCESS;
}
