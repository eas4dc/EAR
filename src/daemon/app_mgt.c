/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/output/verbose.h>
#include <daemon/app_mgt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void app_mgt_new_job(app_mgt_t *a)
{
    if (a == NULL)
        return;
    memset(a, 0, sizeof(app_mgt_t));
}

void app_mgt_end_job(app_mgt_t *a)
{
    if (a == NULL)
        return;
    memset(a, 0, sizeof(app_mgt_t));
}

void print_app_mgt_data(app_mgt_t *a)
{
    if (a == NULL)
        return;

    verbose(VEARD_NMGR, "App_info: master_rank %u ppn %u nodes %u total_processes %u max_ppn %u", a->master_rank,
            a->ppn, a->nodes, a->total_processes, a->max_ppn);
}

uint is_app_master(app_mgt_t *a)
{
    if (a == NULL)
        return 0;
    return (a->master_rank == 0);
}
