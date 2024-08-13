/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
