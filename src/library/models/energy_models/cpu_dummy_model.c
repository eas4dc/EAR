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
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
 * and COPYING.EPL files.
 */

#include <common/hardware/architecture.h>
#include <common/output/debug.h>
#include <common/states.h>
#include <common/types/coefficient.h>
#include <common/types/signature.h>
#include <daemon/shared_configuration.h>
#include <library/common/verbose_lib.h>
#include <library/models/energy_models/common.h>
#include <management/cpufreq/frequency.h>
#include <stdlib.h>

static coefficient_t **coefficients;
static coefficient_t *coefficients_sm;
static int num_coeffs;
static uint num_pstates;
static uint basic_model_init;

/** Returns whether there exists a coefficients entry from \ref from_ps to \ref to_ps. */
static uint projection_available(ulong from_ps, ulong to_ps);

/** Returns whether the pair <from_ps, to_ps> is between the configured pstate list. */
static int valid_range(ulong from_ps, ulong to_ps);

/* This function loads any information needed by the energy model */
state_t energy_model_init(char *ear_coeffs_path, char *ear_tmp_path, architecture_t *arch_desc)
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

static int valid_range(ulong from_ps, ulong to_ps)
{
    if ((from_ps < num_pstates) && (to_ps < num_pstates))
        return 1;
    else
        return 0;
}

static uint projection_available(ulong from_ps, ulong to_ps)
{
    return 1;
}
