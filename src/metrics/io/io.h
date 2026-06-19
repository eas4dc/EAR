/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IO_H
#define METRICS_IO_H
// clang-format off

#include <common/types.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>
#include <metrics/common/apis.h>

typedef struct io_s {
    pid_t  pid;
    ullong rchar; // Bytes given to the process by syscalls (read, recv...)
    ullong wchar; // ^
    ullong syscr; // Read syscalls
    ullong syscw; // Write syscalls
    union {
        ullong read_bytes; // Bytes read from storage device
        ullong rstor;
    };
    union {
        ullong write_bytes; // Bytes written into storage device
        ullong wstor;
    };
    union {
        timestamp_t time;
        double secs;
    };
    ullong cancelled;
} io_t;

//#define io_data_t io_t

typedef struct io_ops_s {
    void    (*unload)     ();
    state_t (*update)     (uint option, void *value);
    void    (*pids_clean) ();
    void    (*get_info)   (apinfo_t *info);
    state_t (*read)       (io_t *io);
    void    (*data_diff)  (io_t *io2, io_t *io1, ulong *io_diff, double *mbs);
} io_ops_t;

#define IO_F_LOAD(name)       void io_##name##_load(topology_t *tp, io_ops_t *ops, int options)
#define IO_F_UNLOAD(name)     void io_##name##_unload()
#define IO_F_UPDATE(name)     state_t io_##name##_update(uint option, void *value)
#define IO_F_GET_INFO(name)   void io_##name##_get_info(apinfo_t *info)
#define IO_F_READ(name)       state_t io_##name##_read(io_t *io)
#define IO_F_DATA_DIFF(name)  void io_##name##_data_diff(io_t *io2, io_t *io1, ulong *io_diff, double *mbs)

#define IO_DEFINES(name)         \
    IO_F_LOAD(name);             \
    IO_F_UNLOAD(name);           \
    IO_F_UPDATE(name);           \
    IO_F_GET_INFO(name);         \
    IO_F_READ(name);             \
    IO_F_DATA_DIFF(name);

void io_load(topology_t *tp, int options);

void io_unload();

state_t io_update(uint option, void *value);

void io_get_info(apinfo_t *info);

state_t io_read(io_t *io);

state_t io_read_diff(io_t *io2, io_t *io1, io_t *io_diff, double *mbs);

state_t io_read_copy(io_t *io2, io_t *io1, io_t *io_diff, double *mbs);
// mbs is the mega bytes transfered from/to main storage per second.
void io_data_diff(io_t *io2, io_t *io1, io_t *io_diff, double *mbs);

// Helpers
void io_data_alloc(io_t **io);

void io_data_free(io_t **io);

void io_data_copy(io_t *io2, io_t *io1);

void io_data_print(io_t *io_diff, double mbs, int fd);

char *io_data_tostr(io_t *io_diff, double mbs, char *buffer, size_t length);

// clang-format on
#endif // METRICS_IO_H