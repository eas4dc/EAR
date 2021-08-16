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

#ifndef COMMON_SYSTEM_SYMPLUG_H
#define COMMON_SYSTEM_SYMPLUG_H

#include <dlfcn.h>
#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>

state_t symplug_join(void *handle, void *calls[], const char *names[], uint n);

state_t symplug_open(char *path, void *calls[], const char *names[], uint n);

state_t symplug_open_flags(char *path, void *calls[], const char *names[], uint n, int flags);

state_t symplug_test(void *calls[], uint n);

#endif //COMMON_SYSTEM_SYMPLUG_H
