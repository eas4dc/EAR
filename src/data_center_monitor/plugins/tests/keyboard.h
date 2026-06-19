/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef UP_KEYBOARD_H
#define UP_KEYBOARD_H

#include <common/system/plugin_manager.h>

typedef void (*callback_t)(char **parameters);

void command_register(cchar *command, cchar *format, callback_t call);

#endif // UP_CONFIG_H