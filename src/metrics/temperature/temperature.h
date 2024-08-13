/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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

void temp_get_api(uint *api);

state_t temp_init(ctx_t *c);

state_t temp_dispose(ctx_t *c);
/** It returns the number of devices (normally sockets). */
state_t temp_count_devices(ctx_t *c, uint *devs_count);
/** Reads the last temperature value and the average per device. */
state_t temp_read(ctx_t *c, llong *temp_list, llong *average);

state_t temp_read_copy(ctx_t *c, llong *t2, llong *t1, llong *tD, llong *average);

// Data
state_t temp_data_alloc(llong **temp_list);

state_t temp_data_copy(llong *tempD, llong *tempS);

void temp_data_diff(llong *temp2, llong *temp1, llong *tempD, llong *average);

state_t temp_data_free(llong **temp_list);

void temp_data_print(llong *list, llong avrg, int fd);

char *temp_data_tostr(llong *list, llong avrg, char *buffer, int length);

#endif