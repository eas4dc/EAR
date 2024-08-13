/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_APIS_H
#define METRICS_COMMON_APIS_H

#include <common/types/generic.h>

#define no_ctx                 NULL
// Load EARD API or not
#define EARD                   API_EARD
#define NO_EARD                API_FREE
#define all_devs              -1
#define all_cpus               all_devs
#define all_cores             -2
#define API_NONE               0
#define API_FREE               API_NONE
#define API_DUMMY              1
#define API_EARD               2
#define API_BYPASS             3
#define API_DEFAULT            4
#define API_INTEL63            5
#define API_AMD17              6
#define API_NVML               7
#define API_PERF               8
#define API_INTEL106           9
#define API_LIKWID            10
#define API_CUPTI             11
#define API_ONEAPI            12
#define API_RAPL              API_INTEL63
#define API_ISST              14
#define API_FAKE              15
#define API_CPUMODEL          16
#define API_RSMI              17
#define API_AMD19             18
#define API_INTEL143          19
#define API_LINUX_POWERCAP    20
#define API_DCGMI             21
#define GRANULARITY_NONE       0
#define GRANULARITY_DUMMY      1
#define GRANULARITY_PROCESS    2
#define GRANULARITY_THREAD     3
#define GRANULARITY_CORE       4
#define GRANULARITY_PERIPHERAL 5
#define GRANULARITY_L3_SLICE   7
#define GRANULARITY_IMC        8
#define GRANULARITY_SOCKET     9
#define SCOPE_NONE             0
#define SCOPE_DUMMY            1
#define SCOPE_PROCESS          2
#define SCOPE_NODE             3
#define MONITORING_MODE_STOP  -1
#define MONITORING_MODE_IDLE   0
#define MONITORING_MODE_RUN    1

#define API_IS(flag, api) \
    (flag == api)

typedef struct ctx_s {
    void *context;
} ctx_t;

typedef struct apinfo_s {
    char *layer;
    uint  api;
    uint  api_under;
    uint  ok;
    char *api_str;
    uint  devs_count;
    uint  granularity;
    char *granularity_str;
    uint  scope;
    char *scope_str;
    uint  bits;
    uint  dev_model;
    void *list1;
    void *list2;
    void *list3;
    uint  list1_count;
    uint  list2_count;
    uint  list3_count;
    void *avail_list;
    uint  avail_count;
    void *current_list;
    void *set_list;
} apinfo_t;

#define metrics_t apinfo_t

#if USE_GPUS
// Below type was created to mantain compatibility with the current design of library/metrics and new GPU APIs
typedef struct metrics_gpus_s {
    uint  api;
    uint  ok;
    uint  devs_count;
    void **avail_list;
    uint *avail_count;
    void *current_list;
} metrics_gpus_t;
#endif

/* Replaces the function of the operation if its NULL. */
#define apis_put(op, func) \
    if (op == NULL) { \
        *((void **) &op) = func; \
    }

/* Conditional put. */
#define apis_pin(op, func, cond) \
    if (cond) { \
        apis_put(op, func); \
    }

/* Replaces the currect function of the operation. */
#define apis_set(op, func) *((void **) &op) = func;

/* Verbose "if loaded" */
#define apis_loaded(ops) (ops->init != NULL)

/* Verbose "if not loaded" */
#define apis_not(ops) (ops->init == NULL)

/* Adds a new function to an array. */
#define apis_add(ops, func) \
	apis_append((void **) ops, func);

/* Provisional multicall function */
#define apis_multi(ops, ...) \
    state_t s; \
    int i = 0; \
    while (ops [i] != NULL) { \
        if (state_fail(s = ops [i](__VA_ARGS__))) { \
            debug("failed " #ops "[%d]: %s", i, state_msg); \
            return s; \
        } \
        ++i; \
    }

#define apis_multiret(ops, ...) \
    apis_multi(ops, __VA_ARGS__); \
    return s;

// Rename apis to api.
void apis_append(void *op[], void *func);

void apis_print(uint api, char *prefix);

void apis_tostr(uint api, char *buffer, size_t size);

void apinfo_tostr(apinfo_t *info);

/* Obsolete, remove it. */
#define replace_ops(p1, p2) \
	if (p1 == NULL) { \
		p1 = p2; \
	}

#endif
