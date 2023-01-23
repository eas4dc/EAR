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

#ifndef _EARDBD_CONF_H
#define _EARDBD_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef struct eardb_conf
{
  uint aggr_time;
  uint insr_time;
  uint tcp_port;
  uint sec_tcp_port;
  uint sync_tcp_port;
  uint mem_size;
  uint use_log;
  uchar mem_size_types[EARDBD_TYPES];
  char  plugins[SZ_PATH_INCOMPLETE];
} eardb_conf_t;



state_t EARDBD_token(char *token);
state_t EARDBD_parse_token(eardb_conf_t *conf,char *token);
void copy_eardbd_conf(eardb_conf_t *dest,eardb_conf_t *src);
void set_default_eardbd_conf(eardb_conf_t *eargmc);
void print_db_manager(eardb_conf_t *conf);

#endif
