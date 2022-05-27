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

#ifndef METRICS_CPI_DUMMY_H
#define METRICS_CPI_DUMMY_H

#include <metrics/cpi/cpi.h>

state_t cpi_dummy_load(topology_t *tp, cpi_ops_t *ops);

state_t cpi_dummy_init(ctx_t *c);

state_t cpi_dummy_dispose(ctx_t *c);

state_t cpi_dummy_read(ctx_t *c, cpi_t *ci);

// Helpers
state_t cpi_dummy_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi);

state_t cpi_dummy_data_copy(cpi_t *src, cpi_t *dst);

state_t cpi_dummy_data_print(cpi_t *b, double cpi, int fd);

state_t cpi_dummy_data_tostr(cpi_t *b, double cpi, char *buffer, size_t length);

#endif //METRICS_CPI_DUMMY_H
