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
#include <common/output/verbose.h>
#include <common/utils/strtable.h>
#include <metrics/common/apis.h>

void apis_append(void *op[], void *func)
{
    while (*op != NULL) {
        ++op;
    }
    *op = func;
}

void apis_print(uint api, char *prefix)
{
    char buffer[128];
    apis_tostr(api, buffer, 128);
    verbose(0, "%s%s", prefix, buffer);
}

static char *_apis_tostr(uint api)
{
    if (api == API_NONE)
        return "NONE";
    else if (api == API_DUMMY)
        return "DUMMY";
    else if (api == API_EARD)
        return "EARD";
    else if (api == API_BYPASS)
        return "BYPASS";
    else if (api == API_DEFAULT)
        return "DEFAULT";
    else if (api == API_INTEL63)
        return "INTEL63";
    else if (api == API_AMD17)
        return "AMD17";
    else if (api == API_NVML)
        return "NVML";
    else if (api == API_PERF)
        return "PERF";
    else if (api == API_INTEL106)
        return "INTEL106";
    else if (api == API_LIKWID)
        return "LIKWID";
    else if (api == API_CUPTI)
        return "CUPTI";
    else if (api == API_ONEAPI)
        return "ONEAPI";
    else if (api == API_ISST)
        return "INTELSST";
    else if (api == API_FAKE)
        return "FAKE";
    else if (api == API_CPUMODEL)
        return "CPUMODEL";
    else if (api == API_RSMI)
        return "RSMI";
    else if (api == API_AMD19)
        return "AMD19";
    else if (api == API_INTEL143)
        return "INTEL143";
    else if (api == API_LINUX_POWERCAP)
        return "LINUX_POWERCAP";
    else if (api == API_DCGMI)
        return "DCGMI";
    else if (api == API_ACPI_POWER)
        return "ACPI_POWER";
    else if (api == API_GRACE_CPU)
        return "GRACE_CPU";
    else if (api == API_HWMON)
        return "HWMON";
    else if (api == API_PVC_HWMON)
        return "PVC_HWMON";

    return "NONE";
}

void apis_tostr(uint api, char *buffer, size_t size)
{
    snprintf(buffer, size, "%s", _apis_tostr(api));
}

static char *granularity_tostr(uint granularity)
{
    if      (granularity == GRANULARITY_NONE      ) return "none";
    else if (granularity == GRANULARITY_DUMMY     ) return "dummy";
    else if (granularity == GRANULARITY_PROCESS   ) return "process";
    else if (granularity == GRANULARITY_THREAD    ) return "cpu/thread";
    else if (granularity == GRANULARITY_CORE      ) return "core";
    else if (granularity == GRANULARITY_PERIPHERAL) return "peripheral";
    else if (granularity == GRANULARITY_L3_SLICE  ) return "l3 slice/ccx";
    else if (granularity == GRANULARITY_CCD       ) return "ccd";
    else if (granularity == GRANULARITY_IMC       ) return "imc";
    else if (granularity == GRANULARITY_SOCKET    ) return "socket";
    return "none";
}

static char *scope_tostr(uint scope)
{
    if      (scope == SCOPE_NONE   ) return "none";
    else if (scope == SCOPE_DUMMY  ) return "dummy";
    else if (scope == SCOPE_PROCESS) return "process";
    else if (scope == SCOPE_NODE   ) return "node";
    return "none";
}

void apinfo_tostr(apinfo_t *info)
{
    info->api_str         = _apis_tostr(info->api);
    info->scope_str       = scope_tostr(info->scope);
    info->granularity_str = granularity_tostr(info->granularity);
}
