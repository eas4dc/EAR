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
#include <common/states.h>
#include <library/metrics/dcgmi_lib.h>
#include <metrics/gpu/gpu.h>

int main(int argc, char **argv)
{
    verb(0, "Loading GPU API...");
    gpu_load(0); // 0 means no forcing any API
    gpu_init(no_ctx);

    VERB_SET_LV(4);

    verb(0, "Loading EARL DCGMI module...");
    if (state_fail(dcgmi_lib_load())) {
        verb(0, "Error loading dcgmi_lib module: %s", state_msg);
        return -1;
    }

    sleep(30);

    return 0;
}
