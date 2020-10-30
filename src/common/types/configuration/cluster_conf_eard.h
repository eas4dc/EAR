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
  uint    use_log;
} eard_conf_t;



state_t EARD_token(char *token);
state_t EARD_parse_token(eard_conf_t *conf,char *token);
void copy_eard_conf(eard_conf_t *dest,eard_conf_t *src);
void set_default_eard_conf(eard_conf_t *eard);
void print_eard_conf(eard_conf_t *conf);

#endif
