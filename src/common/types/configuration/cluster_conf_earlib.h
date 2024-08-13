/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
