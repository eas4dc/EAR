/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_SLURM_PLUGIN_ENV_H
#define EAR_SLURM_PLUGIN_ENV_H

#include <slurm_plugin/slurm_plugin.h>

/*
 * Reading function
 */
int plug_read_plugstack(spank_t sp, int ac, char **av, plug_serialization_t *sd);

int plug_print_application(spank_t sp, new_job_req_t *app);

int plug_read_application(spank_t sp, plug_serialization_t *sd);

int plug_read_hostlist(spank_t sp, plug_serialization_t *sd);

/*
 * Serialization functions
 */
int plug_print_variables(spank_t sp);

int plug_print_items(spank_t sp);

int plug_deserialize_components(spank_t sp);

int plug_deserialize_local(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_local_alloc(spank_t sp, plug_serialization_t *sd);

int plug_serialize_remote(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_remote(spank_t sp, plug_serialization_t *sd);

int plug_serialize_task_settings(spank_t sp, plug_serialization_t *sd);

int plug_serialize_task_preload(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_task(spank_t sp, plug_serialization_t *sd);

/*
 * Cleaning functions
 */
int plug_clean_remote(spank_t sp);

int plug_clean_task(spank_t sp);

#endif