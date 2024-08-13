/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_CONF_H
#define _EARD_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>



typedef struct eard_conf
{ 
  uint verbose;     /* default 1 */
  ulong period_powermon;  /* default 30000000 (30secs) */
  ulong max_pstate;     /* default 1 */
  uint turbo;       /* Fixed to 0 by the moment */
  uint port;        /* mandatory */
  uint use_mysql;     /* Must EARD report to DB */
  uint use_eardbd;    /* Must EARD report to DB using EARDBD */
  uint force_frequencies; /* 1=EARD will force pstates specified in policies , 0=will not */
  uint use_log;
  char plugins[SZ_PATH_INCOMPLETE];
} eard_conf_t;



state_t EARD_token(char *token);
state_t EARD_parse_token(eard_conf_t *conf,char *token);
void copy_eard_conf(eard_conf_t *dest,eard_conf_t *src);
void set_default_eard_conf(eard_conf_t *eard);
void print_eard_conf(eard_conf_t *conf);

#endif
