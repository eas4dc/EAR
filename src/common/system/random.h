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

#include <common/types/generic.h>

/* Gets a 32 bits random number, it uses hardware functions which
 * spends up to 300 cycles, so use it carefully. */
uint random_get();

/* Gets a 32 bits random number between min and min + offset, it uses
 * random_get() internally, so use it carefully. */
uint random_getrank(uint min, uint offset);

#endif //EAR_COMMON_TIME_H
