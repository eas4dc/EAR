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

#include <library/loader/loader.h>

int load_libear_mpi();

int load_libear_mpi_given_libmpi(char *libmpi_path);

// 'module' names are obsolete. It doesn't reflect the module where it belongs,
// in this case the loader.

// It returns if MPI is detected.
int module_mpi_is_enabled();

// It returns if the MPI version is OpenMPI.
int module_mpi_is_open();

#endif // LIBRARY_LOADER_MPI_H