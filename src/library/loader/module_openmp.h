/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_LOADER_OPENMP_H
#define LIBRARY_LOADER_OPENMP_H

int module_constructor_openmp(char *path_lib_so,char *libhack);

void module_destructor_openmp();

#endif
