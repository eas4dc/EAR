/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_SLURM_PLUGIN_REPORTS_H
#define EAR_SLURM_PLUGIN_REPORTS_H

#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

int plug_shared_readservs(spank_t sp, plug_serialization_t *sd);
int plug_shared_readfreqs(spank_t sp, plug_serialization_t *sd);
int plug_shared_readsetts(spank_t sp, plug_serialization_t *sd);

int plug_rcom_eard_job_start(spank_t sp, plug_serialization_t *sd);
int plug_rcom_eard_job_finish(spank_t sp, plug_serialization_t *sd);

int plug_rcom_eargmd_job_start(spank_t sp, plug_serialization_t *sd);
int plug_rcom_eargmd_job_finish(spank_t sp, plug_serialization_t *sd);

int plug_rcom_eard_task_start(spank_t sp, plug_serialization_t *sd);
int plug_rcom_eard_task_finish(spank_t sp, plug_serialization_t *sd);

#endif // EAR_SLURM_PLUGIN_REPORTS_H
