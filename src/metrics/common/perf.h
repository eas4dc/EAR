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

#ifndef METRICS_COMMON_PERF_H
#define METRICS_COMMON_PERF_H

#include <common/types.h>
#include <common/states.h>
#include <linux/perf_event.h>

// Options (combine with &)
#define pf_exc	0x0001
#define pf_pin	0x0002		 

typedef struct perf_s {
	struct perf_event_attr attr;
	void *group;
	int fd;
} perf_t;

state_t perf_open(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event);

state_t perf_opex(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event, uint options);

state_t perf_reset(perf_t *perf);

state_t perf_start(perf_t *perf);

state_t perf_stop(perf_t *perf);

state_t perf_close(perf_t *perf);

state_t perf_read(perf_t *perf, llong *value);

#endif //METRICS_COMMON_PERF_H
