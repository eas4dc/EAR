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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef _EAR_LIB_CONF_H
#define _EAR_LIB_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef struct earlib_conf
{
	char coefficients_pathname[GENERIC_NAME];
    uint dynais_levels;
    uint dynais_window;
	uint dynais_timeout;
	uint lib_period;
	uint check_every;
    char plugins[SZ_PATH_INCOMPLETE];
} earlib_conf_t;


state_t EARLIB_token(char *token);
state_t EARLIB_parse_token(earlib_conf_t *conf,char *token);

void set_default_earlib_conf(earlib_conf_t *earlibc);
void copy_ear_lib_conf(earlib_conf_t *dest,earlib_conf_t *src);
void print_earlib_conf(earlib_conf_t *conf);


#endif
