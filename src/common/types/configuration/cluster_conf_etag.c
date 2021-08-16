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

#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/config.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/string_enhanced.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf_etag.h>
#include <common/types/configuration/cluster_conf_generic.h>



state_t ETAG_token(char *token)
{
	if (strcasestr(token,"ENERGYTAG")!=NULL) return EAR_SUCCESS;
	return EAR_ERROR;
}

state_t ETAG_parse_token(unsigned int *num_etagsp, energy_tag_t **e_tagsl,char *line)
{
    char *primary_ptr;
    char *secondary_ptr;
    char *token;
    state_t found=EAR_SUCCESS;
    int num_etags=*num_etagsp;
    energy_tag_t *e_tags=*e_tagsl;
    debug("EnergyTag %s",line);
    line[strlen(line)] = '=';
    token = strtok_r(line, " ", &primary_ptr);
    if (num_etags==0)
        e_tags=NULL;
    while (token != NULL)
    {
        token = strtok_r(token, "=", &secondary_ptr);
        strtoup(token);

        if (!strcmp(token, "ENERGYTAG"))
        {
            num_etags++;
            e_tags = realloc(e_tags, sizeof(energy_tag_t) * (num_etags));
            if (e_tags==NULL){
                *e_tagsl=e_tags;
                *num_etagsp=num_etags;
                error("NULL pointer reading energy tags");
                return EAR_ERROR;
            }
            token = strtok_r(NULL, "=", &secondary_ptr);
            memset(&e_tags[num_etags-1], 0, sizeof(energy_tag_t));
            remove_chars(token, ' ');
            strcpy(e_tags[num_etags-1].tag, token);
            e_tags[num_etags-1].users = NULL;
            e_tags[num_etags-1].groups = NULL;
            e_tags[num_etags-1].accounts = NULL;
        }
        else if (!strcmp(token, "PSTATE"))
        {
            token = strtok_r(NULL, "=", &secondary_ptr);
            e_tags[num_etags-1].p_state = atoi(token);
        }
        else if (!strcmp(token, "USERS"))
        {
            token = strtok_r(NULL, "=", &secondary_ptr);
            if (LIST_parse_token(token,&e_tags[num_etags-1].num_users,&e_tags[num_etags-1].users) != EAR_SUCCESS) return EAR_ERROR;
        }
        else if (!strcmp(token, "GROUPS"))
        {
            token = strtok_r(NULL, "=", &secondary_ptr);
            if (LIST_parse_token(token,&e_tags[num_etags-1].num_groups,&e_tags[num_etags-1].groups) != EAR_SUCCESS) return EAR_ERROR;
        }
        else if (!strcmp(token, "ACCOUNTS"))
        {
            token = strtok_r(NULL, "=", &secondary_ptr);
            if (LIST_parse_token(token,&e_tags[num_etags-1].num_accounts,&e_tags[num_etags-1].accounts) != EAR_SUCCESS) return EAR_ERROR;
        }
        token = strtok_r(NULL, " ", &primary_ptr);
    } /* END WHILE */
    *num_etagsp=num_etags;
    *e_tagsl=e_tags;

    return found;

}

void print_energy_tag(energy_tag_t *etag)
{
	verbosen(VCCONF, "--> Tag: %s\t pstate: %u\n", etag->tag, etag->p_state);
	int i;
	for (i = 0; i < etag->num_users; i++)
		verbosen(VCCONF, "---> user: %s\n", etag->users[i]);

	for (i = 0; i < etag->num_accounts; i++)
		verbosen(VCCONF, "---> accounts: %s\n", etag->accounts[i]);
	
	for (i = 0; i < etag->num_groups; i++)
		verbosen(VCCONF, "---> group: %s\n", etag->groups[i]);

}


