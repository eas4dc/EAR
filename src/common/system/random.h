/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_COMMON_RANDOM_H
#define EAR_COMMON_RANDOM_H

#include <stdint.h>

uint64_t random_getrank64(uint64_t max, uint64_t min);

#endif // EAR_COMMON_RANDOM_H
