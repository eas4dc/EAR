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
#include <common/output/error.h>

/* error definitions */
#define EAR_SUCCESS              0
#define EAR_ERROR               -1
#define EAR_WARNING             -2
#define EAR_BAD_ARGUMENT		-13
#define EAR_INITIALIZED			-7
#define EAR_NOT_INITIALIZED		-8
#define EAR_UNDEFINED			-24
#define EAR_TIMEOUT				-21
#define EAR_SYSCALL_ERROR		-18
#define EAR_UNAVAILABLE			-10

/* type & functions */
typedef int state_t;

/* global data */
char *state_msg;

#define state_ok(state) \
	state == EAR_SUCCESS

#define state_fail(state) \
	state != EAR_SUCCESS

#define state_is(state1, state2) \
	state1 == state2

#define return_msg(no, msg) { \
	state_msg = msg; \
	return no; \
	}

#define xtate_fail(s, function) \
	(s = function) != EAR_SUCCESS

#define state_assert(s, func, cons) \
    if (xtate_fail(s, func)) { \
        error(#func " returned %d (%s)\n", s, state_msg); \
        cons; \
    }

struct generr_s {
	char *api_undefined;
	char *api_incompatible;
	char *api_uninitialized;
	char *alloc_error;
	char *input_null;
	char *input_uninitialized;
	char *lock;
} Generr __attribute__((weak)) = {
	.api_undefined = "the API is undefined",
	.api_incompatible = "the current hardware is not supported by the API",
	.api_uninitialized = "the API is not initialized",
	.alloc_error = "error ocurred during allocation",
	.input_null = "an argument of the input is NULL",
	.input_uninitialized = "an argument is not initialized",
	.lock = "error while using mutex_lock",
};

/*
 *
 * Legacy
 *
 */

/* legacy errors */
#define EAR_IGNORE              -16		//*
#define EAR_ALLOC_ERROR         -3		//*
#define EAR_READ_ERROR          -4
#define EAR_OPEN_ERROR			-5
#define EAR_WRITE_ERROR			-6		//*
#define EAR_NOT_READY		    -9
#define EAR_BUSY				-10
#define EAR_ALREADY_CLOSED		-11		//*
#define EAR_ARCH_NOT_SUPPORTED	-12		//*
#define EAR_MYSQL_ERROR         -14
#define EAR_MYSQL_STMT_ERROR    -15
#define EAR_ADDR_NOT_FOUND		-17		//*
#define EAR_SOCK_OP_ERROR		-18
#define EAR_SOCK_BAD_PROTOCOL	-19		//*
#define EAR_SOCK_DISCONNECTED	-20
#define EAR_NO_RESOURCES		-22		//*
#define EAR_NOT_FOUND			-23		//*
#define EAR_DL_ERROR			-25		//*
#define EAR_INCOMPATIBLE		-26		//*

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
	intern_error_num = error_num; \
	intern_error_str = error_str; \
	return state;

#endif //STATES_H
