/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef STATES_H
#define STATES_H

#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/utils/string.h>
#include <errno.h>
#include <stdio.h>

// Error definitions. ALWAYS use EAR_ERROR instead more specific
// definitions. Use specific errors ONLY when you are controlling
// differently from outside.
#define EAR_SUCCESS         0
#define EAR_ERROR           -1
#define EAR_WARNING         -2
#define EAR_NOT_INITIALIZED -8
#define EAR_BAD_ARGUMENT    -13
#define EAR_TIMEOUT         -21
#define EAR_UNDEFINED       -24
#define EAR_CANCELLED       -25

/* error buffer */
char state_buffer[SZ_BUFFER] __attribute__((weak));
char ______buffer[SZ_BUFFER] __attribute__((weak));

/* type & functions */
typedef int state_t;

/* global data */
char *state_msg __attribute__((weak));
int state_no __attribute__((weak));

#define state_ok(state)          ((state) == EAR_SUCCESS)

#define state_fail(state)        ((state) != EAR_SUCCESS)

#define state_is(state1, state2) (state1 == state2)

#define serror(str)              error("%s: %s (%d)", str, state_msg, state_no);

#define sserror(format, ...)     error(format ": %s (%d)", __VA_ARGS__, state_msg, state_no);

#define return_msg(no, msg)                                                                                            \
    {                                                                                                                  \
        state_msg = msg;                                                                                               \
        return no;                                                                                                     \
    }

#define return_print(no, ...)                                                                                          \
    {                                                                                                                  \
        state_msg = state_buffer;                                                                                      \
        xsnprintf(state_buffer, sizeof(state_buffer), __VA_ARGS__);                                                    \
        return no;                                                                                                     \
    }

#define return_reprint(no, ...)                                                                                        \
    {                                                                                                                  \
        state_msg = strncpy(______buffer, state_msg, SZ_BUFFER);                                                       \
        xsnprintf(state_buffer, sizeof(state_buffer), __VA_ARGS__);                                                    \
        state_msg = state_buffer;                                                                                      \
        return no;                                                                                                     \
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
    char *gpus_not;
    char *dlopen;
} Generr __attribute__((weak)) = {
    .api_undefined       = "The API is undefined",
    .api_incompatible    = "Current hardware is not supported by the API",
    .api_uninitialized   = "The API is not initialized",
    .api_initialized     = "The API is already initialized",
    .alloc_error         = "Error ocurred during allocation",
    .input_null          = "An argument of the input is NULL",
    .input_uninitialized = "An argument is not initialized",
    .lock                = "Error while using mutex_lock",
    .context_null        = "Context can not be null",
    .arg_outbounds       = "Argument out of bounds",
    .cpu_invalid         = "Invalid CPU",
    .no_permissions      = "Not enough privileges to perform this action",
    .not_found           = "Not found",
    .gpus_not            = "No GPUs detected",
    .dlopen              = "Error during dlopen",
};

/*
 *
 * Legacy
 *
 */

/* legacy errors */
#define EAR_IGNORE            -16 //*
#define EAR_ALLOC_ERROR       -3  //*
#define EAR_READ_ERROR        -4
#define EAR_OPEN_ERROR        -5
#define EAR_NOT_READY         -9
#define EAR_BUSY              -10
#define EAR_MYSQL_ERROR       -14
#define EAR_MYSQL_STMT_ERROR  -15
#define EAR_SOCK_OP_ERROR     -18
#define EAR_SOCK_DISCONNECTED -20
#define EAR_NO_RESOURCES      -22 //*
#define EAR_NOT_FOUND         -23 //*

// TODO: this is a config not a state
#define DYNAIS_ENABLED    1
#define DYNAIS_DISABLED   0
#define PERIODIC_MODE_ON  1
#define PERIODIC_MODE_OFF 0

#define intern_error_str  state_msg
int intern_error_num __attribute__((weak));

#define state_return(state) return state;

#define state_return_msg(state, error_num, error_str)                                                                  \
    {                                                                                                                  \
        intern_error_num = error_num;                                                                                  \
        intern_error_str = error_str;                                                                                  \
        return state;                                                                                                  \
    }

#define xtate_fail(s, function) ((s = function) != EAR_SUCCESS)

#define sassert(function, consecuence)                                                                                 \
    if (state_fail(function)) {                                                                                        \
        consecuence;                                                                                                   \
    }

#define state_assert(s, func, cons)                                                                                    \
    if (state_fail(s = func)) {                                                                                        \
        cons;                                                                                                          \
    }

#endif // STATES_H
