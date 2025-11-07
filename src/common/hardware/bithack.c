/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/hardware/bithack.h>

ullong setbits64(ullong reg, ullong val, uint left_bit, uint right_bit)
{
    uint digits = (left_bit - right_bit);
    ullong mask = ~((((0x01 << (digits + 1)) - 1) << right_bit));
    return (reg & mask) | ((val) << right_bit);
}

uint setbits32(uint reg, uint val, uint left_bit, uint right_bit)
{
    uint digits = (left_bit - right_bit);
    uint mask   = ~((((0x01 << (digits + 1)) - 1) << right_bit));
    return (reg & mask) | ((val) << right_bit);
}

ullong getbits64(ullong reg, uint left_bit, uint right_bit)
{
    uint digits = left_bit - right_bit;
    ullong mask = (((0x01 << digits) - 1) << 1) + 1;
    return ((reg >> right_bit) & mask);
}

uint getbits32(uint reg, uint left_bit, uint right_bit)
{
    uint digits = left_bit - right_bit;
    uint mask   = (((0x01 << digits) - 1) << 1) + 1;
    return ((reg >> right_bit) & mask);
}

uchar getbits8(uchar reg, uchar left_bit, uchar right_bit)
{
    uchar digits = left_bit - right_bit;
    uchar mask   = (((0x01 << digits) - 1) << 1) + 1;
    return ((reg >> right_bit) & mask);
}