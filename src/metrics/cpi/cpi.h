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

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>

// Compatibility:
//  ----------------------------------------------------------------------------
//  | Architecture    | F/M | Comp. | Granularity | Comments                   |
//  ----------------------------------------------------------------------------
//  | Intel HASWELL   | 63  | v     | Process     |                            |
//  | Intel BROADWELL | 79  | v     | Process     |                            |
//  | Intel SKYLAKE   | 85  | v     | Process     |                            |
//  | AMD ZEN+/2      | 17h | v     | Process     |                            |
//  | AMD ZEN3        | 19h | v     | Process     |                            |
//  ---------------------------------------------------------------------------|
// Props:
//  - Thread safe: yes
//  - Requires root: no

typedef struct cpi_s {
	ullong instructions;
	ullong cycles;
	ullong stalls;
} cpi_t;

typedef struct cpi_ops_s {
	state_t (*init)            (ctx_t *c);
	state_t (*dispose)         (ctx_t *c);
	state_t (*read)            (ctx_t *c, cpi_t *ci);
	state_t (*data_diff)       (cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);
	state_t (*data_copy)       (cpi_t *dst, cpi_t *src);
	state_t (*data_print)      (cpi_t *ci, double cpi, int fd);
	state_t (*data_tostr)      (cpi_t *ci, double cpi, char *buffer, size_t length);
} cpi_ops_t;

state_t cpi_load(topology_t *tp, int force_api);

state_t cpi_get_api(uint *api);

state_t cpi_init(ctx_t *c);

state_t cpi_dispose(ctx_t *c);

state_t cpi_read(ctx_t *c, cpi_t *ci);

// Helpers
state_t cpi_read_diff(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

state_t cpi_read_copy(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

state_t cpi_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

state_t cpi_data_copy(cpi_t *src, cpi_t *dst);

state_t cpi_data_print(cpi_t *ciD, double cpi, int fd);

state_t cpi_data_tostr(cpi_t *ciD, double cpi, char *buffer, size_t length);

#endif //METRICS_CPI_H