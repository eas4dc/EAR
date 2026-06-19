/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/output/verbose.h>
#include <global_manager/plugins/powercap_plugin.h>

state_t plugin_init(cluster_conf_t conf[static 1])
{
    verbose(VGM_PC, "Dummy plugin init");
    return EAR_SUCCESS;
}

state_t plugin_hard_powercap_decision(powercap_status_t status[static 1], powercap_opt_t opt[static 1],
                                      bool send_options[static 1], cluster_powercap_ctx_t ctx[static 1])
{
    verbose(VGM_PC, "Dummy plugin hard powercap decision");
    return EAR_SUCCESS;
}

state_t plugin_soft_powercap_decision(power_check_t power[static 1])
{
    verbose(VGM_PC, "Dummy plugin soft powercap decision");
    return EAR_SUCCESS;
}
