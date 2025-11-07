/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_OFFSET_H
#define METRICS_COMMON_OFFSET_H

#include <common/types.h>

typedef struct offset_op_s {
    long offset_result;
    long offset_oprnd1;
    long offset_oprnd2;
    char operation;
    char type;
} offset_op_t;

typedef struct offset_ops_s {
    offset_op_t *ops;
    uint ops_count;
} offset_ops_t;

#define NO_OFFSET -1L

void offsets_add(offset_ops_t *ofs, long offset_result, long offset_oprnd1, long offset_oprnd2, char op);

void offsets_calc_all(offset_ops_t *ofs, void *base_addr);

void offsets_calc_one(offset_ops_t *ops, void *base_addr, int i, ullong value1, ullong value2);

#endif // METRICS_COMMON_OFFSET_H