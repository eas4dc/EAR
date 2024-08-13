/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_GENERIC_CONF_H
#define _EAR_GENERIC_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>



state_t GENERIC_parse_token(cluster_conf_t *conf,char *token,char *def_policy);
state_t LIST_parse_token(char *token,unsigned int *num_elemsp,char ***list_elemsp);


state_t AUTH_token(char *token);
state_t AUTH_parse_token(char *token,unsigned int *num_elemsp,char ***list_elemsp);



#endif
