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

#ifndef _ETAG_CONF_H
#define _ETAG_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>


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


state_t ETAG_token(char *token);
state_t ETAG_parse_token(unsigned int *num_etagsp, energy_tag_t **e_tagsl,char *line);
void print_energy_tag(energy_tag_t *etag);


#endif
