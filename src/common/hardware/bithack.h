/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_BITHACK
#define METRICS_COMMON_BITHACK

#include <common/types/generic.h>

ullong setbits64(ullong reg, ullong val, uint left_bit, uint right_bit);

ullong getbits64(ullong reg, uint left_bit, uint right_bit);

uint setbits32(uint reg, uint val, uint left_bit, uint right_bit);

uint getbits32(uint reg, uint left_bit, uint right_bit);

#endif //METRICS_COMMON_BITHACK