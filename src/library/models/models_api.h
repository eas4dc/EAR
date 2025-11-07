/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MODEL_API_H
#define MODEL_API_H
#include <common/hardware/architecture.h>
#include <common/states.h>
#include <common/types/signature.h>

/* This function loads any information needed by the energy model */
state_t model_init(char *etc, char *tmp, architecture_t *arch_desc);

state_t model_project_time(signature_t *sign, ulong from, ulong to, double *ptime);

state_t model_project_power(signature_t *sign, ulong from, ulong to, double *ppower);

state_t model_projection_available(ulong from, ulong to);
#endif
