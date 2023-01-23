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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef METRICS_COMMON_LIKWID_H
#define METRICS_COMMON_LIKWID_H

#include <common/types.h>
#include <common/states.h>

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
