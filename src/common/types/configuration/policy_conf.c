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
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <limits.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <management/cpufreq/frequency.h>


/*
 *   POLICY FUNCTIONS
 */

int get_default_policies(cluster_conf_t *conf, policy_conf_t **policies, int tag_id)
{
    int i, total_policies = 0, uses_def_tag = 1;
    policy_conf_t *policy_aux = NULL;
    if (tag_id < 0)
    {
        uses_def_tag = 0;
        verbose(VCCONF, "No default tag id found");
    }

    for (i = 0; i < conf->num_policies; i++)
    {
        // If there's a default tag we search compatibilities with default tags and non-tagged policies
        if (uses_def_tag)
        {
            if (conf->power_policies[i].tag == NULL || !strcmp(conf->tags[tag_id].id, conf->power_policies[i].tag) || strlen(conf->power_policies[i].tag) < 1)
            {
                policy_aux = realloc(policy_aux, sizeof(policy_conf_t)*(total_policies + 1));
                memcpy(&policy_aux[total_policies], &conf->power_policies[i], sizeof(policy_conf_t));
                total_policies++;
            }
        }
        else // if there is no default tag, we just search for non-tagged policies as defaults
        {
            if (conf->power_policies[i].tag == NULL || strlen(conf->power_policies[i].tag) < 1)
            {
                policy_aux = realloc(policy_aux, sizeof(policy_conf_t)*(total_policies + 1));
                memcpy(&policy_aux[total_policies], &conf->power_policies[i], sizeof(policy_conf_t));
                total_policies++;
            }
        }
    }

    *policies = policy_aux;
    return total_policies;

}

void copy_policy_conf(policy_conf_t *dest,policy_conf_t *src)
{
	memcpy((void *)dest,(void *)src,sizeof(policy_conf_t));
}

void init_policy_conf(policy_conf_t *p)
{
//    p->th = 0;
    p->policy = -1;
    p->is_available = 0;
    p->def_freq=(float)0;
    p->p_state=UINT_MAX;
    memset(p->settings, 0, sizeof(double)*MAX_POLICY_SETTINGS);
}

void print_policy_conf(policy_conf_t *p) 
{
    int i;
	if (p==NULL) return;

    verbosen(VCCONF,"---> policy %s p_state %u def_freq %.3f NormalUsersAuth %u", p->name ,p->p_state,p->def_freq,p->is_available);
    for (i = 0; i < MAX_POLICY_SETTINGS; i++)
        verbosen(VCCONF, " setting%d  %.2lf ", i, p->settings[i]);
    verbosen(VCCONF, " tag: %s", p->tag);
    verbosen(VCCONF, "\n");
}

state_t POLICY_token(unsigned int *num_policiesp, policy_conf_t **power_policiesl,char *line)
{
    char *primary_ptr;
    char *secondary_ptr;
    char *key, *value,*token;
    int num_policies=*num_policiesp;
    policy_conf_t *power_policies=(policy_conf_t *)*power_policiesl;
    policy_conf_t *curr_policy;
    line[strlen(line)] = '=';
    token = strtok_r(line, " ", &primary_ptr);
    if (num_policies>=TOTAL_POLICIES){
        power_policies = realloc(power_policies, sizeof(policy_conf_t)*(num_policies + 1));
        if (power_policies==NULL){
            error("NULL pointer in get_cluster_config");
            return EAR_ERROR;
        }
        *power_policiesl=power_policies;
    }
    curr_policy = &power_policies[num_policies];
    init_policy_conf(curr_policy);
    /* POLICY DEFINITION */
    while (token != NULL)
    {
        key = strtok_r(token, "=", &secondary_ptr);
        strtoup(key);
        value = strtok_r(NULL, "=", &secondary_ptr);
        if (!strcmp(key, "POLICY"))
        {
            strcpy(curr_policy->name, value);
        }
        else if (!strcmp(key, "SETTINGS"))
        {
            value = strtok(value, ",");
            int i;
            for (i = 0; (i < MAX_POLICY_SETTINGS) && (value != NULL); i++)
            {
                curr_policy->settings[i] = atof(value);
                value = strtok(NULL, ",");
            }
        }
        else if (!strcmp(key, "DEFAULTFREQ"))
        {
            curr_policy->def_freq= atof(value);
        }
        else if (!strcmp(key, "DEFAULTPSTATE"))
        {
            curr_policy->p_state=atoi(value);
        }
        else if (!strcmp(key, "PRIVILEGED"))
        {
            curr_policy->is_available=(atoi(value)==0);
        }
        else if (!strcmp(key, "TAG"))
        {
            strclean(value, '\n');
            strcpy(curr_policy->tag, value);
        }

        token = strtok_r(NULL, " ", &primary_ptr);
    }
    curr_policy->policy = num_policies;
    num_policies++;
    *num_policiesp = num_policies;
    return EAR_SUCCESS;
}
