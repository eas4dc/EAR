/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_TYPES_COEFFICIENT
#define _EAR_TYPES_COEFFICIENT

#include <common/config.h>
#include <common/types/generic.h>

typedef struct coefficient {
    ulong pstate_ref;
    ulong pstate;
    uint available;
    /* For power projection */
    double A;
    double B;
    double C;
    /* For CPI projection */
    double D;
    double E;
    double F;
} coefficient_t;

typedef struct coefficient_gpu {
    ulong pstate_ref;
    ulong pstate;
    uint available;
    /* For power projection */
    double A0;
    double A1;
    double A2;
    double A3;
    /* For time projection */
    double B0;
    double B1;
    double B2;
    double B3;
} coefficient_gpu_t;

// File
int coeff_file_size(char *path);

int coeff_file_read(char *path, coefficient_t **coeffs);

int coeff_file_read_no_alloc(char *path, coefficient_t *coeffs, int size);

int coeff_gpu_file_read_no_alloc(char *path, coefficient_gpu_t *coeffs, int size, size_t offset);

// Misc
void coeff_reset(coefficient_t *coeff);

void coeff_print(coefficient_t *coeff);

#endif
