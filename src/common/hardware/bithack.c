/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

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