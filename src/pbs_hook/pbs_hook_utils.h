/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#ifndef _EAR_PBSHOOK_UTILS_H
#define _EAR_PBSHOOK_UTILS_H

#define _GNU_SOURCE
#include <common/types/generic.h>
#include <sched.h>

uint pbs_util_get_ID(int job_id, int step_id);
int pbs_util_get_eard_port(char *ear_tmp);
int pbs_util_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask, int *errno_sym);
#if 0
int pbs_util_print_affinity_mask(pid_t pid, cpu_set_t *mask);
#endif
#endif // _EAR_PBSHOOK_UTILS_H
