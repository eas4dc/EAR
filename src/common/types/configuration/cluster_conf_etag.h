/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
