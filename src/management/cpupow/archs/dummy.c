/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <management/cpupow/archs/dummy.h>
#include <metrics/common/apis.h>
#include <stdlib.h>

static topology_t tp;

CPUPOW_F_LOAD(mgt_cpupow_dummy_load)
{
    topology_select(tpo, &tp, TPSelect.socket, TPGroup.merge, 0);

    apis_put(ops->get_info, mgt_cpupow_dummy_get_info);
    apis_put(ops->count_devices, mgt_cpupow_dummy_count_devices);
    apis_put(ops->powercap_is_capable, mgt_cpupow_dummy_powercap_is_capable);
    apis_put(ops->powercap_is_enabled, mgt_cpupow_dummy_powercap_is_enabled);
    apis_put(ops->powercap_get, mgt_cpupow_dummy_powercap_get);
    apis_put(ops->powercap_set, mgt_cpupow_dummy_powercap_set);
    apis_put(ops->powercap_reset, mgt_cpupow_dummy_powercap_reset);
    apis_put(ops->tdp_get, mgt_cpupow_dummy_tdp_get);
}

CPUPOW_F_GET_INFO(mgt_cpupow_dummy_get_info)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_DUMMY;
    info->devs_count  = 1;
}

CPUPOW_F_COUNT_DEVICES(mgt_cpupow_dummy_count_devices)
{
    if (domain != CPUPOW_SOCKET)
        return 0;
    return tp.cpu_count;
}

CPUPOW_F_POWERCAP_IS_CAPABLE(mgt_cpupow_dummy_powercap_is_capable)
{
    return 0;
}

CPUPOW_F_POWERCAP_IS_ENABLED(mgt_cpupow_dummy_powercap_is_enabled)
{
    int i;
    for (i = 0; i < tp.cpu_count; ++i) {
        enabled[i] = 0;
    }
    return EAR_SUCCESS;
}

CPUPOW_F_POWERCAP_GET(mgt_cpupow_dummy_powercap_get)
{
    watts[0] = 0U;
    return EAR_SUCCESS;
}

CPUPOW_F_POWERCAP_SET(mgt_cpupow_dummy_powercap_set)
{
    return EAR_SUCCESS;
}

CPUPOW_F_POWERCAP_RESET(mgt_cpupow_dummy_powercap_reset)
{
    return EAR_SUCCESS;
}

CPUPOW_F_TDP_GET(mgt_cpupow_dummy_tdp_get)
{
    int i;
    for (i = 0; i < tp.cpu_count; ++i) {
        // Taken from the sum of TDPs divided by the number of processors of these models/families.
        if (tp.vendor == VENDOR_INTEL && tp.model == MODEL_HASWELL_X) {
            watts[i] = 120;
        } else if (tp.vendor == VENDOR_INTEL && tp.model == MODEL_BROADWELL_X) {
            watts[i] = 130;
        } else if (tp.vendor == VENDOR_INTEL && tp.model == MODEL_SKYLAKE_X) {
            watts[i] = 150;
        } else if (tp.vendor == VENDOR_INTEL && tp.model == MODEL_ICELAKE_X) {
            watts[i] = 235;
        } else if (tp.vendor == VENDOR_AMD && tp.family == FAMILY_ZEN) {
            watts[i] = 170;
        } else if (tp.vendor == VENDOR_AMD && tp.family == FAMILY_ZEN3) {
            watts[i] = 225;
        } else
            watts[i] = 200;
        debug("watts[%02d]: %u", tp.cpus[i].id, watts[i]);
    }
    return EAR_SUCCESS;
}
