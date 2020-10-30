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

#ifndef EAR_TIMESTAMP_H
#define EAR_TIMESTAMP_H

#include <time.h>
#include <stdio.h>
#include <string.h>

int time_enabled 	__attribute__ ((weak, unused)) = 0;
struct tm *tm_log	__attribute__ ((weak, unused));
time_t time_log		__attribute__ ((weak, unused));
char s_log[64]		__attribute__ ((weak, unused));

#define TIMESTAMP_SET_EN(en) time_enabled = en;

#define timestamp(channel) \
	if (time_enabled) \
	{ \
		time(&time_log); \
		tm_log = localtime(&time_log); \
		strftime(s_log, sizeof(s_log), "%c", tm_log); \
		dprintf(channel, "%s:", s_log); \
	}

#endif //EAR_VERBOSE_H
