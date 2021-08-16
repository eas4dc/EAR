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
	if      (api == API_NONE      ) { verbose(0, "%sNONE"      , prefix); }
	else if (api == API_DUMMY     ) { verbose(0, "%sDUMMY"     , prefix); }
	else if (api == API_EARD      ) { verbose(0, "%sEARD"      , prefix); }
	else if (api == API_BYPASS    ) { verbose(0, "%sBYPASS"    , prefix); }
	else if (api == API_OS_CPUFREQ) { verbose(0, "%sOS_CPUFREQ", prefix); }
	else if (api == API_INTEL63   ) { verbose(0, "%sINTEL63"   , prefix); }
	else if (api == API_AMD17     ) { verbose(0, "%sAMD17"     , prefix); }
	else if (api == API_NVML      ) { verbose(0, "%sNVML"      , prefix); }
}

void apis_tostr(uint api, char *buff,size_t size)
{
  if      (api == API_NONE      ) { snprintf(buff, size, "NONE"      ); }
  else if (api == API_DUMMY     ) { snprintf(buff, size, "DUMMY"     ); }
  else if (api == API_EARD      ) { snprintf(buff, size, "EARD"      ); }
  else if (api == API_BYPASS    ) { snprintf(buff, size, "BYPASS"    ); }
  else if (api == API_OS_CPUFREQ) { snprintf(buff, size, "OS_CPUFREQ"); }
  else if (api == API_INTEL63   ) { snprintf(buff, size, "INTEL63"   ); }
  else if (api == API_AMD17     ) { snprintf(buff, size, "AMD17"     ); }
  else if (api == API_NVML      ) { snprintf(buff, size, "NVML"      ); }
}

