/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_API_H_
#define _EAR_API_H_
#if MPI
#include <library/api/mpi.h>
#endif
#include <library/common/global_comm.h>

/** Initializes all the elements of the library as well as connecting to the daemon. */
void ear_init();

/** Given the information corresponding a MPI call, creates a DynAIS event
 *   and processes it as well as creating the trace. If the library is in
 *   a learning phase it does nothing. */
void ear_mpi_call(mpi_call call_type, p2i buf, p2i dest);

/** Finalizes the processes, closing and registering metrics and traces, as well as
 *   closing the connection to the daemon and releasing the memory from DynAIS. */
void ear_finalize(int exit_status);

/***** API for manual application modification *********/
void ear_new_iteration(unsigned long loop_id);

void ear_end_loop(unsigned long loop_id);

unsigned long ear_new_loop();

#endif
