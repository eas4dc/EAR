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
typedef struct power_signature
{
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

typedef struct accum_power_sig{
	ulong DC_energy;
	ulong DRAM_energy;
	ulong PCK_energy;
	ulong avg_f;
	double max, min;
#if USE_GPUS
	ulong GPU_energy;
#endif
}accum_power_sig_t;


// Function declarations

/** Replicates the power_signature in *source to *destiny */
void copy_power_signature(power_signature_t *destiny, power_signature_t *source);

/** Initializes all values of the power_signature to 0.*/
void init_power_signature(power_signature_t *sig);

/** returns true if basic values for sig1 and sig2 are equal with a maximum %
*   of difference defined by threshold (th) */
uint are_equal_power_sig(power_signature_t *sig1,power_signature_t *sig2,double th);

/** Outputs the power_signature contents to the file pointed by the fd. */
void print_power_signature_fd(int fd, power_signature_t *sig);

void clean_db_power_signature(power_signature_t *ps, double limit);

void power_signature_serialize(serial_buffer_t *b, power_signature_t *power_sig);

void power_signature_deserialize(serial_buffer_t *b, power_signature_t *power_sig);

#endif
