/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_VERBOSE_H
#define EAR_VERBOSE_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common/output/error.h>
#include <common/output/debug.h>
#include <common/output/output_conf.h>

// Dependencies:
//  -------------          -------          -----------
// | timestamp.h | <----- | log.h | <----- | verbose.h |
//  -------------          -------     |    -----------
//                                     |
//  ---------                          |    ---------
// | debug.h |                         --- | error.h |
//  ---------                               ---------

#define fdout	STDOUT_FILENO
#define fderr	STDERR_FILENO

int verb_level		__attribute__((weak)) = 0;
int VCCONF          __attribute__((weak)) = VCCONF_DEF;
int verb_channel	__attribute__((weak)) = 2;
int verb_enabled	__attribute__((weak)) = 1;
int warn_channel	__attribute__((weak)) = 2;

// Set
#define WARN_SET_FD(fd)	  warn_channel = fd;
#define VERB_SET_FD(fd)	  verb_channel = fd;
#define VERB_SET_EN(en)	  verb_enabled = en;
#define VERB_SET_LV(lv)	  verb_level   = lv;
#define VERB_GET_LV()	    (verb_level)

#define verbose(v, ...) \
	{ \
	if (verb_enabled && v <= verb_level) \
	{ \
		if (!log_bypass) { \
			timestamp(verb_channel); \
			dprintf(verb_channel, __VA_ARGS__); \
			dprintf(verb_channel, "\n"); \
		} else { \
			vlog(__VA_ARGS__); \
		} \
	} \
	}

#define VERB_ON(v) \
	(verb_enabled && (v <= verb_level))

// No new line version
#define verbosen(v, ...) \
	{\
	if (verb_enabled && v <= verb_level) \
	{ \
		if (!log_bypass) { \
            dprintf(verb_channel, __VA_ARGS__); \
        } else { \
			vlog(__VA_ARGS__); \
		} \
	}\
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
