/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
