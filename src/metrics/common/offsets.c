/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <stdlib.h>
#include <common/output/debug.h>
#include <common/system/random.h>
#include <common/math_operations.h>
#include <metrics/common/offsets.h>

void ofops_add(ofops_t *ops, int type, char op, size_t st_size, uint64_t res, uint64_t op1, uint64_t op2, int extra)
{
    ops->ops = realloc(ops->ops, sizeof(ofop_t) * (ops->ops_count + 1));
    ops->ops[ops->ops_count].result      = res;
    ops->ops[ops->ops_count].oprnd1      = op1;
    ops->ops[ops->ops_count].oprnd2      = op2;
    ops->ops[ops->ops_count].struct_size = st_size;
    ops->ops[ops->ops_count].type        = type;
    ops->ops[ops->ops_count].operation   = op;
    ops->ops[ops->ops_count].extra       = extra;
    ++ops->ops_count;
}

void ofops_calc_array(ofops_t *ops, void *array, uint array_count)
{
    void    *pR; //when result is a pointer
    void    *p1; //when operand1 is a pointer
    void    *p2; //when operand2 is a pointer
    void    *pRf; //fixed pointer to the first pR value
    void    *pRl; //pointer to the pR value of the previous operation
    uint64_t val_op1; //when operand1 is a value, or auxiliar
    uint64_t val_op2; //when operand2 is a value, or auxiliar
    uint64_t val_inc; //increment value, taken from operand1
    uint64_t val_rnd; //random value, a value between operand1 (max) and operand2 (min)
    uint64_t val_ext; //extra value, used in some operations
    int a, o;

    #define operate(op_id, op_sym, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            *((type_sym *) pR) = *((type_sym *) p1) op_sym *((type_sym *) p2); \
            goto next; \
        }
    #define opsubst(op_id, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            val_op1 = (uint64_t) *((type_sym *) p1); \
            val_op2 = (uint64_t) *((type_sym *) p2); \
            *((type_sym *) pR) = (type_sym) overflow_zeros_u64(val_op1, val_op2); \
            goto next; \
        }
    #define opequal(op_id, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            *((type_sym *) pR) = *((type_sym *) p1); \
            goto next; \
        }
    #define oplitrl(op_id, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            *((type_sym *) pR) = (type_sym) val_inc; \
            goto next; \
        }
    #define opaccum(op_id, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            *((type_sym *) pRf) += *((type_sym *) p1); \
            goto next; \
        }
    #define opincrm(op_id, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            *((type_sym *) pR) = (type_sym) val_inc; \
            val_inc += 1; \
            goto next; \
        }
    #define oprandm(op_id, type_id, type_sym) \
        if (ops->ops[o].operation == op_id && ops->ops[o].type == type_id) { \
            val_rnd = (val_ext == 0)? val_op1: *((uint64_t *) pRl); \
            val_rnd = (val_rnd <= val_op1)? val_rnd: val_op1; \
            val_rnd = random_getrank64(val_rnd, ops->ops[o].oprnd2); \
            *((type_sym *) pR) = (type_sym) val_rnd; \
            goto next; \
        }
    #define oppoint(op_id) \
        if (ops->ops[o].operation == op_id) { \
            *((void **) pR) = p1; \
            goto next; \
        }
    #define multi_operate(type_id, type) \
        operate('+',  +, type_id, type); \
        opsubst('-',     type_id, type); \
        operate('*',  *, type_id, type); \
        operate('/',  /, type_id, type); \
        opequal('=',     type_id, type); \
        oplitrl('l',     type_id, type); \
        opaccum('a',     type_id, type); \
        opincrm('i',     type_id, type); \
        oprandm('r',     type_id, type);
    for (o = 0; o < ops->ops_count; ++o) {
        val_op1 = ops->ops[o].oprnd1;
        val_ext = ops->ops[o].extra;
        pR  = array + ops->ops[o].result;
        p1  = array + ops->ops[o].oprnd1;
        p2  = array + ops->ops[o].oprnd2;
        pRf = array + ops->ops[o].result;
        pRl = array + ops->ops[o+val_ext].result;
        val_inc = (uint64_t) ops->ops[o].oprnd1;
        for (a = 0; a < array_count; ++a) {
            oppoint('&');
            multi_operate(ID_UINT32, uint32_t);
            multi_operate(ID_UINT64, uint64_t);
            multi_operate(ID_FLOAT , float   );
            multi_operate(ID_DOUBLE, double  );
next:
            pR  = pR  + ops->ops[o].struct_size;
            p1  = p1  + ops->ops[o].struct_size;
            p2  = p2  + ops->ops[o].struct_size;
            pRl = pRl + ops->ops[o+val_ext].struct_size;
        }
    }
}

/* Deprecated */
void offsets_add(ofops_t *ops, size_t off_result, size_t off_oprnd1, size_t off_oprnd2, char op)
{
    ofops_add(ops, ID_ULLONG, op, 0, off_result, off_oprnd1, off_oprnd2, 0);
}

void offsets_calc(ofops_t *ops, void *base_addr)
{
    int i;
    for (i = 0; i < ops->ops_count; ++i) {
        int is_oprnd1 = (long) ops->ops[i].oprnd1 != NO_OFFSET;
        int is_oprnd2 = (long) ops->ops[i].oprnd2 != NO_OFFSET;
        void *p_op1   = base_addr + ops->ops[i].oprnd1;
        void *p_op2   = base_addr + ops->ops[i].oprnd2;
        if (ops->ops[i].operation == '+' && is_oprnd1 && is_oprnd2)
            offsets_calc_one(ops, base_addr, i, *((ullong *) p_op1), *((ullong *) p_op2));
        if (ops->ops[i].operation == '-' && is_oprnd1 && is_oprnd2)
            offsets_calc_one(ops, base_addr, i, *((ullong *) p_op1), *((ullong *) p_op2));
        if (ops->ops[i].operation == '=' && is_oprnd1)
            offsets_calc_one(ops, base_addr, i, *((ullong *) p_op1), 0);
        if (ops->ops[i].operation == '&' && is_oprnd1)
            offsets_calc_one(ops, base_addr, i, (ullong) ops->ops[i].oprnd1, 0);
    }
}

void offsets_calc_one(ofops_t *ops, void *base_addr, int i, ullong value1, ullong value2)
{
    void *p_res = base_addr + ops->ops[i].result;
    if (ops->ops[i].operation == '+')
        *((ullong *) p_res) = value1 + value2;
    if (ops->ops[i].operation == '-' && (value1 > value2))
        *((ullong *) p_res) = overflow_zeros_u64(value1, value2);
    if (ops->ops[i].operation == '=')
        *((ullong *) p_res) = value1;
    if (ops->ops[i].operation == '&')
        *((void **) p_res) = base_addr + value1;
}