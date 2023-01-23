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

#ifndef MANAGEMENT_GOVERNOR_H
#define MANAGEMENT_GOVERNOR_H

#define _GNU_SOURCE
#include <sched.h>
#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>

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

// To compile well
#define mgt_governor_tostr(g, b)       governor_tostr(g, b)
#define mgt_governor_toint(b, g)       governor_toint(b, g)
#define mgt_governor_is(b, g)          governor_is(b, g)

/* Returns a governor name given a governor id. */
char *governor_tostr(uint governor, char *buffer);
/* Returns a governor id given a governor name. */
state_t governor_toint(char *buffer, uint *governor);
/* Given a governor name and id, returns true if it is the same. */
int governor_is(char *buffer, uint governor);

#endif //MANAGEMENT_GOVERNOR_H
