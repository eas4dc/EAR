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
// clang-format off

#include <common/types.h>

// This is the class of Offset Operations (ofops)
//   Objective: computing a huge number of operations in one single line, and
//   be flexible enough to support a variety of structs, arrays and types.
//
//   Supported operations:
//   Op  | Description
//   '+' | *result (o) = *oprnd1 (o) + *oprnd2 (o)
//   '-' | *result (o) = *oprnd1 (o) - *oprnd2 (o)
//   '*' | *result (o) = *oprnd1 (o) * *oprnd2 (o)
//   '/' | *result (o) = *oprnd1 (o) / *oprnd2 (o)
//   '=' | *result (o) = *oprnd1 (o)
//   '&' | &result (o) = &oprnd1 (o)
//   'r' | *result (o) = random(operand1 (v,max), operand2 (v,min))
//   'l' | *result (o) = oprnd1 (v)
//   'i' | *result (o) = oprnd1++ (v)
//   'a' | *result (of) += *oprnd1 (o)
//   (o) offset, (of) offset fixed, (v) value
//
//   Example:
//       typedef struct simple_s {
//           int32_t a;
//           int32_t b;
//           int32_t c;
//       } simple_t;
//       simple_t simp[10];
//       ofops_t ops;
//
//       ofops_add(&ops, ID_INT32, '+', sizeof(simple_t),
//           offsetof(simple_t, c), offsetof(simple_t, a), offsetof(simple_t, b), 0);
//       ofops_calc_array(&ops, simp, 10);
//   And you will get 'c = a + b' in all positions of the simp array.

typedef struct ofop_s {
    uint64_t result; //result operand, can be a value or an offset
    uint64_t oprnd1; //first operand, can be a value or an offset
    uint64_t oprnd2; //second operand, cane be a value or an offset
    size_t   struct_size;
    char     operation;
    int      type;
    int      extra;
} ofop_t;

typedef struct ofops_s {
    ofop_t *ops;
    uint ops_count;
} ofops_t;

#define NO_OFFSET ((uint64_t) -1)

void ofops_add(ofops_t *ops, int type, char op, size_t st_size, uint64_t res, uint64_t op1, uint64_t op2, int extra);

void ofops_calc_array(ofops_t *ops, void *array, uint array_count);

/* Old class, deprecated */
void offsets_add(ofops_t *ops, uint64_t off_result, uint64_t off_oprnd1, uint64_t off_oprnd2, char op);

void offsets_calc(ofops_t *ops, void *base_addr);

void offsets_calc_one(ofops_t *ops, void *base_addr, int i, ullong value1, ullong value2);

// clang-format on
#endif // METRICS_COMMON_OFFSET_H