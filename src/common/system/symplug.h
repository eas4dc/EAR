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
/* clang-format off */

#include <dlfcn.h>
#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>

// This module provides methods for loading symbols from shared object files.

// This function calls dlsym() internally. Given 'n' function 'names' from the
// shared object, the function symbols are loaded and stored in 'calls' in order.
state_t plug_join(void *handle, void *calls[], const char *names[], uint n);

// This function calls dlopen() and dlsym() internally. The flags parameter is
// the same as the one used by dlopen().
state_t plug_open(char *path, void *calls[], const char *names[], uint n, int flags);

// This version returns the dlopen() handler instead a state.
void *plug_open2(char *path, void *calls[], const char *names[], uint n, int flags);

// Old name (depcrecated)
state_t symplug_open(char *path, void *calls[], const char *names[], uint n);

/* clang-format on */
#endif // COMMON_SYSTEM_SYMPLUG_H