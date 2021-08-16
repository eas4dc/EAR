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

#ifndef METRICS_COMMON_APIS_H
#define METRICS_COMMON_APIS_H

#include <common/types/generic.h>

// APIs IDs
#define API_NONE         0
#define API_DUMMY        1
#define API_EARD         2
#define API_BYPASS       3
#define API_OS_CPUFREQ   4
#define API_INTEL63      5
#define API_AMD17        6
#define API_NVML         7
// Load EARD API or not
#define EARD             1
#define NO_EARD          0

#define replace_ops(p1, p2) \
	if (p1 == NULL) { \
		p1 = p2; \
	}

/* Replaces the function of the operation if its NULL. */
#define apis_put(op, func) \
    if (op == NULL) { \
        op = func; \
    }

/* Replaces the currect function of the operation. */
#define apis_set(op, func) op = func;

/* */
#define apis_loaded(ops) (ops->init != NULL)

/* */
#define apis_not(ops) (ops->init == NULL)

/* Adds a new function to an array. */
#define apis_add(op, func) \
	apis_append((void **) op, func);

void apis_append(void **op, void *func);

void apis_print(uint api, char *prefix);

void apis_tostr(uint api, char *buff,size_t size);

#endif
