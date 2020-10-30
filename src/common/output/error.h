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

#ifndef EAR_ERROR_H
#define EAR_ERROR_H

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <common/colors.h>
#include <common/output/timestamp.h>
#include <common/output/output_conf.h>

int error_channel	__attribute__((weak)) = 2;
int error_enabled	__attribute__((weak)) = 1;

// Set
#define ERROR_SET_FD(fd) error_channel = fd;
#define ERROR_SET_EN(en) error_enabled = 0;

//
#if SHOW_ERRORS
#define error(...) \
	if (error_enabled) \
	{ \
		timestamp(error_channel); \
		dprintf(error_channel, COL_RED "ERROR" COL_CLR ", " __VA_ARGS__); \
		dprintf(error_channel, "\n"); \
	}
#else
#define error(...)
#endif

// Log
#if SHOW_LOGS
#define log_write(...) syslog(LOG_DAEMON|LOG_ERR, "LOG: " __VA_ARGS__);
#define log_open(package) openlog(package, LOG_PID|LOG_PERROR, LOG_DAEMON);
#define log_close() closelog();
#else
#define log_write(...)
#define log_open(package);
#define log_close()
#endif

#endif //EAR_ERROR_H
