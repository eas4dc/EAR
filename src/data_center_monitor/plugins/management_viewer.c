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

#include <string.h>
#include <report/report.h>
#include <common/output/debug.h>
#include <common/system/plugin_manager.h>
#include <data_center_monitor/plugins/management.h>

declr_up_get_tag()
{
    *tag = "management_viewer";
    *tags_deps = "!management";
}

declr_up_action_init(_management_viewer)
{
    printf("%s", management_data_tostr());
    return NULL;
}
