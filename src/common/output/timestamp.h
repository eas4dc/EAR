/**************************************************************
 * *   Energy Aware Runtime (EAR)
 * *   This program is part of the Energy Aware Runtime (EAR).
 * *
 * *   EAR provides a dynamic, transparent and ligth-weigth solution for
 * *   Energy management.
 * *
 * *       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
 * *
 * *       Copyright (C) 2017
 * *   BSC Contact     mailto:ear-support@bsc.es
 * *   Lenovo contact  mailto:hpchelp@lenovo.com
 * *
 * *   EAR is free software; you can redistribute it and/or
 * *   modify it under the terms of the GNU Lesser General Public
 * *   License as published by the Free Software Foundation; either
 * *   version 2.1 of the License, or (at your option) any later version.
 * *
 * *   EAR is distributed in the hope that it will be useful,
 * *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 * *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * *   Lesser General Public License for more details.
 * *
 * *   You should have received a copy of the GNU Lesser General Public
 * *   License along with EAR; if not, write to the Free Software
 * *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * *   The GNU LEsser General Public License is contained in the file COPYING
 * */

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
