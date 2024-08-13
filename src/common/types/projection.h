/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/**
*    \file projection.h
*    \brief Projections are used by time&power models.
*
*/

#ifndef _EAR_TYPES_PROJECTION
#define _EAR_TYPES_PROJECTION

#include <common/types/signature.h>
#include <common/types/coefficient.h>


/* Basic for compatibility and tools */
double basic_project_time(signature_t *sign,coefficient_t *coeff);
double basic_project_cpi(signature_t *sign,coefficient_t *coeff);
double basic_project_power(signature_t *sign,coefficient_t *coeff);


#endif
