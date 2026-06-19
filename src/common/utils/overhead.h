/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_OVERHEAD_H
#define COMMON_UTILS_OVERHEAD_H
/* clang-format off */

#include <common/types.h>

#define ENABLE_OVERHEAD 0

// Subscribes a section of code whose execution time will be measured. It
// receives a name or description of that code section, and returns a ID which
// will be used to circle the section by start-stop calls. Later, you can call
// report to print the measuring metrics.
//
// The EAR_OVERHEAD_ENABLE environment variable enables the subscribed systems
// in a list of comma separated names or the keyword 'all'.
//
// Example:
//  EAR_OVERHEAD_ENABLE="get_freq"
//  overhead_subscribe("get_freq", &id_get_freq);
//  overhead_subscribe("set_freq", &id_set_freq);
//  overhead_subscribe("reset_freq", &id_reset_freq);
//  overhead_start(id_freq);
//  ... (get frequency code)
//  overhead_stop(id_freq);
//  ... (other things)
//  overhead_report(1);

// The name or description string will appear in the report.
void overhead_subscribe(const char *name_desc, uint *id);

void overhead_subsprint(uint *id, const char *fmt, ...);

void overhead_start(uint id);

void overhead_stop(uint id);

void overhead_report(int print_header);

void overhead_print_header();

/* clang-format on */
#endif // COMMON_UTILS_OVERHEAD_H