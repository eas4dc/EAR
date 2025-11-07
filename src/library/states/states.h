/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_BASE_H_
#define _EAR_BASE_H_
#include <common/types.h>

/** Executed at application start. Reads EAR configuration environment variables. */
void states_begin_job(int my_id, char *app_name);

/** Executed at NEW_LOOP or END_NEW_LOOP DynAIS event */
void states_begin_period(int my_id, ulong event, ulong size, ulong level);

/** Executed at NEW_ITERATION DynAIS event */
void states_new_iteration(int my_id, uint size, uint iterations, uint level, ulong event, ulong mpi_calls_iter,
                          uint dynais_used);

/** Executed at END_LOOP or END_NEW_LOOP DynAIS event */
void states_end_period(uint iterations);

/** Executed at application end */
void states_end_job(int my_id, char *app_name);
#endif
