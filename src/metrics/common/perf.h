/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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

/* For per-process events */
state_t perf_open(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event);
// Perf open extended/extra. With more options. Using cpu > 0 will imply system wide configuration
state_t perf_open_cpu(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event, uint options, int cpu);

state_t perf_reset(perf_t *perf);

state_t perf_start(perf_t *perf);

state_t perf_stop(perf_t *perf);

state_t perf_close(perf_t *perf);

state_t perf_read(perf_t *perf, llong *value);

#endif //METRICS_COMMON_PERF_H
