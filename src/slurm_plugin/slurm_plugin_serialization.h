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

int plug_deserialize_components(spank_t sp);

int plug_deserialize_local(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_local_alloc(spank_t sp, plug_serialization_t *sd);

int plug_serialize_remote(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_remote(spank_t sp, plug_serialization_t *sd);

int plug_serialize_task_settings(spank_t sp, plug_serialization_t *sd);

int plug_serialize_task_preload(spank_t sp, plug_serialization_t *sd);

/*
 * Cleaning functions
 */ 
int plug_clean_remote(spank_t sp);

int plug_clean_task(spank_t sp);

#endif
