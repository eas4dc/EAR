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
#include <report/report.h>
#include <string.h>

static ullong zero = 0LLU;

declr_up_get_tag()
{
    *tag       = "dummy";
    *tags_deps = NULL;
}

declr_up_action_init(_dummy)
{
    *data_alloc = &zero;
    debug("dummy init");
    return rsprintf("dummy action init(_dummy): received tag %s", tag);
}

declr_up_action_init(_conf)
{
    return rsprintf("dummy action init(_conf): received tag %s", tag);
}

declr_up_action_periodic(_dummy)
{
    debug("dummy action");
    return rsprintf("dummy action periodic(_dummy): received tag %s", tag);
}

declr_up_post_data()
{
    return rsprintf("dummy post data: received msg %s", msg);
}

declr_up_action_close()
{
    return NULL;
}