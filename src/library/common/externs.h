/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_GLOBAL_H
#define _EAR_GLOBAL_H

#include <common/sizes.h>
#include <common/types/loop.h>
#include <common/system/time.h>
#include <common/hardware/architecture.h>
#include <common/types/application.h>
#include <library/common/global_comm.h>
#include <library/common/library_shared_data.h>
#include <daemon/shared_configuration.h>

extern application_t application;
extern settings_conf_t *system_conf;
extern resched_t *resched_conf;
extern char system_conf_path[SZ_PATH];
extern char resched_conf_path[SZ_PATH];
extern char node_name[SZ_NAME_MEDIUM];


extern char loop_summary_path[SZ_PATH];
extern char app_summary_path[SZ_PATH];
extern char ear_app_name[SZ_PATH];

// Common variables
extern unsigned long ear_frequency;
extern unsigned long EAR_default_frequency;
extern unsigned int EAR_default_pstate;
extern timestamp_t ear_application_time_init;

extern int ear_use_turbo;
extern int ear_whole_app;
extern int ear_my_rank;
extern int eard_ok;
//extern int my_master_rank;
extern int my_job_id;
extern int my_step_id;
extern char my_account[GENERIC_NAME];

// DynAIS
extern uint loop_with_signature;
extern ulong last_first_event;
extern ulong last_calls_in_loop;
extern ulong last_loop_size;
extern ulong last_loop_level;
extern uint dynais_enabled;
extern uint check_periodic_mode;

// Shared regions for processes in same node
extern lib_shared_data_t *lib_shared_region;
extern shsignature_t *sig_shared_region;
extern int my_node_id;
extern masters_info_t masters_info;
extern cpu_set_t ear_process_mask;
extern int ear_affinity_is_set;
extern architecture_t arch_desc;


extern uint sh_sig_per_node;
extern uint sh_sig_per_proces;
extern uint report_node_sig;
extern uint report_all_sig;
extern uint earl_phase_classification;
extern topology_t mtopo;

#endif
