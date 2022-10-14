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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef _CPU_POWER_MODEL_H_
#define _CPU_POWER_MODEL_H_

#include <common/states.h>

#include <common/hardware/architecture.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/shared_configuration.h>
#include <library/common/library_shared_data.h>

/** Loads a CPU power model. It will first try to load cpu_power_model_default.so, located at EARL's install path
 * \param libconf    The current configuration.
 * \param arch_desc  The current architecture description used for models.
 * \param load_dummy Specify whether you want to load the DUMMY CPU power model. */
state_t cpu_power_model_load(settings_conf_t *libconf, architecture_t *arch_desc,
                             uint load_dummy);

state_t cpu_power_model_init();
state_t cpu_power_model_status();

state_t cpu_power_model_project(lib_shared_data_t *data, shsignature_t *sig,
                                node_mgr_sh_data_t *nmgr);
#endif
