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

#include <common/includes.h>
#include <common/system/symplug.h>

state_t symplug_join(void *handle, void *calls[], const char *names[], uint n)
{
	char *error;
	uint i;

	for (i = 0; i < n; ++i)
	{
		calls[i] = dlsym(handle, names[i]);
		error    = dlerror();
		if ((calls[i] != NULL) && (error == NULL)) {
			debug("symbol %s found (%p)", names[i], calls[i]);
		} else {
			debug("warning, symbol %s not found (%s)", names[i], error);
			calls[i] = NULL;
		}
	}

	return EAR_SUCCESS;
}

static state_t load(char *path, void *calls[], const char *names[], uint n, int flags)
{
	void *handle = dlopen(path, flags);
	if (handle == NULL) {
		return_msg(EAR_ERROR, dlerror());
	}
	return symplug_join(handle, calls, names, n);
}

state_t symplug_open(char *path, void *calls[], const char *names[], uint n)
{
	return load(path, calls, names, n, RTLD_GLOBAL | RTLD_NOW);
}

state_t symplug_open_flags(char *path, void *calls[], const char *names[], uint n, int flags)
{
	return load(path, calls, names, n, flags);
}

state_t symplug_test(void *calls[], uint n)
{
	uint i;
	for (i = 0; i < n; ++i)
	{
		if (calls[i] == NULL) {
			return EAR_ERROR;
		}
	}
	return EAR_SUCCESS;
}
