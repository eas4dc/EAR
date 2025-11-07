/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_LIKWID_H
#define METRICS_COMMON_LIKWID_H

#include <common/states.h>
#include <common/types.h>

typedef struct likevs_s {
    uint evs_count;
    int gid;
} likevs_t;

state_t likwid_init();

state_t likwid_dispose();
/* Given a list of event names, it returns a LIKWID event struct and allocations for counters. */
state_t likwid_events_open(likevs_t *id, double **ctrs_alloc, uint *ctrs_count, char *evs_names, uint evs_count);

state_t likwid_events_read(likevs_t *id, double *ctrs);

state_t likwid_events_close(likevs_t *id, double **ctrs);

#endif
