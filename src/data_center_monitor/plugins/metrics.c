/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <data_center_monitor/plugins/conf.h>
#include <data_center_monitor/plugins/metrics.h>

static mets_t     m;
static conf_t    *conf;
static char      *np_path; //Node power
static char       buffer[4096];

declr_up_get_tag()
{
    *tag = "metrics";
    *tags_deps = "!conf";
}

declr_up_action_init(_conf)
{
    conf = (conf_t *) data;
    sprintf(buffer, "%s/energy/%s", conf->cluster.install.dir_plug, conf->cluster.install.obj_ener);
    np_path = buffer;
    return NULL;
}

declr_up_action_init(_metrics)
{
    *data_alloc = &m;
    metrics_load(&m.mi, &conf->tp, np_path, atoull(getenv("MFLAGS")));
    metrics_data_alloc(&m.mr, NULL, NULL);
    return rsprintf("Metrics plugin initialized correctly %s", (!np_path) ? "(missing np library)": "");
}

declr_up_action_periodic(_metrics)
{
    metrics_read(&m.mr);
    return NULL;
}
