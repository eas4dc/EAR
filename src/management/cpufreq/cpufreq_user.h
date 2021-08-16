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

#ifndef MANAGEMENT_CPUFREQ_USER
#define MANAGEMENT_CPUFREQ_USER

#include <management/cpufreq/cpufreq.h>

// This is the userspace API.
//
// Check cpufreq.h to learn about the cpufreq API. The purpose of this API is
// to offer an unprivileged version of some functions of cpufreq API. These
// functions contact with EARD internally.

// Returns the current P_STATE (struct) per CPU.
state_t mgt_cpufreq_user_get_current_list(ctx_t *c, pstate_t *pstate_list);

#endif //MANAGEMENT_CPUFREQ_USER