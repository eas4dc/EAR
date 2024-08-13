/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_STRESS_H
#define COMMON_UTILS_STRESS_H

#include <common/system/time.h>

void stress_alloc();

void stress_free();
// Stress the system bandwidth by memcpy during a period in miliseconds.
void stress_bandwidth(ullong ms);
// Stress the system through while-spinning during a period in miliseconds.
void stress_spin(ullong ms);

#endif