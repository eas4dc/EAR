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

#include <common/types/generic.h>

/* 
 * Auxiliary structure for range_def_t
 * The first and (optional) second numbers in a range definition.
 * leading_zeroes is the amount of zeroes to the left that get removed during the conversion to 
 * integer but should be there for the nodename.
 *
 * Examples:
 * node10[00-09] would have 0 as first, 9 as second, and 1 as leading_zeroes.
 * cmp25[45-48] would have 45 as first, 48 as second, and 0 as leading_zeroes.
 * node10[000-001] would have 0 as first, 1 as second, and 2 as leading_zeroes.
 */
typedef struct pair {
    int32_t first;
    int32_t second;
    int32_t leading_zeroes;
} pair;

/* 
 * Structure to parse SLURM-like nodelists.
 * -prefix is the initial text. For example, "cmp25" in cmp25[45,46]; or "island" and "rack" in "island[0-2]rack[0-2]
 * -numbers is a list of pairs that describe the inside of a [].
 *    the possible pair combinations are in "numbers" are: 
 *    1)0-0 if NO numbers had to be set (ideally shouldn't happen and the pointer would be null with no range). cmp2546[] (useless [])
 *    2)0-N or N-M if BOTH numbers have been set. ex: cmp25[45-47]
 *    3)N-0 if only ONE number has been set. ex: cmp25[45] 
 *    The following example would have types 2 and 3: cmp25[45,47-48]. 
 *    The following is NOT an example of the type 1: cmp2545,cmp2546 -> The pairs here would be NULL pointers
 *    Type 1 is only supported as a happy accident. cmp25[,45,47-48] IS an example of types 1, 2 and 3. This
 *    will generate nodes cmp25,cmp2545,cmp2547,cmp2548
 * -next is the next range_def. in "island[0-2]rack[0-2]", "island[0-2]" would represent a range_def and "rack[0-2]" a second
 *
 * Two ranges defined as "cmp2545,cmp2546" will be defined in two separate entries, and will NOT be linked together
 * by *next
 * */
typedef struct range_def {
    char prefix[32]; //should be enough??
    pair *numbers;
    int32_t numbers_count;
    struct range_def *next;
} range_def_t;


/* Checks if a given jobid stepid is still valid in the scheduler. Only SLURM is supported for now */
uint is_job_step_valid(job_id jid, job_id sid);
int get_my_sched_node_rank();
int get_my_sched_group_node_size();
int get_my_sched_global_rank();

void sched_local_barrier(char *file,int N);
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

#endif


