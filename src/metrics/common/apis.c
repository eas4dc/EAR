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

void apis_tostr(uint api, char *buffer, size_t size)
{
    if      (api == API_NONE    ) { snprintf(buffer, size, "NONE"    ); }
    else if (api == API_DUMMY   ) { snprintf(buffer, size, "DUMMY"   ); }
    else if (api == API_EARD    ) { snprintf(buffer, size, "EARD"    ); }
    else if (api == API_BYPASS  ) { snprintf(buffer, size, "BYPASS"  ); }
    else if (api == API_DEFAULT ) { snprintf(buffer, size, "DEFAULT" ); }
    else if (api == API_INTEL63 ) { snprintf(buffer, size, "INTEL63" ); }
    else if (api == API_AMD17   ) { snprintf(buffer, size, "AMD17"   ); }
    else if (api == API_NVML    ) { snprintf(buffer, size, "NVML"    ); }
    else if (api == API_PERF    ) { snprintf(buffer, size, "PERF"    ); }
    else if (api == API_INTEL106) { snprintf(buffer, size, "INTEL106"); }
    else if (api == API_LIKWID  ) { snprintf(buffer, size, "LIKWID"  ); }
    else if (api == API_CUPTI   ) { snprintf(buffer, size, "CUPTI"   ); }
    else if (api == API_ONEAPI  ) { snprintf(buffer, size, "ONEAPI"  ); }
    else if (api == API_ISST    ) { snprintf(buffer, size, "INTELSST"); }
    else if (api == API_FAKE    ) { snprintf(buffer, size, "FAKE"    ); }
    else if (api == API_CPUMODEL) { snprintf(buffer, size, "CPUMODEL"); }
}

void granularity_print(uint granularity, char *prefix)
{
    char buffer[128];
    granularity_tostr(granularity, buffer, 128);
    verbose(0, "%s%s", prefix, buffer);
}

void granularity_tostr(uint granularity, char *buffer, size_t size)
{
    if      (granularity == GRANULARITY_NONE    ) { snprintf(buffer, size, "none"    ); }
    else if (granularity == GRANULARITY_DUMMY   ) { snprintf(buffer, size, "dummy"   ); }
    else if (granularity == GRANULARITY_PROCESS ) { snprintf(buffer, size, "process" ); }
    else if (granularity == GRANULARITY_THREAD  ) { snprintf(buffer, size, "thread"  ); }
    else if (granularity == GRANULARITY_CORE    ) { snprintf(buffer, size, "core"    ); }
    else if (granularity == GRANULARITY_CCX     ) { snprintf(buffer, size, "ccx"     ); }
    else if (granularity == GRANULARITY_CCD     ) { snprintf(buffer, size, "ccd"     ); }
    else if (granularity == GRANULARITY_L3_SLICE) { snprintf(buffer, size, "l3 slice"); }
    else if (granularity == GRANULARITY_IMC     ) { snprintf(buffer, size, "imc"     ); }
    else if (granularity == GRANULARITY_SOCKET  ) { snprintf(buffer, size, "socket"  ); }
    else if (granularity == GRANULARITY_NODE    ) { snprintf(buffer, size, "node"    ); }
    else if (granularity == GRANULARITY_NODE    ) { snprintf(buffer, size, "node"    ); }
}
