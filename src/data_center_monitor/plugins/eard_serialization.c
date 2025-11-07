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
#include <daemon/local_api/eard_api.h>
#include <stdio.h>

static application_t app;

declr_up_get_tag()
{
    *tag       = "eard_serialization";
    *tags_deps = NULL;
}

declr_up_action_init(_eardcon)
{
    eards_write_app_signature(&app);
    return "SENDED";
}
