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

#ifndef EAR_SLURM_PLUGIN_H
#define EAR_SLURM_PLUGIN_H

#include <common/types.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/config/config_install.h>
#include <daemon/remote_api/eard_rapi.h>
#include <daemon/shared_configuration.h>
#include <global_manager/eargm_rapi.h>
#include <slurm_plugin/spank_interposer.h>

#define ESPANK_STACKABLE  0 // Allows the same plugin stacking N times.
#define ESPANK_STOP      -1

#ifndef ERUN
#define NULL_C NULL
#else
#define NULL_C 0
#endif

/*
 * Job data
 */

typedef struct plug_vars {
	char ld_preload[SZ_BUFFER_EXTRA];
	char ld_library[SZ_BUFFER_EXTRA];
} plug_vars_t;

typedef struct plug_user {
	char user[SZ_NAME_MEDIUM];
	char group[SZ_NAME_MEDIUM];
	char account[SZ_NAME_MEDIUM];
	plug_vars_t env;
} plug_user_t;

typedef struct plug_job {
	new_job_req_t app;
	plug_user_t user;
	char **nodes_list;
	uint nodes_count;
} plug_job_t;

/*
 * EAR Package data
 */
typedef struct plug_freqs {
	ulong *freqs;
	int n_freqs;
} plug_freqs_t;

typedef struct plug_eard {
	settings_conf_t setts;
	services_conf_t servs;
	plug_freqs_t freqs;
	uint connected;
	uint port;
} plug_eard_t;

typedef struct plug_eargmd {
	char host[SZ_NAME_MEDIUM];
	uint connected;
	uint secured;
	uint enabled;
	uint port;
	uint min;
} plug_eargmd_t;

typedef struct plug_package {
	char nodes_excluded[SZ_PATH];
	char nodes_allowed[SZ_PATH];
	char path_temp[SZ_PATH];
	char path_inst[SZ_PATH];
	plug_eargmd_t eargmd;
	plug_eard_t eard;
} plug_package_t;

/*
 * Current subject
 */
typedef struct plug_subject {
	char host[SZ_NAME_MEDIUM];
	int context_local;
	int exit_status;
	int is_master;
} plug_subject_t;

typedef struct plug_erun {
	int is_master;
	int is_erun;
	int step_id;
	int job_id;
} plug_erun_t;

typedef struct plug_serialization {
	plug_subject_t subject;
	plug_package_t pack;
	plug_erun_t erun;
	plug_job_t job;
} plug_serialization_t;

#endif
