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

#ifndef METRICS_TEMPERATURE_H
#define METRICS_TEMPERATURE_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

// The API
//
// This API returns a temperature in Celsius degrees per socket.
//
// Props:
//	- Thread safe: yes.
//	- User mode: just in amd17.
//	- Type: direct value.
//
// Folders:
//  - archs: different node architectures, such as AMD and Intel.
//  - tests: examples.
//
// Future work:
//
// Use example:
//  - You can find an example in cpufreq/tests folder.

state_t temp_load(topology_t *tp);

state_t temp_init(ctx_t *c);

state_t temp_dispose(ctx_t *c);

/** It returns the number of sockets. */
state_t temp_count_devices(ctx_t *c, uint *count);

// Data
state_t temp_data_alloc(ctx_t *c, llong **temp_list, uint *temp_count);

/* Copies temp1 in temp2. */
state_t temp_data_copy(ctx_t *c, llong *temp_list2, llong *temp_list1);

state_t temp_data_free(ctx_t *c, llong **temp_list);

// Getter
/** Requires a llong array of a length of total node sockets. */
state_t temp_read(ctx_t *c, llong *temp_list, llong *average);

#endif
