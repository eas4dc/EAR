/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_LOADER_COMMON_H
#define LIBRARY_LOADER_COMMON_H

#include <library/loader/loader.h>

// Depcrecated, clean.
int is_cuda_enabled();

// These functions are intended to be used ONLY for the loader
int is_libear_loaded();

void set_libear_loaded();

// Returns the complete libear path
void loader_get_path_libear(char **path_lib, char **hack_lib);

int load_libear_nonspec(char *lib_extension, char *syms[], char *libs[]);

#endif