/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <common/config.h>
#include <common/math_operations.h>
#include <common/types/power_signature.h>


void power_signature_copy(power_signature_t *dst, power_signature_t *src)
{
    memcpy(dst, src, sizeof(power_signature_t));
}

void power_signature_init(power_signature_t *power_signature)
{
    memset(power_signature, 0, sizeof(power_signature_t));
}

void power_signature_print_fd(int fd, power_signature_t *power_signature)
{
	if (!power_signature) {
		return;
	}
	dprintf(fd, "%lf;%lf;%lf;%lf;%lf;%lf;%lu;%lu",
			power_signature->DC_power, power_signature->DRAM_power, power_signature->PCK_power,
			power_signature->max_DC_power,
			power_signature->min_DC_power,
			power_signature->time, power_signature->avg_f, power_signature->def_f);
}

void power_signature_db_clean(power_signature_t *ps, double limit)
{
		if (!isnormal(ps->DC_power))		ps->DC_power = 0;
		if (!isnormal(ps->DRAM_power))  ps->DRAM_power = 0;
		if (!isnormal(ps->PCK_power)) 	ps->PCK_power = 0;
		if (!isnormal(ps->EDP))					ps->EDP = 0;
		if (!isnormal(ps->max_DC_power)) ps->max_DC_power = 0;
		if (!isnormal(ps->min_DC_power)) ps->min_DC_power = 0;
		if (ps->DC_power > limit)				ps->DC_power = limit;
		if (ps->DRAM_power > limit)			ps->DRAM_power = 0;
		if (ps->PCK_power > limit)			ps->PCK_power = 0;
}

void power_signature_serialize(serial_buffer_t *b, power_signature_t *power_sig)
{
    serial_dictionary_push_auto(b, power_sig->DC_power); //
    serial_dictionary_push_auto(b, power_sig->DRAM_power);
    serial_dictionary_push_auto(b, power_sig->PCK_power);
    serial_dictionary_push_auto(b, power_sig->EDP);
    serial_dictionary_push_auto(b, power_sig->max_DC_power);
    serial_dictionary_push_auto(b, power_sig->min_DC_power);
    serial_dictionary_push_auto(b, power_sig->time);
    serial_dictionary_push_auto(b, power_sig->avg_f);
    serial_dictionary_push_auto(b, power_sig->def_f);
}

void power_signature_deserialize(serial_buffer_t *b, power_signature_t *power_sig)
{
    serial_dictionary_pop_auto(b, power_sig->DC_power); //
    serial_dictionary_pop_auto(b, power_sig->DRAM_power);
    serial_dictionary_pop_auto(b, power_sig->PCK_power);
    serial_dictionary_pop_auto(b, power_sig->EDP);
    serial_dictionary_pop_auto(b, power_sig->max_DC_power);
    serial_dictionary_pop_auto(b, power_sig->min_DC_power);
    serial_dictionary_pop_auto(b, power_sig->time);
    serial_dictionary_pop_auto(b, power_sig->avg_f);
    serial_dictionary_pop_auto(b, power_sig->def_f);
}

#define MAX_CPU_FREQ 10000000

void power_signature_clean_before_db(power_signature_t *sig, double pwr_limit)
{
    if (!isnormal(sig->time))          sig->time         = 0;
    if (!isnormal(sig->EDP))           sig->EDP          = 0;
    if (!isnormal(sig->DC_power)) 	   sig->DC_power     = 0;
    if (!isnormal(sig->DRAM_power))    sig->DRAM_power   = 0;
    if (!isnormal(sig->PCK_power))     sig->PCK_power    = 0;
    if (!isnormal(sig->max_DC_power))  sig->max_DC_power = 0;
    if (!isnormal(sig->min_DC_power))  sig->min_DC_power = 0;

    if (sig->DC_power   > pwr_limit) sig->DC_power   = pwr_limit;
    if (sig->DRAM_power > pwr_limit) sig->DRAM_power = 0;
    if (sig->PCK_power  > pwr_limit) sig->PCK_power  = 0;

    if (sig->avg_f > MAX_CPU_FREQ) sig->avg_f = MAX_CPU_FREQ;
    if (sig->def_f > MAX_CPU_FREQ) sig->def_f = MAX_CPU_FREQ;
}
