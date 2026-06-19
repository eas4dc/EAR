/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_DTOOLS_H
#define COMMON_UTILS_DTOOLS_H
/* clang-format off */

// Given an address and by settings a GDB breakpoint in dtools_break() function,
// when the address or a near address is detected in free(), then dtools_break()
// is called, prompting the GDB breakpoint where you can inspect traces and
// variables. You have to define DTOOLS_FREE in compile time.
void dtools_set_address(void *address);

// In returns a list of the function backtrace called before reaching the
// current function.
char *dtools_get_backtrace_library(char *buffer, int calls_count);

// Returns 1 if a library is detected in the list of loaded shared objects.
int dtools_is_ldd_library(char *library);

/* clang-format on */
#endif
