/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_EAR_MPI_H
#define LIBRARY_EAR_MPI_H

#include <library/api/mpi.h>

void before_init();

void after_init();

void before_mpi(mpi_call call_type, p2i buf, p2i dest);

void after_mpi(mpi_call call_type);

void before_finalize();

void after_finalize();

#endif // LIBRARY_EAR_MPI_H
