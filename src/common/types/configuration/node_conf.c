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
	dest->max_power_cap=src->max_power_cap;
	strcpy(dest->power_cap_type,src->power_cap_type);
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
        verbose(VCCONF,"\n");
        for (i=0;i<my_node_conf->num_policies;i++){
            print_policy_conf(&my_node_conf->policies[i]);
        }
    }
	verbose(VCCONF,"max_sig_power %.0lf min_sig_power %.0lf error_power %.0lf max_temp %lu power_cap %.1lf power_cap_type %s",my_node_conf->max_sig_power,my_node_conf->min_sig_power,my_node_conf->max_error_power,my_node_conf->max_temp,my_node_conf->max_power_cap,my_node_conf->power_cap_type);
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

