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

#ifndef _TAG_CONF_H
#define _TAG_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef struct tags
{
    char id[GENERIC_NAME];
    char type;
    char is_default;
    char powercap_type;
    ulong max_avx512_freq;
    ulong max_avx2_freq;
    ulong max_power;
    ulong min_power;
    ulong max_temp;
    ulong error_power;
		ulong gpu_def_freq;
    long powercap;
    long max_powercap;
    char energy_model[GENERIC_NAME];
    char energy_plugin[GENERIC_NAME];
    char powercap_plugin[GENERIC_NAME];
    char powercap_gpu_plugin[GENERIC_NAME];
    char coeffs[GENERIC_NAME];
} tag_t;


state_t TAG_token(char *token);
state_t TAG_parse_token(tag_t **tags_i, unsigned int *num_tags_i, char *line);
void print_tags_conf(tag_t *tag);


#endif
