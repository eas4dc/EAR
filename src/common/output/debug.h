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

#ifndef EAR_DEBUG_H
#define EAR_DEBUG_H

#include <stdio.h>
#include <string.h>

int debug_channel	__attribute__((weak)) = 2;

#if SHOW_DEBUGS
#if 1
#define debug(...) \
{ \
        dprintf(debug_channel, "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__); \
        dprintf(debug_channel, __VA_ARGS__); \
        dprintf(debug_channel, "\n"); \
}
#else
#define debug(...) \
{ \
        dprintf(debug_channel, __VA_ARGS__); \
        dprintf(debug_channel, "\n"); \
}
#endif
#else
#define debug(...)
#endif

// Set
#define DEBUG_SET_FD(fd) debug_channel = fd;

#endif //EAR_DEBUG_H
