/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <metrics/common/offsets.h>
#include <stdlib.h>

void offsets_add(offset_ops_t *ops, long offset_result, long offset_oprnd1, long offset_oprnd2, char op)
{
    ops->ops                               = realloc(ops->ops, sizeof(offset_op_t) * (ops->ops_count + 1));
    ops->ops[ops->ops_count].offset_result = offset_result;
    ops->ops[ops->ops_count].offset_oprnd1 = offset_oprnd1;
    ops->ops[ops->ops_count].offset_oprnd2 = offset_oprnd2;
    ops->ops[ops->ops_count].operation     = op;
    ++ops->ops_count;
}

void offsets_calc_all(offset_ops_t *ops, void *base_addr)
{
    int i;
    for (i = 0; i < ops->ops_count; ++i) {
        int is_oprnd1 = (long) ops->ops[i].offset_oprnd1 != NO_OFFSET;
        int is_oprnd2 = (long) ops->ops[i].offset_oprnd2 != NO_OFFSET;
        void *p_op1   = base_addr + ops->ops[i].offset_oprnd1;
        void *p_op2   = base_addr + ops->ops[i].offset_oprnd2;
        if (ops->ops[i].operation == '+' && is_oprnd1 && is_oprnd2)
            offsets_calc_one(ops, base_addr, i, *((ullong *) p_op1), *((ullong *) p_op2));
        if (ops->ops[i].operation == '-' && is_oprnd1 && is_oprnd2)
            offsets_calc_one(ops, base_addr, i, *((ullong *) p_op1), *((ullong *) p_op2));
        if (ops->ops[i].operation == '=' && is_oprnd1)
            offsets_calc_one(ops, base_addr, i, *((ullong *) p_op1), 0);
        if (ops->ops[i].operation == '&' && is_oprnd1)
            offsets_calc_one(ops, base_addr, i, (ullong) ops->ops[i].offset_oprnd1, 0);
    }
}

void offsets_calc_one(offset_ops_t *ops, void *base_addr, int i, ullong value1, ullong value2)
{
    void *p_res = base_addr + ops->ops[i].offset_result;
    if (ops->ops[i].operation == '+')
        *((ullong *) p_res) = value1 + value2;
    if (ops->ops[i].operation == '-' && (value1 > value2))
        *((ullong *) p_res) = value1 - value2;
    if (ops->ops[i].operation == '=')
        *((ullong *) p_res) = value1;
    if (ops->ops[i].operation == '&')
        *((void **) p_res) = base_addr + value1;
}