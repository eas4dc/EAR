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

#include <linux/limits.h>
#include <common/types/application.h>
#include <common/types/loop.h>
#include <common/states.h>
#include <daemon/shared_configuration.h>
#include <library/common/library_shared_data.h>
#include <library/api/clasify.h>

loop_t loop;
application_t loop_signature;
application_t application;
settings_conf_t *system_conf=NULL;
resched_t *resched_conf=NULL;
char system_conf_path[PATH_MAX];
char resched_conf_path[PATH_MAX];
char node_name[PATH_MAX];

char loop_summary_path[PATH_MAX];
char app_summary_path[PATH_MAX];
char ear_app_name[PATH_MAX]; //TODO: use application.app_id

// Common variables
ulong ear_frequency; 
ulong EAR_default_frequency; 
uint EAR_default_pstate;
timestamp ear_application_time_init;

int ear_use_turbo = USE_TURBO; 
int ear_whole_app;
int ear_my_rank;
int my_master_rank=-1;
int my_job_id;
int my_step_id;
char my_account[GENERIC_NAME];
int eard_ok=1;

// DynAIS
uint loop_with_signature;
ulong last_first_event;
ulong last_calls_in_loop;
ulong last_loop_size;
ulong last_loop_level;
uint dynais_enabled = DYNAIS_ENABLED;
uint check_periodic_mode=1;

lib_shared_data_t *lib_shared_region=NULL;
shsignature_t *sig_shared_region=NULL;
int my_node_id;

uint sh_sig_per_node=1;
uint sh_sig_per_proces=0;
uint show_signatures=0;
uint report_node_sig=0;
uint report_all_sig=0;

uint earl_phase_classification = APP_COMP_BOUND;
topology_t mtopo;
ulong *cpufreq_data_per_core;

