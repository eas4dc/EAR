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
#include <library/common/externs_alloc.h>
#include <library/common/global_comm.h>
#include <library/policies/policy_plugin.h>
#include <stdio.h>
#include <stdlib.h>

masters_info_t masters_info;

int main(int argc, char **argv)
{
    VERB_SET_LV(2);
    p_plugin_t plugin = policy_plugin_load("policy_plugins/test.so");
    if (!plugin) {
        printf("Error\n");
        return EXIT_FAILURE;
    }

    polctx_t c;
    signature_t sig;
    node_freqs_t freqs;
    int ready;
    loop_id_t loop_id;
    node_freq_domain_t domain;

    policy_plugin_init(plugin, &c);
    policy_plugin_apply(plugin, &c, &sig, &freqs, &ready);
    policy_plugin_app_apply(plugin, &c, &sig, &freqs, &ready);
    policy_plugin_get_default_freq(plugin, &c, &freqs, &sig);
    policy_plugin_ok(plugin, &c, &sig, &sig, &ready);
    policy_plugin_max_tries(plugin, &c, &ready);
    policy_plugin_end(plugin, &c);
    policy_plugin_loop_init(plugin, &c, &loop_id);
    policy_plugin_loop_end(plugin, &c, &loop_id);
    policy_plugin_new_iteration(plugin, &c, &sig);
    policy_plugin_mpi_init(plugin, &c, 0, &freqs, &ready);
    policy_plugin_mpi_end(plugin, &c, 0, &freqs, &ready);
    policy_plugin_configure(plugin, &c);
    policy_plugin_domain(plugin, &c, &domain);
    policy_plugin_io_settings(plugin, &c, &sig, &freqs);
    policy_plugin_cpu_gpu_settings(plugin, &c, &sig, &freqs);
    policy_plugin_busy_wait_settings(plugin, &c, &sig, &freqs);
    policy_plugin_restore_settings(plugin, &c, &sig, &freqs);

    policy_plugin_dispose(plugin);

    return EXIT_SUCCESS;
}
