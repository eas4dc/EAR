/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/config.h>
#include <common/environment.h>
#include <common/utils/string.h>
#include <common/output/verbose.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/configuration/node_conf.h>
#include <common/types/configuration/cluster_conf_tag.h>

//#define __OLD__CONF__

//checks if small is one number shorter
int is_smaller_unit(int small, int big)
{
	while (small) {
		small /= 10;
		big /= 10;
	}
	return big; //big will be > 0 if it's a bigger unit
}

int range_conf_contains_node(node_conf_t *node, char *nodename)
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
        for (j = node->range[i].end; j >= node->range[i].start && j >= 0; j--)
        {
            if (is_smaller_unit(j, node->range[i].end))
                sprintf(aux_name, "%s0%u", node->range[i].prefix, j);
            else
                sprintf(aux_name, "%s%u", node->range[i].prefix, j);
            if (!strcmp(aux_name, nodename)) { return 1; }
        }
    }

    return 0;
}

int island_range_conf_contains_node(node_island_t *node, char *nodename)
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
        for (j = node->ranges[i].end; j >= node->ranges[i].start && j >= 0; j--)
        {
            if (is_smaller_unit(j, node->ranges[i].end))
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

    memcpy(dest->db_ip, src->db_ip, sizeof(src->db_ip));
    memcpy(dest->db_sec_ip, src->db_sec_ip, sizeof(src->db_sec_ip));

    if (src->coef_file == NULL) {
        dest->coef_file="";
    } else {
        dest->coef_file = malloc(strlen(src->coef_file) + 1);
        strcpy(dest->coef_file,src->coef_file);
    }

    dest->num_policies = src->num_policies;
    dest->policies = (policy_conf_t *) malloc(src->num_policies * sizeof(policy_conf_t));

    for (i = 0; i < src->num_policies; i++) {
        copy_policy_conf(&dest->policies[i], &src->policies[i]);
    }

    dest->max_sig_power   = src->max_sig_power;
    dest->min_sig_power   = src->min_sig_power;
    dest->max_error_power = src->max_error_power;
    dest->max_temp        = src->max_temp;
    dest->max_powercap    = src->max_powercap;
    dest->powercap        = src->powercap;
    dest->powercap_type   = src->powercap_type;
    dest->gpu_def_freq    = src->gpu_def_freq;
    dest->cpu_max_pstate  = src->cpu_max_pstate;
    dest->imc_max_pstate  = src->imc_max_pstate;
    dest->imc_max_freq    = src->imc_max_freq;
    dest->imc_min_freq    = src->imc_min_freq;
    dest->idle_governor   = src->idle_governor;
    dest->idle_pstate     = src->idle_pstate;

    memcpy(dest->tag, src->tag, strlen(src->tag));

}

void print_node_conf(node_conf_t *my_node_conf)
{
    int i;
    verbose(VCCONF,"-->cpus %u def_file: %s\n", my_node_conf->cpus, my_node_conf->coef_file);
    for (i = 0; i < my_node_conf->range_count; i++)
        verbosen(VCCONF,"---->prefix: %s\tstart: %u\tend: %u\n", my_node_conf->range[i].prefix,
                my_node_conf->range[i].start, my_node_conf->range[i].end);
}


void report_my_node_conf(my_node_conf_t *my_node_conf)
{
    int i;
    char buffer[128];
    if (my_node_conf!=NULL){
        xsnprintf(buffer, sizeof(buffer), "My node: cpus %u max_pstate %lu island %u tag %s\n",
                my_node_conf->cpus, my_node_conf->max_pstate,
                my_node_conf->island, my_node_conf->tag);
        printf("%60.60s", buffer);
        xsnprintf(buffer, sizeof(buffer), "eardbd main server %s eardbd mirror %s \n", my_node_conf->db_ip, (strlen(my_node_conf->db_sec_ip) < 1) ? "not defined" : my_node_conf->db_sec_ip);
        printf("%60.60s", buffer);
        xsnprintf(buffer, sizeof(buffer), "coefficients file %s\n", (my_node_conf->coef_file == NULL)?"not defined":my_node_conf->coef_file);
        printf("%60.60s", buffer);
        xsnprintf(buffer, sizeof(buffer), "energy plugin file %s\n ", (my_node_conf->energy_plugin == NULL)?"not defined":my_node_conf->energy_plugin);
        printf("%60.60s", buffer);
        xsnprintf(buffer, sizeof(buffer), "energy model file %s \n",(my_node_conf->energy_model == NULL)?"not defined":my_node_conf->energy_model);
        printf("%60.60s", buffer);
        xsnprintf(buffer, sizeof(buffer), "powercap plugin file %s\n", (my_node_conf->powercap_plugin == NULL)?"not defined":my_node_conf->powercap_plugin);
        printf("%60.60s", buffer);
        xsnprintf(buffer, sizeof(buffer), "powercap_gpu_plugin %s\n", (my_node_conf->powercap_gpu_plugin != NULL)?"not defined":my_node_conf->powercap_gpu_plugin);
        for (i=0;i<my_node_conf->num_policies;i++){
          report_policy_conf(&my_node_conf->policies[i]);
        }

        snprintf(buffer, sizeof(buffer),"max_sig_power %.0lf min_sig_power %.0lf \n",
          my_node_conf->max_sig_power, my_node_conf->min_sig_power);
        printf("%60.60s", buffer);

        snprintf(buffer, sizeof(buffer),"error_power %.0lf max_temp %lu \n", my_node_conf->max_error_power, my_node_conf->max_temp);
        printf("%60.60s", buffer);

        snprintf(buffer, sizeof(buffer),"powercap_type %s\n", (my_node_conf->powercap_type == POWERCAP_TYPE_NODE)?"NODE":"JOB");
        printf("%60.60s", buffer);

        snprintf(buffer, sizeof(buffer),"gpu_def_freq %lu / cpu_max_pstate %d / imc_max_pstate %d",
            my_node_conf->gpu_def_freq, my_node_conf->cpu_max_pstate, my_node_conf->imc_max_pstate);
        printf("%60.60s\n", buffer);

        snprintf(buffer, sizeof(buffer), "imc_max_freq %lu imc_min_freq %lu", my_node_conf->imc_max_freq, my_node_conf->imc_min_freq);
        printf("%60.60s\n", buffer);
        snprintf(buffer, sizeof(buffer), "Governor used in the IDLE %s pstate %d", my_node_conf->idle_governor, my_node_conf->idle_pstate);
        printf("%60.60s\n", buffer);

        char bpower[64];

        power2str((ulong)my_node_conf->powercap, bpower);
        snprintf(buffer, sizeof(buffer),"powercap %s ",bpower);
        printf("%30.30s", buffer);

        power2str((ulong)my_node_conf->max_powercap, bpower);
        snprintf(buffer, sizeof(buffer), "max_powercap %s ",bpower);
        printf("%30.30s\n", buffer);

    }
}


void print_my_node_conf(my_node_conf_t *my_node_conf)
{
    int i;
    char buffer[64];
    if (my_node_conf!=NULL){
        verbose(VCCONF,"My node: cpus %u max_pstate %lu island %u main eardbd %s tag %s",
                my_node_conf->cpus, my_node_conf->max_pstate,
                my_node_conf->island, my_node_conf->db_ip, my_node_conf->tag);
        if (strlen(my_node_conf->db_sec_ip) > 0) {
            verbose(VCCONF, "secondary eardbd %s", my_node_conf->db_sec_ip);
        } else
        {
            verbose(VCCONF, "secondary eardbd not set.");
        }
        if (my_node_conf->coef_file!=NULL) {
            verbose(VCCONF,"coeffs %s ",my_node_conf->coef_file);
        }
        if (my_node_conf->energy_plugin != NULL && strlen(my_node_conf->energy_plugin) > 1) {
            verbose(VCCONF,"energy_plugin %s ",my_node_conf->energy_plugin);
        }
        if (my_node_conf->energy_model != NULL && strlen(my_node_conf->energy_model) > 1) {
            verbose(VCCONF,"energy_model %s ",my_node_conf->energy_model);
        }
        if (my_node_conf->powercap_plugin != NULL && strlen(my_node_conf->powercap_plugin) > 1) {
            verbose(VCCONF,"powercap_plugin %s ",my_node_conf->powercap_plugin);
        }
        if (my_node_conf->powercap_gpu_plugin != NULL && strlen(my_node_conf->powercap_gpu_plugin) > 1) {
            verbose(VCCONF,"powercap_gpu_plugin %s ",my_node_conf->powercap_gpu_plugin);
        }
        verbose(VCCONF,"\n");
        for (i=0;i<my_node_conf->num_policies;i++){
            print_policy_conf(&my_node_conf->policies[i]);
        }
	      verbose(VCCONF, "max_sig_power %.0lf min_sig_power %.0lf error_power %.0lf\nmax_temp %lu "
            " powercap_type %d\ngpu_def_freq %lu / cpu_max_pstate %d / imc_max_pstate %d\n",
            my_node_conf->max_sig_power, my_node_conf->min_sig_power,
            my_node_conf->max_error_power, my_node_conf->max_temp,
            my_node_conf->powercap_type,
            my_node_conf->gpu_def_freq, my_node_conf->cpu_max_pstate, my_node_conf->imc_max_pstate);
	      verbose(VCCONF, "imc_max_freq %lu imc_min_freq %lu", my_node_conf->imc_max_freq, my_node_conf->imc_min_freq);
        verbose(VCCONF, "Governor used in the IDLE %s pstate %d", my_node_conf->idle_governor, my_node_conf->idle_pstate);
        power2str((ulong)my_node_conf->powercap, buffer);
        verbosen(VCCONF, "powercap %s ",buffer);
        power2str((ulong)my_node_conf->max_powercap, buffer);
        verbose(VCCONF, "max_powercap %s ",buffer);

    }
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

