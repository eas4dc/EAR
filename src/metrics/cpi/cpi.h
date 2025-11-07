/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_CPI_H
#define METRICS_CPI_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/system/time.h>

typedef struct stalls_s {
    ullong fetch_decode; // Instruction fetch-decode pipeline
    ullong resources;    // Reservation Station, PORTs, Physical Registers, load/store buffers...
    ullong memory;       // Cache miss waits
} stalls_t;

typedef struct cpi_s {
    ullong instructions;
    ullong cycles;
    stalls_t stalls;
} cpi_t;

typedef struct cpi_ops_s {
    void (*get_info)(apinfo_t *info);
    state_t (*init)();
    state_t (*dispose)();
    state_t (*read)(cpi_t *ci);
} cpi_ops_t;

// API building scheme
#define CPI_F_LOAD(name)     void name(topology_t *tpo, cpi_ops_t *ops)
#define CPI_F_GET_INFO(name) void name(apinfo_t *info)
#define CPI_F_INIT(name)     state_t name()
#define CPI_F_DISPOSE(name)  state_t name()
#define CPI_F_READ(name)     state_t name(cpi_t *cpi)

#define CPI_DEFINES(name)                                                                                              \
    CPI_F_LOAD(cpi_##name##_load);                                                                                     \
    CPI_F_GET_INFO(cpi_##name##_get_info);                                                                             \
    CPI_F_INIT(cpi_##name##_init);                                                                                     \
    CPI_F_DISPOSE(cpi_##name##_dispose);                                                                               \
    CPI_F_READ(cpi_##name##_read);

void cpi_load(topology_t *tp, int force_api);

void cpi_get_api(uint *api);

void cpi_get_info(apinfo_t *info);

state_t cpi_init(ctx_t *c);

state_t cpi_dispose(ctx_t *c);

state_t cpi_read(ctx_t *c, cpi_t *ci);

// Helpers
state_t cpi_read_diff(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

state_t cpi_read_copy(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

void cpi_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

void cpi_data_copy(cpi_t *src, cpi_t *dst);

void cpi_data_print(cpi_t *ciD, double cpi, int fd);

void cpi_data_tostr(cpi_t *ciD, double cpi, char *buffer, size_t length);

#endif // METRICS_CPI_H