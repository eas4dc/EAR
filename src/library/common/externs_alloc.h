/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/


#include <linux/limits.h>
#include <common/types/application.h>
#include <common/types/loop.h>
#include <common/states.h>
#include <daemon/shared_configuration.h>

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

int ear_use_turbo = USE_TURBO; 
int ear_whole_app;
int ear_my_rank;
int my_master_rank=-1;
int my_job_id;
int my_step_id;
char my_account[GENERIC_NAME];

// DynAIS
uint loop_with_signature;
ulong last_first_event;
ulong last_calls_in_loop;
ulong last_loop_size;
ulong last_loop_level;
uint dynais_enabled = DYNAIS_ENABLED;
uint check_periodic_mode=1;
