/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_ERROR_H
#define EAR_ERROR_H

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <common/colors.h>
#include <common/output/log.h>
#include <common/output/timestamp.h>
#include <common/output/output_conf.h>

int error_channel	__attribute__((weak)) = 2;
int error_enabled	__attribute__((weak)) = 1;

// Set
#define ERROR_SET_FD(fd) error_channel = fd;
#define ERROR_SET_EN(en) error_enabled = en;

//
#if SHOW_ERRORS
#define error(...) \
	if (error_enabled) \
	{ \
		if (!log_bypass) { \
			timestamp(error_channel); \
			dprintf(error_channel, COL_RED "Error:" COL_CLR " " __VA_ARGS__); \
			dprintf(error_channel, "\n"); \
		} else { \
			vlog(__VA_ARGS__); \
		} \
	}
#else
#define error(...)
#endif

#endif //EAR_ERROR_H
