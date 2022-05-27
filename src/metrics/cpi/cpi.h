/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

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

state_t cpi_load(topology_t *tp, int eard);

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