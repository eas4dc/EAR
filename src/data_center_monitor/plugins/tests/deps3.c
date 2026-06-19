/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <stdio.h>
#include <common/output/debug.h>
#include <common/system/plugin_manager.h>

declr_up_get_tag()
{
    *tag       = "deps3";
    *tags_deps = "<!deps1";
}

declr_up_action_init(_deps3)
{
    return "deps3 init";
}

declr_up_action_periodic(_deps1)
{
    return "deps3 periodic (coming from deps1)";
}

declr_up_action_periodic(_deps3)
{
    return "deps3 periodic";
}
