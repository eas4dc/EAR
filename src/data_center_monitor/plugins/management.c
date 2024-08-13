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

#include <stdio.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <data_center_monitor/plugins/conf.h>
#include <data_center_monitor/plugins/management.h>

static mans_t     m;
static conf_t    *conf;

declr_up_get_tag()
{
    *tag = "management";
    *tags_deps = "!conf+!metrics";
}

declr_up_action_init(_conf)
{
    conf = (conf_t *) data;
    return NULL;
}

declr_up_action_init(_management)
{
    *data_alloc = &m;
    management_init(&m.mi, &conf->tp, atoull(getenv("MFLAGS")));
    return rsprintf("Management plugin initialized correctly");
}
