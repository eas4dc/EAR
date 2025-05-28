/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_POWERCAP_H
#define _EAR_POWERCAP_H
#include <pthread.h>
#include <common/types/pc_app_info.h>
#include <daemon/powercap/powercap_status_conf.h>

#define PC_MODE_AUTO   1
#define PC_MODE_MANUAL 2

#define DEFAULT_CPU_TDP 150

int powercap_init();
void powercap_end();
//void get_powercap_status(powercap_status_t *my_status);
void powercap_get_status(powercap_status_t *my_status, pmgt_status_t *status, int release_power);
void powercap_set_opt(powercap_opt_t *opt, int id);
uint powercap_get_value();
void print_powercap_opt(powercap_opt_t *opt);
int powercap_idle_to_run();
int powercap_run_to_idle();
int powercap_init();
int powercap_set_power_per_domain(dom_power_t *pdomain,uint use_earl,ulong eff_f);

void set_powercapstatus_mode(uint mode);

void copy_node_powercap_opt(node_powercap_opt_t *dst);
uint powercap_get_cpu_strategy();
uint powercap_get_gpu_strategy();
void powercap_set_app_req_freq();
void powercap_release_idle_power(pc_release_data_t *release);
void powercap_reset_default_power();
void powercap_reduce_def_power(uint power);
void powercap_increase_def_power(uint power);
void powercap_set_powercap(uint power);
void powercap_process_message(char *action, char *mode, char *domain, int32_t num_values, int32_t values[num_values]);
ulong powercap_elapsed_last_powercap();

void powercap_new_job();
void powercap_end_job();

#endif
