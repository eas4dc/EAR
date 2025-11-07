/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#include <assert.h>
#include <common/hardware/hardware_info.h>
#include <pbs_hook/pbs_hook_utils.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    printf("Calling pbs_util_get_ID\n");
    assert(pbs_util_get_ID(1000, 4) != 0);
    printf("pbs_util_get_ID works well\n");

    printf("Calling pbs_util_get_eard_port\n");
    char *ear_tmp = getenv("EAR_TMP");
    assert(ear_tmp != NULL);
    assert(pbs_util_get_eard_port(ear_tmp) == 50001);
    printf("pbs_util_get_eard_port works well\n");

    printf("Calling pbs_util_setaffinity\n");
    pid_t curr_pid = getpid();
    cpu_set_t set;
    int errno_val;
    assert(pbs_util_getaffinity(curr_pid, sizeof(set), &set, &errno_val) == 0);
    print_this_affinity_mask(&set, stderr);

    return 0;
}
