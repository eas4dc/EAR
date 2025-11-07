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

#include <common/output/debug.h>
#include <common/system/plugin_manager.h>
#include <data_center_monitor/plugins/metrics.h>
#include <report/report.h>
#include <string.h>

static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;

declr_up_get_tag()
{
    *tag       = "metrics_viewer";
    *tags_deps = "!metrics";
}

static void metrics_read_static(metrics_read_t *mr)
{
    metrics_data_copy(mr, &((mets_t *) plugin_manager_action("metrics"))->mr);
}

declr_up_action_init(_metrics_viewer)
{
    metrics_data_alloc(&mr1, &mr2, &mrD);
    metrics_read_static(&mr1);
    return NULL;
}

declr_up_action_periodic(_metrics_viewer)
{
    metrics_read_static(&mr2);
    metrics_data_diff(&mr2, &mr1, &mrD);
    metrics_data_copy(&mr1, &mr2);
    printf("%s", metrics_data_tostr(&mrD));
    return NULL;
}
