/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_TEMPERATURE_H
#define METRICS_TEMPERATURE_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>

typedef struct temp_ops_s {
    void (*unload)();
    void (*get_info)(apinfo_t *info);
    state_t (*read)(llong *temp, llong *average);
} temp_ops_t;

#define TEMP_F_LOAD(name)     void temp_##name##_load(topology_t *tp, temp_ops_t *ops, int options)
#define TEMP_F_UNLOAD(name)   void temp_##name##_unload()
#define TEMP_F_GET_INFO(name) void temp_##name##_get_info(apinfo_t *info)
#define TEMP_F_READ(name)     state_t temp_##name##_read(llong *temp, llong *average)

#define TEMP_DEFINES(name)                                                                                             \
    TEMP_F_LOAD(name);                                                                                                 \
    TEMP_F_UNLOAD(name);                                                                                               \
    TEMP_F_GET_INFO(name);                                                                                             \
    TEMP_F_READ(name);

void temp_load(topology_t *tp, int options);

void temp_unload();

void temp_get_info(apinfo_t *info);

// Reads the last temperature value and the all devices average.
state_t temp_read(llong *temp_list, llong *average);

state_t temp_read_copy(llong *t2, llong *t1, llong *tD, llong *average);

// Data
void temp_data_alloc(llong **temp_list);

void temp_data_copy(llong *tempD, llong *tempS);

void temp_data_diff(llong *temp2, llong *temp1, llong *tempD, llong *average);

void temp_data_free(llong **temp_list);

char *temp_data_tostr(llong *list, llong avrg, char *buffer, int length);

void temp_data_print(llong *list, llong avrg, int fd);

#endif