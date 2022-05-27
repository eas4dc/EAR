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

#ifndef EAR_LOG_H
#define EAR_LOG_H

#include <syslog.h>
#include <common/output/timestamp.h>

int log_bypass __attribute__((weak)) = 0;

#define USE_SYSLOG 0

#define vlog(...) \
		syslog(LOG_DAEMON|LOG_ERR, __VA_ARGS__);

#if !USE_SYSLOG
	#define create_log(path, service_name, fd) \
        { \
		fd = log_open(path, service_name); \
        }
#else
	#define create_log(path, service_name, fd) \
        { \
		openlog(service_name, LOG_PID | LOG_PERROR, LOG_DAEMON); \
		log_bypass = 1; \
        fd = -1; \
        }
#endif

#if !USE_SYSLOG
	#define log_close(fd) \
        close(fd);
#else
	#define log_close(fd) \
		closelog();
#endif

int log_open(char *path, char *service_name);

#endif //EAR_LOG_H
