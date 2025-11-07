/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#define _GNU_SOURCE

#include <common/hardware/hardware_info.h>
#include <common/types/application.h>
#include <daemon/shared_configuration.h>
#include <errno.h>
#include <pbs_hook/pbs_hook_utils.h>
#include <sched.h>
#include <stdio.h>

uint pbs_util_get_ID(int job_id, int step_id)
{
    return create_ID(job_id, step_id);
}

int pbs_util_get_eard_port(char *ear_tmp)
{
    int ret;
    char path[GENERIC_NAME];

    ret = get_services_conf_path(ear_tmp, path);
    if (state_ok(ret)) {
        services_conf_t *services_conf;
        services_conf = attach_services_conf_shared_area(path);
        if (services_conf == NULL) {
            return -1;
        }

        return services_conf->eard.port;
    } else {
        return -1;
    }
}

int pbs_util_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask, int *errno_sym)
{
    int ret = sched_getaffinity(pid, cpusetsize, mask);
    if (ret < 0) {
        *errno_sym = errno;
    }
#if SHOW_DEBUGS
    else {
        FILE *fd;
        fd = fopen("/hpc/xshared/xovidal/PBS/PBSPro_2021.1.1/hooks/verbose.txt", "a");
        fprintf(fd, "Mask affinity for process %d:\n", pid);
        print_this_affinity_mask(mask, fd);
    }
#endif
    return ret;
}

#if 0
int pbs_util_print_affinity_mask(pid_t pid, cpu_set_t *mask)
{
    FILE *fd;
    fd = fopen("/hpc/xshared/xovidal/PBS/PBSPro_2021.1.1/hooks/verbose.txt", "a");
    fprintf(fd, "Mask affinity for process %d:\n", pid);
    print_this_affinity_mask(mask, fd);

    return 0;
}
#endif
