/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <library/models/energy_models/common.h>

double em_common_project_cpi(signature_t *signature, coefficient_t *coeff)
{
    return coeff->D * signature->CPI + coeff->E * signature->TPI + coeff->F;
}
