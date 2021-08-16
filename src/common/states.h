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

#ifndef STATES_H
#define STATES_H

#include <errno.h>
#include <stdio.h>
#include <common/sizes.h>
#include <common/utils/string.h>
#include <common/output/verbose.h>

/* error definitions */
#define EAR_SUCCESS              0
#define EAR_ERROR               -1
#define EAR_WARNING             -2
#define EAR_NO_PERMISSIONS		-6
#define EAR_INITIALIZED			-7
#define EAR_NOT_INITIALIZED		-8
#define EAR_BAD_ARGUMENT		-13
#define EAR_SYSCALL_ERROR		-18
#define EAR_TIMEOUT				-21
#define EAR_UNDEFINED			-24

/* error buffer */
char state_buffer[SZ_BUFFER];

/* type & functions */
typedef int state_t;

/* global data */
char *state_msg;

#define state_ok(state) \
	((state) == EAR_SUCCESS)

#define state_fail(state) \
	((state) != EAR_SUCCESS)

#define state_is(state1, state2) \
	(state1 == state2)
	
#define return_msg(no, msg) { \
	state_msg = msg; \
	return no; \
	}

#define return_print(no, ...) { \
	state_msg = state_buffer; \
	xsnprintf(state_buffer, sizeof(state_buffer), __VA_ARGS__); \
	return no; \
	}

// error(#func " returned %d (%s)", s, state_msg);

#define state_assert(s, func, cons) \
    if (xtate_fail(s, func)) { \
        error("returned %d, %s (%s:%d)", s, state_msg, __FILE__, __LINE__); \
        cons; \
    }

struct generr_s {
	char *api_undefined;
	char *api_incompatible;
	char *api_uninitialized;
	char *api_initialized;
	char *alloc_error;
	char *input_null;
	char *input_uninitialized;
	char *lock;
	char *context_null;
	char *arg_outbounds;
	char *cpu_invalid;
	char *no_permissions;
	char *not_found;
} Generr __attribute__((weak)) = {
	.api_undefined = "the API is undefined",
	.api_incompatible = "current hardware is not supported by the API",
	.api_uninitialized = "the API is not initialized",
	.api_initialized = "the API is already initialized",
	.alloc_error = "error ocurred during allocation",
	.input_null = "an argument of the input is NULL",
	.input_uninitialized = "an argument is not initialized",
	.lock = "error while using mutex_lock",
	.context_null = "context can not be null",
	.arg_outbounds = "argument out of bounds",
	.cpu_invalid = "invalid CPU",
	.no_permissions = "not enough privileges to perform this action.",
	.not_found = "Not found",
};

/*
 *
 * Legacy
 *
 */

/* legacy errors */
#define EAR_IGNORE              -16 //*
#define EAR_ALLOC_ERROR         -3  //*
#define EAR_READ_ERROR          -4
#define EAR_OPEN_ERROR          -5
#define EAR_NOT_READY           -9
#define EAR_BUSY                -10
#define EAR_MYSQL_ERROR         -14
#define EAR_MYSQL_STMT_ERROR    -15
#define EAR_ADDR_NOT_FOUND      -17 //*
#define EAR_SOCK_OP_ERROR       -18
#define EAR_SOCK_DISCONNECTED   -20
#define EAR_NO_RESOURCES        -22 //*
#define EAR_NOT_FOUND           -23 //*

// TODO: this is a config not a state
#define DYNAIS_ENABLED      1
#define DYNAIS_DISABLED     0
#define PERIODIC_MODE_ON    1
#define PERIODIC_MODE_OFF   0

#define intern_error_str state_msg
int intern_error_num;

#define state_return(state) \
	return state;

#define state_return_msg(state, error_num, error_str) \
{ \
	intern_error_num = error_num; \
	intern_error_str = error_str; \
	return state;\
}

#define xtate_fail(s, function) \
	((s = function) != EAR_SUCCESS)

#define xtate_ok(s, function) \
	((s = function) == EAR_SUCCESS)

#endif //STATES_H
