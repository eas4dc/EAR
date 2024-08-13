/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_LOADER_MPI_H
#define LIBRARY_LOADER_MPI_H

int module_mpi(char *path_lib_so, char *libhack);

void module_mpi_destructor();

int module_mpi_is_enabled();

int module_mpi_is_intel();

int module_mpi_is_open();

int module_mpi_is_detected();

void module_mpi_set_forced();

#endif //LIBRARY_LOADER_MPI_H