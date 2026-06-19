/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#pragma once

#include <daemon/remote_api/eard_rapi.h>
#include <global_manager/cluster_powercap.h>

typedef struct eargm_powercap_functions {
    state_t (*plugin_init)(cluster_conf_t conf[static 1]);
    state_t (*plugin_hard_powercap_decision)(powercap_status_t status[static 1], powercap_opt_t opt[static 1],
                                             bool send_options[static 1], cluster_powercap_ctx_t ctx[static 1]);
    state_t (*plugin_soft_powercap_decision)(power_check_t power[static 1]);
} eargm_powercap_symbols_t;

state_t eargm_plugin_load(const char *path, const char *plugs);
state_t eargm_plugin_init(cluster_conf_t conf[static 1]);
state_t eargm_plugin_hard_powercap_decision(powercap_status_t status[static 1], powercap_opt_t opt[static 1],
                                            bool send_options[static 1], cluster_powercap_ctx_t ctx[static 1]);
state_t eargm_plugin_soft_powercap_decision(power_check_t power[static 1]);
