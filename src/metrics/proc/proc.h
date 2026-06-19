/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_PROC_H
#define METRICS_PROC_H
// clang-format off

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

typedef struct proc_s {
    pid_t  pid;
    double utime; // Time spent in user mode
    double stime; // Time spent in kernel mode
    uint   cpu_util; // CPU utilization in percentage
    timestamp_t time; // Timestamp
    double secs; // Clock time
} proc_t;

typedef struct proc_ops_s {
    void    (*unload)     ();
    state_t (*update)     (uint option, void *value);
    void    (*get_info)   (apinfo_t *info);
    state_t (*read)       (proc_t *proc);
} proc_ops_t;

// API building scheme
#define PROC_F_LOAD(name)       void    proc_##name##_load(topology_t *tp, proc_ops_t *ops, int options)
#define PROC_F_UNLOAD(name)     void    proc_##name##_unload()
#define PROC_F_UPDATE(name)     state_t proc_##name##_update(uint option, void *value)
#define PROC_F_GET_INFO(name)   void    proc_##name##_get_info(apinfo_t *info)
#define PROC_F_READ(name)       state_t proc_##name##_read(proc_t *pr)

#define PROC_DEFINES(name)    \
    PROC_F_LOAD(name);        \
    PROC_F_UNLOAD(name);      \
    PROC_F_UPDATE(name);      \
    PROC_F_GET_INFO(name);    \
    PROC_F_READ(name);

// Primer problema, necesitamos el debug en el load
void proc_load(topology_t *tp, int options);

void proc_unload();

state_t proc_update(uint option, void *value);

void proc_get_info(apinfo_t *info);

state_t proc_read(proc_t *pr);

state_t proc_read_diff(proc_t *pr2, proc_t *pr1, proc_t *prD);

state_t proc_read_copy(proc_t *pr2, proc_t *pr1, proc_t *prD);

void proc_data_diff(proc_t *pr2, proc_t *pr1, proc_t *prD);
// Reduces the diff of the data to a single system average.
void proc_data_reduce(proc_t *prD, proc_t *prA);

void proc_data_alloc(proc_t **pr);

void proc_data_free(proc_t **pr);

void proc_data_copy(proc_t *src, proc_t *dst);

void proc_data_print(proc_t *prD, int fd);

char *proc_data_tostr(proc_t *prD, char *buffer, size_t length);

// clang-format on
#endif // METRICS_PROC_H