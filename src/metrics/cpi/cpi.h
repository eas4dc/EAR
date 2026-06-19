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
// clang-format off

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

typedef struct stalls_s {
    ullong fetch_decode; // Instruction fetch-decode pipeline
    ullong resources;    // Reservation Station, PORTs, Physical Registers, load/store buffers...
    ullong memory;       // Cache miss waits
} stalls_t;

typedef struct cpi_s {
    pid_t    pid;
    ullong   instructions;
    ullong   cycles;
    stalls_t stalls;
    double cpi; // (cycles / instructions) or average cycle consumed per instruction
} cpi_t;

typedef struct cpi_ops_s {
    void    (*unload)     ();
    state_t (*update)     (uint option, void *value);
    void    (*get_info)   (apinfo_t *info);
    state_t (*read)       (cpi_t *cpi);
} cpi_ops_t;

// API building scheme
#define CPI_F_LOAD(name)       void    cpi_##name##_load(topology_t *tp, cpi_ops_t *ops, int options)
#define CPI_F_UNLOAD(name)     void    cpi_##name##_unload()
#define CPI_F_UPDATE(name)     state_t cpi_##name##_update(uint option, void *value)
#define CPI_F_GET_INFO(name)   void    cpi_##name##_get_info(apinfo_t *info)
#define CPI_F_READ(name)       state_t cpi_##name##_read(cpi_t *cpi)

#define CPI_DEFINES(name)    \
    CPI_F_LOAD(name);        \
    CPI_F_UNLOAD(name);      \
    CPI_F_UPDATE(name);      \
    CPI_F_GET_INFO(name);    \
    CPI_F_READ(name);

// Primer problema, necesitamos el debug en el load
void cpi_load(topology_t *tp, int options);

void cpi_unload();

state_t cpi_update(uint option, void *value);

void cpi_get_info(apinfo_t *info);

state_t cpi_read(cpi_t *ci);

state_t cpi_read_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpis_avg);

state_t cpi_read_copy(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpis_avg);

void cpi_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpis_avg);

void cpi_data_alloc(cpi_t **ci);

void cpi_data_free(cpi_t **ci);

void cpi_data_copy(cpi_t *src, cpi_t *dst);

void cpi_data_print(cpi_t *ciD, double cpis_avg, int fd);

char *cpi_data_tostr(cpi_t *ciD, double cpis_avg, char *buffer, size_t length);

// clang-format on
#endif // METRICS_CPI_H