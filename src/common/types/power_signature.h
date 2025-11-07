/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_TYPES_POWER_SIGNATURE
#define _EAR_TYPES_POWER_SIGNATURE

#include <common/config.h>
#include <common/types/generic.h>
#include <common/utils/serial_buffer.h>

// WARNING! This type is serialized through functions pwoer_signature_serialize
// and power_signature_deserialize. If you want to add new types, make sure to
// update these functions too.
typedef struct power_signature {
    double DC_power;
    double DRAM_power;
    double PCK_power;
    double EDP;
    double max_DC_power;
    double min_DC_power;
    double time;
    ulong avg_f;
    ulong def_f;
} power_signature_t;

typedef struct accum_power_sig {
    ulong DC_energy;
    ulong DRAM_energy;
    ulong PCK_energy;
    ulong avg_f;
    double max, min;
#if USE_GPUS
    ulong GPU_energy;
#endif
} accum_power_sig_t;

/** Replicates the power signature in *src to *dst. */
void power_signature_copy(power_signature_t *dst, power_signature_t *src);

/** Initializes all values of the power_signature to 0.*/
void power_signature_init(power_signature_t *power_signature);

/** Outputs the power_signature contents to the file pointed by the fd. */
void power_signature_print_fd(int fd, power_signature_t *power_signature);

void power_signature_db_clean(power_signature_t *ps, double limit);

void power_signature_serialize(serial_buffer_t *b, power_signature_t *power_sig);

void power_signature_deserialize(serial_buffer_t *b, power_signature_t *power_sig);

/** \todo This function is mostly the same as power_signature_db_clean, except
 * that it also controls freq values. It is not called from anywhere.  */
void power_signature_clean_before_db(power_signature_t *sig, double pwr_limit);

#endif
