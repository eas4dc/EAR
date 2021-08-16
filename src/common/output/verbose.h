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
		dprintf(warn_channel, COL_YLW "WARNING" COL_CLR ", " __VA_ARGS__); \
		dprintf(warn_channel, "\n"); \
	}
#else
#define warning(...)
#endif

// Abbreviations
#define verb(v, ...) \
	verbose(v, __VA_ARGS__);

#endif //EAR_VERBOSE_H
