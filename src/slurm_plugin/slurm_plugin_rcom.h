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

#endif //EAR_SLURM_PLUGIN_REPORTS_H
