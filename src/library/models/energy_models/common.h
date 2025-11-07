/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_ENERGY_MODELS_COMM_
#define _EAR_ENERGY_MODELS_COMM_

#include <common/types/coefficient.h>
#include <common/types/signature.h>

/** Computes the projection of the \ref signature CPI by using \ref coeff.
 * \pre Input arguments must be initialized. */
double em_common_project_cpi(signature_t *signature, coefficient_t *coeff);

#endif // _EAR_ENERGY_MODELS_COMM_
