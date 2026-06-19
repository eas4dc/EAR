/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IMCFREQ_H
#define METRICS_IMCFREQ_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/types.h>
#include <metrics/common/apis.h>
#include <metrics/common/pstate.h>

typedef struct imcfreq_s {
    timestamp_t time;
    ulong freq; // KHz
    uint error;
} imcfreq_t;

typedef struct imcfreq_ops_s {
    void (*unload)();
    void (*get_info)(apinfo_t *info);
    state_t (*read)(imcfreq_t *list);
    void (*data_diff)(imcfreq_t *l2, imcfreq_t *l1, ulong *ldiff, ulong *freq_avg);
} imcfreq_ops_t;

#define IMCFREQ_F_LOAD(name)     void imcfreq_##name##_load(topology_t *tp_in, imcfreq_ops_t *ops, int options)
#define IMCFREQ_F_UNLOAD(name)   void imcfreq_##name##_unload()
#define IMCFREQ_F_GET_INFO(name) void imcfreq_##name##_get_info(apinfo_t *info)
#define IMCFREQ_F_READ(name)     state_t imcfreq_##name##_read(imcfreq_t *list)
#define IMCFREQ_F_DATA_DIFF(name)                                                                                      \
    void imcfreq_##name##_data_diff(imcfreq_t *l2, imcfreq_t *l1, ulong *ldiff, ulong *freq_avg)

#define IMCFREQ_DEFINES(name)                                                                                          \
    IMCFREQ_F_LOAD(name);                                                                                              \
    IMCFREQ_F_UNLOAD(name);                                                                                            \
    IMCFREQ_F_GET_INFO(name);                                                                                          \
    IMCFREQ_F_READ(name);                                                                                              \
    IMCFREQ_F_DATA_DIFF(name);

void imcfreq_load(topology_t *tp, int options);

void imcfreq_unload();

void imcfreq_get_info(apinfo_t *info);

state_t imcfreq_read(imcfreq_t *l);

state_t imcfreq_read_diff(imcfreq_t *l2, imcfreq_t *l1, ulong *l_diff, ulong *freq_avg);

state_t imcfreq_read_copy(imcfreq_t *l2, imcfreq_t *l1, ulong *l_diff, ulong *freq_avg);
// Frequency is KHz
void imcfreq_data_diff(imcfreq_t *l2, imcfreq_t *l1, ulong *l_diff, ulong *freq_avg);

// Helpers
void imcfreq_data_alloc(imcfreq_t **l, ulong **l_diff);

void imcfreq_data_free(imcfreq_t **l, ulong **l_diff);

void imcfreq_data_copy(imcfreq_t *l2, imcfreq_t *l1);

void imcfreq_data_print(ulong *l_diff, ulong *freq_avg, int fd);

char *imcfreq_data_tostr(ulong *l_diff, ulong *freq_avg, char *buffer, size_t length);

#endif // METRICS_IMCFREQ_H