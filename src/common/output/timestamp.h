/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_TIMESTAMP_H
#define EAR_TIMESTAMP_H

#include <stdio.h>
#include <string.h>
#include <time.h>

int time_enabled __attribute__((weak, unused)) = 0;
struct tm *tm_log __attribute__((weak, unused));
time_t time_log __attribute__((weak, unused));
char s_log[64] __attribute__((weak, unused));

#define TIMESTAMP_SET_EN(en) time_enabled = en;

#define timestamp(channel)                                                                                             \
    if (time_enabled) {                                                                                                \
        time(&time_log);                                                                                               \
        tm_log = localtime(&time_log);                                                                                 \
        strftime(s_log, sizeof(s_log), "%c", tm_log);                                                                  \
        dprintf(channel, "[%s] ", s_log);                                                                              \
    }

#endif // EAR_VERBOSE_H
