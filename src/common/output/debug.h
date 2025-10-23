/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_DEBUG_H
#define EAR_DEBUG_H

#include <stdio.h>
#include <string.h>

#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED     0
#endif
int  debug_channel	       __attribute__((weak)) = 2;
int adebug_enabled         __attribute__((weak)) = 0;

// Set
#define  DEBUG_SET_FD(fd)  debug_channel = fd;
#define ADEBUG_SET_EN(en) adebug_enabled = en;

// Long format
#if 1
#define _debug(...) \
    do { \
      dprintf(debug_channel, "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__); \
      dprintf(debug_channel, __VA_ARGS__); \
      dprintf(debug_channel, "\n"); \
    } while(0);
#else
// Short format
#define _debug(...) \
    dprintf(debug_channel, __VA_ARGS__); \
    dprintf(debug_channel, "\n");
#endif

// Traditional debug
#if SHOW_DEBUGS && DEBUG_ENABLED
#define debug(...) \
{ \
    _debug(__VA_ARGS__); \
}
#else
#define debug(...)
#endif

// Active debug
#define adebug(...) \
{ \
    if (adebug_enabled) { \
        dprintf(debug_channel, __VA_ARGS__); \
        dprintf(debug_channel, "\n"); \
    } \
}

#endif //EAR_DEBUG_H
