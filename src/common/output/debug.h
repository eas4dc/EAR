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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef EAR_DEBUG_H
#define EAR_DEBUG_H

#include <stdio.h>
#include <string.h>

// You can use _debug to avoid SHOW_DEBUGS definition.
// Try to avoid including debug in header files.

int debug_channel	__attribute__((weak)) = 2;

#define DEBUG_SET_FD(fd) debug_channel = fd;

// Long format
#if 1
#define _debug(...) \
    dprintf(debug_channel, "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__); \
    dprintf(debug_channel, __VA_ARGS__); \
    dprintf(debug_channel, "\n");
#else
// Short format
#define _debug(...) \
    dprintf(debug_channel, __VA_ARGS__); \
    dprintf(debug_channel, "\n");
#endif

// Traditional debug
#if SHOW_DEBUGS
#define debug(...) \
{ \
    _debug(__VA_ARGS__); \
}
#else
#define debug(...)
#endif

// Conditional debug
#define cdebug(condition, ...) \
{ \
    if (condition) { \
        _debug(__VA_ARGS__); \
    } \
}

#endif //EAR_DEBUG_H
