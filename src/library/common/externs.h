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

#ifndef _EAR_GLOBAL_H
#define _EAR_GLOBAL_H

#include <linux/limits.h>
#include <common/types/application.h>
#include <common/types/loop.h>
#include <daemon/shared_configuration.h>
#include <library/common/library_shared_data.h>
#include <library/common/global_comm.h>

extern loop_t loop;
extern application_t loop_signature;
extern application_t application;
extern settings_conf_t *system_conf;
extern resched_t *resched_conf;
extern char system_conf_path[PATH_MAX];
extern char resched_conf_path[PATH_MAX];
extern char node_name[PATH_MAX];


extern char loop_summary_path[PATH_MAX];
extern char app_summary_path[PATH_MAX];
extern char ear_app_name[PATH_MAX];

// Common variables
extern unsigned long ear_frequency;
extern unsigned long EAR_default_frequency;
extern unsigned int EAR_default_pstate;

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


#endif
