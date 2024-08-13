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
#include <common/output/debug.h>
#include <common/system/plugin_manager.h>
#include <daemon/local_api/eard_api.h>

declr_up_get_tag()
{
    *tag = "eardcon";
    *tags_deps = NULL;
}

declr_up_action_init(_eardcon)
{
    eards_connection();
    return rsprintf("EARD connection state: %s", eards_connected() ? "CONNECTED": "NOT CONNECTED");
}
