/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_CPI_PERF_H
#define METRICS_CPI_PERF_H

#include <metrics/cpi/cpi.h>

state_t cpi_perf_load(topology_t *tp, cpi_ops_t *ops);

state_t cpi_perf_init(ctx_t *c);

state_t cpi_perf_dispose(ctx_t *c);

state_t cpi_perf_read(ctx_t *c, cpi_t *ci);

#endif //METRICS_CPI_PERF_H
