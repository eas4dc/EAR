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

#ifndef MANAGEMENT_GOVERNOR
#define MANAGEMENT_GOVERNOR

#include <common/types.h>
#include <common/states.h>

// Governors
//
// The purpose of this file is to split the governor from cpufreq API, because
// it created symbol conflicts when compiling.
//
// More information about governors:
// 	https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt
//
// Note: governor last means the previous governor before governor change. Init
//       means the governor set before the API was initialized.
//
struct governor_s {
	uint conservative;
	uint performance;
    uint userspace;
    uint powersave;
    uint ondemand;
    uint other;
	uint last;
	uint init;
} Governor __attribute__((weak)) = {
	.conservative = 0,
	.performance = 1,
    .userspace = 2,
	.powersave = 3,
	.ondemand = 4,
	.other = 5,
    .last = 6,
	.init = 7,
};

struct goverstr_s {
	char *conservative;
	char *performance;
	char *userspace;
	char *powersave;
	char *ondemand;
	char *other;
} Goverstr __attribute__((weak)) = {
	.conservative = "conservative",
	.performance = "performance",
	.userspace = "userspace",
	.powersave = "powersave",
	.ondemand = "ondemand",
	.other = "other",
};

state_t mgt_governor_tostr(uint governor, char *buffer);

state_t mgt_governor_toint(char *buffer, uint *governor);

#endif //MANAGEMENT_GOVERNOR
