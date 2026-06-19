/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_H
#define MANAGEMENT_H
/* clang-format off */

#include <management/cpufreq/cpufreq.h>
#include <management/cpufreq/priority.h>
#include <management/cpupow/cpupow.h>
#include <management/gpu/gpu.h>
#include <management/imcfreq/imcfreq.h>
#include <metrics/metrics.h>

typedef struct manages_info_s {
    apinfo_t cpufreq;
    apinfo_t cpuprio;
    apinfo_t imcfreq;
    apinfo_t cpupow;
    apinfo_t gpu;
} manages_info_t;

#define MGT_OPT_CPUFREQ 0
#define MGT_OPT_IMCFREQ 1
#define MGT_OPT_GPU     2
#define MGT_OPT_CPUPOW  3

void management_load(manages_info_t *man, topology_t *tp, uint *options);

void management_info_get(manages_info_t *m);

char *management_info_tostr(manages_info_t *m, char *buffer);

void management_info_print(manages_info_t *m, int fd);

char *management_data_tostr();

/* clang-format on */
#endif // MANAGEMENT_H
