/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IMCFREQ_H
#define METRICS_IMCFREQ_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/types.h>
#include <metrics/common/apis.h>
#include <metrics/common/pstate.h>

// The API
//
// This API is designed to get the uncore or integrated memory controllers
// frequency.
//
// Props:
// 	- Thread safe: yes.
//	- Daemon API: yes.
//  - Dummy API: yes.
//  - Requires root: yes.
//
// Compatibility:
//  -------------------------------------------------------------------------
//  | Architecture    | F/M | Comp. | Granularity | System                  |
//  -------------------------------------------------------------------------
//  | Intel HASWELL   | 63  | v     | Socket/node | MSR                     |
//  | Intel BROADWELL | 79  | v     | Socket/node | MSR                     |
//  | Intel SKYLAKE   | 85  | v     | Socket/node | MSR                     |
//  | Intel ICELAKE   | 106 | ?     | ?           | -                       |
//  | AMD ZEN+/2      | 17h | v     | Socket/node | MGT IMCFREQ bypass      |
//  | AMD ZEN3        | 19h | v     | Socket/node | MGT IMCFREQ bypass      |
//  -------------------------------------------------------------------------

typedef struct imcfreq_s {
    timestamp_t time;
    ulong freq; // KHz
    uint error;
} imcfreq_t;

typedef struct imcfreq_ops_s {
    void (*get_api)(uint *api, uint *api_intern);
    state_t (*init)(ctx_t *c);
    state_t (*init_static[4])(ctx_t *c);
    state_t (*dispose)(ctx_t *c);
    state_t (*count_devices)(ctx_t *c, uint *dev_count);
    state_t (*read)(ctx_t *c, imcfreq_t *reg_list);
    state_t (*data_alloc)(imcfreq_t **reg_list, ulong **freq_list);
    state_t (*data_free)(imcfreq_t **reg_list, ulong **freq_list);
    state_t (*data_copy)(imcfreq_t *reg_list2, imcfreq_t *reg_list1);
    state_t (*data_diff)(imcfreq_t *reg_list2, imcfreq_t *reg_list, ulong *freq_list, ulong *freq_avg);
    void (*data_print)(ulong *freq_list, ulong *freq_avg, int fd);
    char *(*data_tostr)(ulong *freq_list, ulong *freq_avg, char *buffer, size_t length);
} imcfreq_ops_t;

// Frequency is KHz
void imcfreq_load(topology_t *tp, int force_api);

void imcfreq_get_api(uint *api);

state_t imcfreq_init(ctx_t *c);

state_t imcfreq_dispose(ctx_t *c);

state_t imcfreq_count_devices(ctx_t *c, uint *dev_count);

state_t imcfreq_read(ctx_t *c, imcfreq_t *reg_list);

state_t imcfreq_read_diff(ctx_t *c, imcfreq_t *reg_list2, imcfreq_t *reg_list1, ulong *freq_list, ulong *freq_avg);

state_t imcfreq_read_copy(ctx_t *c, imcfreq_t *reg_list2, imcfreq_t *reg_list1, ulong *freq_list, ulong *freq_avg);

// Helpers
state_t imcfreq_data_alloc(imcfreq_t **reg_list, ulong **freq_list);

state_t imcfreq_data_free(imcfreq_t **reg_list, ulong **freq_list);

state_t imcfreq_data_copy(imcfreq_t *reg_list2, imcfreq_t *reg_list1);

state_t imcfreq_data_diff(imcfreq_t *reg_list2, imcfreq_t *reg_list1, ulong *freq_list, ulong *freq_avg);

void imcfreq_data_print(ulong *freq_list, ulong *freq_avg, int fd);

char *imcfreq_data_tostr(ulong *freq_list, ulong *freq_avg, char *buffer, size_t length);

#endif // METRICS_IMCFREQ_H
