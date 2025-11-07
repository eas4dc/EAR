/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#include <common/hardware/architecture.h>
#include <common/output/debug.h>
#include <common/states.h>
#include <common/types/signature.h>
#include <daemon/shared_configuration.h>
#include <library/common/verbose_lib.h>
#include <library/models/energy_models/common.h>
#include <stdlib.h>

/** Returns whether there exists a coefficients entry from \ref from_ps to \ref to_ps. */
static uint projection_available(ulong from_ps, ulong to_ps);

/* This function loads any information needed by the energy model */
state_t energy_model_init(char *ear_etc_path, char *ear_tmp_path, architecture_t *arch_desc)
{
    return EAR_SUCCESS;
}

state_t energy_model_project_time(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_time)
{
    *proj_time = 1;
    return EAR_SUCCESS;
}

state_t energy_model_project_power(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_power)
{
    *proj_power = 1;
    return EAR_SUCCESS;
}

uint energy_model_projection_available(ulong from_ps, ulong to_ps)
{
    return projection_available(from_ps, to_ps);
}

uint energy_model_any_projection_available()
{
    return 1;
}

static uint projection_available(ulong from_ps, ulong to_ps)
{
    return 1;
}
