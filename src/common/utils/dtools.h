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

// Given an address and a breakpoint in dtools_break() function, when the
// address or a near address is detected in malloc or free, dtools_break() is
// called, prompting the breakpoint in GDB.

// Set the address to control by dtools.
void dtools_set_address(void *address);

char *dtools_get_backtrace_library(char *buffer, int calls_count);

int dtools_is_ldd_library(char *library);

#endif
