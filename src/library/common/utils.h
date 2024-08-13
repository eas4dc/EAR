/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _LIB_UTILS_H
#define _LIB_UTILS_H

#include <common/states.h>

/** Fills the string \ref result_path with the must-to-be-used EARL plug-ins path.
 * The output string will be either the \ref install_path or \ref custom_path with "/plugins" string appended. The latter is selected if \ref authorized_user is true.
 * \return EAR_ERROR An input argument is NULL.
 * \return EAR_SUCCESS Otherwise. */
state_t utils_create_plugin_path(char *result_path, char *install_path, char *custom_path, int authorized_user);

/** Returns the maximum value between the batch scheduler provided number of nodes of the active job (SLURM tested) and 1. It formally calls SCHED_STEP_NUM_NODES env var.*/
int utils_get_sched_num_nodes();
#endif
