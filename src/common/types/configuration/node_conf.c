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

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/environment.h>
#include <common/output/verbose.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/configuration/node_conf.h>


//#define __OLD__CONF__

char range_conf_contains_node(node_conf_t *node, char *nodename)
{
    char aux_name[256];
    int i, j;
    for (i = 0; i < node->range_count; i++)
    {
        if (node->range[i].end == -1)
        {
            sprintf(aux_name, "%s", node->range[i].prefix);
            if (!strcmp(aux_name, nodename)) return 1;
            else continue;
        }
        if (node->range[i].end == node->range[i].start)
        {
            sprintf(aux_name, "%s%u", node->range[i].prefix, node->range[i].start);
            if (!strcmp(aux_name, nodename)) return 1;
            else continue;
        }
        for (j = node->range[i].end; j >= node->range[i].start && j > 0; j--)
        {
            if (j < 10 && node->range[i].end > 10)
                sprintf(aux_name, "%s0%u", node->range[i].prefix, j);
            else
                sprintf(aux_name, "%s%u", node->range[i].prefix, j);
            if (!strcmp(aux_name, nodename)) { return 1; }
        }
    }

    return 0;
}

char island_range_conf_contains_node(node_island_t *node, char *nodename)
{
    char aux_name[256];
    int i, j;
    for (i = 0; i < node->num_ranges; i++)
    {
        if (node->ranges[i].end == -1)
        {
            sprintf(aux_name, "%s", node->ranges[i].prefix);
            if (!strcmp(aux_name, nodename)) return i;
            else continue;
        }
        if (node->ranges[i].end == node->ranges[i].start)
        {
            sprintf(aux_name, "%s%u", node->ranges[i].prefix, node->ranges[i].start);
            if (!strcmp(aux_name, nodename)) return i;
            else continue;
        }
        for (j = node->ranges[i].end; j >= node->ranges[i].start && j > 0; j--)
        {
            if (j < 10 && node->ranges[i].end > 10)
                sprintf(aux_name, "%s0%u", node->ranges[i].prefix, j);
            else
                sprintf(aux_name, "%s%u", node->ranges[i].prefix, j);
            if (!strcmp(aux_name, nodename)) return i;
        }
    }

    return -1;
}



/*
* NODE FUNCTIONS
*/
void copy_my_node_conf(my_node_conf_t *dest,my_node_conf_t *src)
{
	int i;
	dest->cpus=src->cpus;
	dest->island=src->island;
	dest->max_pstate=src->max_pstate;
	memcpy(dest->db_ip,src->db_ip,sizeof(src->db_ip));
	memcpy(dest->db_sec_ip,src->db_sec_ip,sizeof(src->db_sec_ip));
  if (src->coef_file == NULL)
        dest->coef_file="";
  else{
	    dest->coef_file=malloc(strlen(src->coef_file)+1);
	    strcpy(dest->coef_file,src->coef_file);
  }
	dest->num_policies=src->num_policies;
	dest->policies=(policy_conf_t *)malloc(src->num_policies*sizeof(policy_conf_t));
	for (i=0;i<src->num_policies;i++){
		copy_policy_conf(&dest->policies[i],&src->policies[i]);
	}
	dest->max_sig_power=src->max_sig_power;
	dest->min_sig_power=src->min_sig_power;
	dest->max_error_power=src->max_error_power;
	dest->max_temp=src->max_temp;
	dest->max_powercap=src->max_powercap;
	dest->powercap=src->powercap;
  dest->powercap_type=src->powercap_type;
	dest->gpu_def_freq=src->gpu_def_freq;
	
}

void print_node_conf(node_conf_t *my_node_conf)
{
    int i;
    verbose(VCCONF,"-->cpus %u def_file: %s\n", my_node_conf->cpus, my_node_conf->coef_file);
    for (i = 0; i < my_node_conf->range_count; i++)
        verbosen(VCCONF,"---->prefix: %s\tstart: %u\tend: %u\n", my_node_conf->range[i].prefix, my_node_conf->range[i].start, my_node_conf->range[i].end);
}

void print_my_node_conf(my_node_conf_t *my_node_conf)
{
    int i;
    if (my_node_conf!=NULL){
        verbose(VCCONF,"My node: cpus %u max_pstate %lu island %u ip %s ",
            my_node_conf->cpus,my_node_conf->max_pstate,my_node_conf->island,my_node_conf->db_ip);
        if (my_node_conf->db_sec_ip!=NULL){
            verbose(VCCONF,"sec_ip %s ",my_node_conf->db_sec_ip);
        }
        if (my_node_conf->coef_file!=NULL){
            verbose(VCCONF,"coeffs %s ",my_node_conf->coef_file);
        }
        if (my_node_conf->energy_plugin != NULL && strlen(my_node_conf->energy_plugin) > 1)
        {
            verbose(VCCONF,"energy_plugin %s ",my_node_conf->energy_plugin);
        }
        if (my_node_conf->energy_model != NULL && strlen(my_node_conf->energy_model) > 1)
        {
            verbose(VCCONF,"energy_model %s ",my_node_conf->energy_model);
        }
        if (my_node_conf->powercap_plugin != NULL && strlen(my_node_conf->powercap_plugin) > 1)
        {
            verbose(VCCONF,"powercap_plugin %s ",my_node_conf->powercap_plugin);
        }
        verbose(VCCONF,"\n");
        for (i=0;i<my_node_conf->num_policies;i++){
            print_policy_conf(&my_node_conf->policies[i]);
        }
    }
	verbose(VCCONF,"max_sig_power %.0lf min_sig_power %.0lf error_power %.0lf\nmax_temp %lu powercap %ld max_powercap %ld powercap_type %d\ngpu_def_freq %lu\n",my_node_conf->max_sig_power,my_node_conf->min_sig_power,my_node_conf->max_error_power,my_node_conf->max_temp,my_node_conf->powercap,my_node_conf->max_powercap,my_node_conf->powercap_type,my_node_conf->gpu_def_freq);
}

/** Converts from policy name to policy_id . Returns EAR_ERROR if error*/
int policy_name_to_nodeid(char *my_policy, my_node_conf_t *conf)
{
    int i;
		if ( conf == NULL) return EAR_ERROR;
    for (i = 0; i < conf->num_policies; i++)
    {
        if (strcmp(my_policy, conf->policies[i].name) == 0) return conf->policies[i].policy;
    }
  return EAR_ERROR;
}


void print_my_node_conf_fd_binary(int fd,my_node_conf_t *myconf)
{
	// this function needs to be revwritten
    int coef_len;
    int part1,part2;
    part1=sizeof(uint)*2+sizeof(ulong)+NODE_PREFIX*2;
    part2=sizeof(uint)+sizeof(policy_conf_t)*myconf->num_policies+sizeof(double)*3;
    write(fd,(char *)myconf,part1);
    if (myconf->coef_file!=NULL){
        coef_len=strlen(myconf->coef_file);
        write(fd,(char *)&coef_len,sizeof(int));
        write(fd,myconf->coef_file,coef_len);
    }else{
        coef_len=0;
        write(fd,(char *)&coef_len,sizeof(int));
    }
    write(fd,(char *)myconf+part1+sizeof(char *),part2);
}

void read_my_node_conf_fd_binary(int fd,my_node_conf_t *myconf)
{
	//this function needs to be revwritten
    int coef_len;
    int part1,part2;
    part1=sizeof(uint)*2+sizeof(ulong)+NODE_PREFIX*2;
    part2=sizeof(uint)+sizeof(policy_conf_t)*TOTAL_POLICIES+sizeof(double)*3;
    read(fd,(char *)myconf,part1);
    read(fd,(char *)&coef_len,sizeof(int));
    if (coef_len>0){
        myconf->coef_file=(char *)malloc(coef_len+1);
        myconf->coef_file[coef_len]='\0';
        read(fd,myconf->coef_file,coef_len);
    }else{
        myconf->coef_file=NULL;
    }
    read(fd,(char *)myconf+part1+sizeof(char *),part2);

}

/** Given a  node and policy_id, returns the policy configuration (or NULL) */
policy_conf_t *get_my_policy_conf(my_node_conf_t *my_node,uint p_id)
{
    policy_conf_t *my_policy=NULL;
    uint nump=0;
    while((nump<my_node->num_policies) && (my_node->policies[nump].policy!=p_id)) nump++;
    if (nump<my_node->num_policies){
        my_policy=&my_node->policies[nump];
    }   
    return my_policy;
}

