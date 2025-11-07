/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/system/plugin_manager.h>
#include <common/utils/string.h>
#include <cuda_runtime.h>

int kcuda_is_running();
int kcuda_count_devices();
int kcuda_execute(char **conf);

declr_up_get_tag()
{
    *tag       = "kernel_cuda";
    *tags_deps = NULL;
}

declr_up_action_init(_kernel_cuda)
{
    char **args = (char **) data;

    if (!kcuda_count_devices()) {
        return "[D] no GPUs detected";
    }
    if (!kcuda_execute(args)) {
        return "[D] something is not working";
    }
    return rsprintf("%d GPUs detected", kcuda_count_devices());
}

declr_up_action_periodic(_kernel_cuda)
{
    // return NULL;
    return rsprintf("Is running CUDA kernel? %d", kcuda_is_running());
}
