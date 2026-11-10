/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
#ifndef METRICS_COMMON_SCRIPT_H
#define METRICS_COMMON_SCRIPT_H

/* Returns 1 if path names a regular file with execute permission, or 0 otherwise. */
int script_is_executable(const char *path);

/* Executes a script or binary and its parameters in the form of a printf.
 * Returns 1 when the script exits successfully, or 0 otherwise. Sets the errno. */
int script_exec(const char *path, int wait, int quiet, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 4, 5)))
#endif
    ;

#endif // METRICS_COMMON_SCRIPT_H
