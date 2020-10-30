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
