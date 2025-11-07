/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_SLURM_PLUGIN_H
#define EAR_SLURM_PLUGIN_H

#include <common/config/config_install.h>
#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/types.h>
#include <common/types/application.h>
#include <daemon/remote_api/eard_rapi.h>
#include <daemon/shared_configuration.h>
#include <global_manager/eargm_rapi.h>
#include <slurm_plugin/spank_interposer.h>

#define ESPANK_STACKABLE 0 // Allows the same plugin stacking N times.
#define ESPANK_STOP      -1

#ifndef ERUN
#define NULL_C NULL
#else
#define NULL_C 0
#endif

/*
 * Job data
 */

typedef struct plug_user {
    char user[SZ_NAME_MEDIUM];
    char group[SZ_NAME_MEDIUM];
    char account[SZ_NAME_MEDIUM];
} plug_user_t;

typedef struct plug_job {
    new_job_req_t app;
    new_task_req_t task;
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
    char path_temp[SZ_PATH];
    char path_inst[SZ_PATH];
    plug_eargmd_t eargmd;
    plug_eard_t eard;
    char nodes_excluded[SZ_PATH];
    char nodes_allowed[SZ_PATH];
} plug_package_t;

/*
 * Current subject
 */
typedef struct plug_subject {
    char host[SZ_NAME_MEDIUM];
    int context_local;
    int exit_status;
    int is_task_master;
    int is_node_master;
} plug_subject_t;

typedef struct plug_erun {
    int avoid_connecting;
    int is_step_id;
    int is_master;
    int is_erun;
    int step_id;
    int job_id;
} plug_erun_t;

typedef struct plug_serialization {
    plug_subject_t subject; // Self process
    plug_package_t pack;    // Paths, addresses and ports
    plug_job_t job;
    plug_erun_t erun;
} plug_serialization_t;

#endif
