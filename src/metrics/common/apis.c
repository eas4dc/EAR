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

//#define SHOW_DEBUGS 1

#include <metrics/common/apis.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/utils/strtable.h>

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
    if      (api == API_NONE    ) return "NONE";
    else if (api == API_DUMMY   ) return "DUMMY";
    else if (api == API_EARD    ) return "EARD";
    else if (api == API_BYPASS  ) return "BYPASS";
    else if (api == API_DEFAULT ) return "DEFAULT";
    else if (api == API_INTEL63 ) return "INTEL63";
    else if (api == API_AMD17   ) return "AMD17";
    else if (api == API_NVML    ) return "NVML";
    else if (api == API_PERF    ) return "PERF";
    else if (api == API_INTEL106) return "INTEL106";
    else if (api == API_LIKWID  ) return "LIKWID";
    else if (api == API_CUPTI   ) return "CUPTI";
    else if (api == API_ONEAPI  ) return "ONEAPI";
    else if (api == API_ISST    ) return "INTELSST";
    else if (api == API_FAKE    ) return "FAKE";
    else if (api == API_CPUMODEL) return "CPUMODEL";
    else if (api == API_RSMI    ) return "RSMI";
    else if (api == API_AMD19   ) return "AMD19";
    else if (api == API_INTEL143) return "INTEL143";
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
    else if (granularity == GRANULARITY_THREAD    ) return "thread";
    else if (granularity == GRANULARITY_CORE      ) return "core";
    else if (granularity == GRANULARITY_PERIPHERAL) return "peripheral";
    else if (granularity == GRANULARITY_L3_SLICE  ) return "l3 slice";
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
