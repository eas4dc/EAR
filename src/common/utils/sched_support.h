/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _SCHED_SUPPORT_H
#define _SCHED_SUPPORT_H

#include <common/types/configuration/node_conf.h>
#include <common/types/generic.h>

/* Checks if a given jobid stepid is still valid in the scheduler. Only SLURM is supported for now */
uint is_job_step_valid(job_id jid, job_id sid);
int get_my_sched_node_rank();
int get_my_sched_group_node_size();
int get_my_sched_global_rank();

void sched_local_barrier(char *file, int N);
void remove_local_barrier(char *file);

/* Parses a SLURM-like nodelist and places it in range_list. Returns the number of
 * range_def_t contained in range_list. */
int32_t parse_range_list(const char *source, range_def_t **range_list);

/* Given a SLURM-like nodelist in source, it expands it and places it in target.
 * If the fully expanded list does not fit, it will only fill as many full nodes as it can.
 */
int32_t expand_list(const char *source, size_t target_length, char target[target_length]);

/* The same as before, but it will handle the memory allocation.
 * usage example:
 * char *expanded_list = NULL;
 * expand_list_alloc("cmp25[45-48], &expanded_list);
 */
int32_t expand_list_alloc(const char *source, char **target);

/* Given a range_def_t cr, a base (should be empty, i.e. "" in the first call), a max_buffer size and a buffer,
 * puts all the nodes expanded in the final form (node0island0,node1island0,etc) separated by a comma into buffer.
 * Ths size of the final string is stored in current_size.
 * Returns 0 on success. If it fails, it returns the number of nodes that it could not append */
int32_t concat_range(char *base, range_def_t *cr, size_t max_size, size_t *current_size, char buffer[max_size]);
#endif
