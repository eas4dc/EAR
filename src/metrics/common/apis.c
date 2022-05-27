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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

// #define SHOW_DEBUGS 1

#include <metrics/common/apis.h>
#include <common/output/verbose.h>

void apis_append(void *op[], void *func)
{
	// [f0] <- ops->static_init
	// [f1]
	// [f2]
	// [f3]
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
}
