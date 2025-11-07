/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARL_EXTERNS_ALLOC_H
#define _EARL_EXTERNS_ALLOC_H

#include <common/sizes.h>
#include <common/states.h>
#include <common/types/application.h>
#include <common/types/loop.h>
#include <daemon/shared_configuration.h>
#include <library/api/clasify.h>
#include <library/common/library_shared_data.h>

// application_t loop_signature;
application_t application;
settings_conf_t *system_conf = NULL;
resched_t *resched_conf      = NULL;
char system_conf_path[SZ_PATH];
char resched_conf_path[SZ_PATH];
char node_name[SZ_NAME_MEDIUM];

char loop_summary_path[SZ_PATH];
char app_summary_path[SZ_PATH];
char ear_app_name[SZ_PATH]; // TODO: use application.app_id

// Common variables
ulong ear_frequency;
ulong EAR_default_frequency;
uint EAR_default_pstate;
timestamp ear_application_time_init;

int ear_use_turbo = USE_TURBO;
int ear_whole_app;
int ear_my_rank;
int my_master_rank = -1;
int my_job_id;
int my_step_id;
char my_account[GENERIC_NAME];
int eard_ok = 1;

// DynAIS
uint loop_with_signature;
ulong last_first_event;
ulong last_calls_in_loop;
ulong last_loop_size;
ulong last_loop_level;
uint dynais_enabled      = DYNAIS_ENABLED;
uint check_periodic_mode = 1;

lib_shared_data_t *lib_shared_region = NULL;
shsignature_t *sig_shared_region     = NULL;
int my_node_id;

uint sh_sig_per_node   = 1;
uint sh_sig_per_proces = 0;
uint report_node_sig   = 0;
uint report_all_sig    = 0;

uint earl_phase_classification = APP_COMP_BOUND;
topology_t mtopo;

#endif // _EARL_EXTERNS_ALLOC_H
