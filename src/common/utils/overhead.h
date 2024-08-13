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

#include <common/types.h>

#define ENABLE_OVERHEAD 0

// Suscribes a section of code. It receives a description of that code section,
// and returns a ID which will be used to circle the section by start-stop
// marks. Later, you can all report to print all the gathered data.
//
// Ex:
//  overhead_suscribe("get frequency", &id_freq);
//  overhead_start(id_freq);
//  ... (get frequency code)
//  overhead_stop(id_freq);
//  ... (things)
//  overhead_report(1);

void overhead_suscribe(const char *description, uint *id);

void overhead_start(uint id);

void overhead_stop(uint id);

void overhead_report(int print_header);

void overhead_print_header();

#endif //COMMON_UTILS_OVERHEAD_H
