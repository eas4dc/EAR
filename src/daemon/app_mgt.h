/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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


