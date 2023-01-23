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

#ifndef COMMON_SYSTEM_SYMPLUG_H
#define COMMON_SYSTEM_SYMPLUG_H

#include <dlfcn.h>
#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>

state_t plug_join(void *handle, void *calls[], const char *names[], uint n);
// Old name
state_t symplug_open(char *path, void *calls[], const char *names[], uint n);

state_t plug_open(char *path, void *calls[], const char *names[], uint n, int flags);

state_t plug_test(void *calls[], uint n);

#endif //COMMON_SYSTEM_SYMPLUG_H
