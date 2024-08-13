/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_SYSTEM_SYMPLUG_H
#define COMMON_SYSTEM_SYMPLUG_H

#include <dlfcn.h>
#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>

state_t plug_join(void *handle, void *calls[], const char *names[], uint n);
// Old name
state_t symplug_open(char *path, void *calls[], const char *names[], uint n);

state_t plug_open(char *path, void *calls[], const char *names[], uint n, int flags);

state_t plug_test(void *calls[], uint n);

#endif //COMMON_SYSTEM_SYMPLUG_H
