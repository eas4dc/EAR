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

#ifndef METRICS_COMMON_APIS_H
#define METRICS_COMMON_APIS_H

#include <common/types/generic.h>

// Devices
#define all_devs   -1
#define all_cpus   all_devs
#define all_cores  -2
// APIs IDs
#define API_NONE         0
#define API_DUMMY        1
#define API_EARD         2
#define API_BYPASS       3
#define API_DEFAULT      4
#define API_INTEL63      5
#define API_AMD17        6
#define API_NVML         7
#define API_PERF         8
#define API_INTEL106     9
#define API_LIKWID      10
#define API_CUPTI       11
#define API_ONEAPI      12
#define API_RAPL 		13
#define API_ISST        14
#define API_FAKE        15
#define API_CPUMODEL	  16
// Load EARD API or not
#define EARD             1
#define NO_EARD          0
#define no_ctx           NULL
//
#define GRANULARITY_NONE       0
#define GRANULARITY_DUMMY      1
#define GRANULARITY_PROCESS    2
#define GRANULARITY_THREAD     3
#define GRANULARITY_CORE       4
#define GRANULARITY_CCX        5
#define GRANULARITY_CCD        6
#define GRANULARITY_L3_SLICE   7
#define GRANULARITY_IMC        8
#define GRANULARITY_SOCKET     9
#define GRANULARITY_NODE       10

typedef struct ctx_s {
    void *context;
} ctx_t;

typedef struct metrics_s {
    ctx_t c;
    uint  api;
    uint  ok;
    uint  granularity;
    uint  devs_count;
    void *avail_list;
    uint  avail_count;
    void *current_list;
    void *set_list;
} metrics_t;

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

typedef struct metrics_multi{
    uint  api;
    uint  ok;
    uint  dev_count;
    uint  *dev_list;
    void *avail_list;
    uint avail_count;
    void *current_list;
}metrics_multi_t;

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

void apis_append(void *op[], void *func);

void apis_print(uint api, char *prefix);

void apis_tostr(uint api, char *buffer, size_t size);

void granularity_print(uint granularity, char *prefix);

void granularity_tostr(uint granularity, char *buffer, size_t size);

/* Obsolete, remove it. */
#define replace_ops(p1, p2) \
	if (p1 == NULL) { \
		p1 = p2; \
	}

#endif
