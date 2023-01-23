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

#ifndef _APP_MGT_H
#define _APP_MGT_H
#define _GNU_SOURCE
#include <sched.h>


typedef struct app_management_data{
  uint master_rank;
  uint ppn;
  uint nodes;
  uint total_processes;
  uint max_ppn;
	cpu_set_t node_mask;
}app_mgt_t;


void app_mgt_new_job(app_mgt_t *a);
void app_mgt_end_job(app_mgt_t *a);
void print_app_mgt_data(app_mgt_t *a);
uint is_app_master(app_mgt_t *a);

#endif


