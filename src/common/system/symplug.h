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

/** \file symplug.h
 * A module which provides methods for loading symbols from shared object files.
 * It is basically used for loading EAR plug-ins.
 */

#include <dlfcn.h>
#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h> // Not used

/** Reads a set of symbol names from a dynamic loaded shared object.
 * NULL pointers are not checked.
 *
 * \param[in] handle A handle of a dynamic loaded shared object returned by dlopen.
 * \param[out] calls An address to a pointer to an allocated space of memory to 
 *	store symbol addresses.
 * \param[in] names A pointer to a NULL-terminated strings (i.e., symbol names) array.
 * \param[in] n The number of symbols you are expected to read.
 *
 * \return EAR_ERROR If any symbol was found. */
state_t plug_join(void *handle, void *calls[], const char *names[], uint n);

// Old name
state_t symplug_open(char *path, void *calls[], const char *names[], uint n);

state_t plug_open(char *path, void *calls[], const char *names[], uint n, int flags);

state_t plug_test(void *calls[], uint n);

#endif //COMMON_SYSTEM_SYMPLUG_H
