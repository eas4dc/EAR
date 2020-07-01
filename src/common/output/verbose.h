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

#ifndef EAR_VERBOSE_H
#define EAR_VERBOSE_H

#include <stdio.h>
#include <string.h>
#include <common/output/error.h>
#include <common/output/debug.h>
#include <common/output/timestamp.h>
#include <common/output/output_conf.h>

#define fdout	STDOUT_FILENO
#define fderr	STDERR_FILENO

int verb_level		__attribute__((weak)) = 0;
int verb_channel	__attribute__((weak)) = 2;
int verb_enabled	__attribute__((weak)) = 1;
int warn_channel	__attribute__((weak)) = 2;

// Set
#define WARN_SET_FD(fd)	warn_channel = fd;
#define VERB_SET_FD(fd)	verb_channel = fd;
#define VERB_SET_EN(en)	verb_enabled = en;
#define VERB_SET_LV(lv)	verb_level   = lv;

#define verbose(v, ...) \
	if (verb_enabled && v <= verb_level) \
	{ \
		timestamp(verb_channel); \
        dprintf(verb_channel, __VA_ARGS__); \
        dprintf(verb_channel, "\n"); \
	}

// No new line version
#define verbosen(v, ...) \
	if (verb_enabled && v <= verb_level) \
	{ \
		timestamp(verb_channel); \
        dprintf(verb_channel, __VA_ARGS__); \
	}

// Warnings
#if SHOW_WARNINGS
#define warning(...) \
	{ \
    	timestamp(warn_channel); \
	 	dprintf(warn_channel, __VA_ARGS__);\
	}
#else
#define warning(...)
#endif

// Abbreviations
#define verb(v, ...) \
	verbose(v, __VA_ARGS__);

#endif //EAR_VERBOSE_H
