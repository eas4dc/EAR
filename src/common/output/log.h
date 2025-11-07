/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_LOG_H
#define EAR_LOG_H

#include <common/output/timestamp.h>
#include <syslog.h>

int log_bypass __attribute__((weak)) = 0;

#define USE_SYSLOG 0

#define vlog(...)  syslog(LOG_DAEMON | LOG_ERR, __VA_ARGS__);

#if !USE_SYSLOG
#define create_log(path, service_name, fd)                                                                             \
    {                                                                                                                  \
        fd = log_open(path, service_name);                                                                             \
    }
#else
#define create_log(path, service_name, fd)                                                                             \
    {                                                                                                                  \
        openlog(service_name, LOG_PID | LOG_PERROR, LOG_DAEMON);                                                       \
        log_bypass = 1;                                                                                                \
        fd         = -1;                                                                                               \
    }
#endif

#if !USE_SYSLOG
#define log_close(fd) close(fd);
#else
#define log_close(fd) closelog();
#endif

int log_open(char *path, char *service_name);

#endif // EAR_LOG_H
