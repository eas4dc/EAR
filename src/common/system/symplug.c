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

//#define SHOW_DEBUGS 1

#include <unistd.h>
#include <common/includes.h>
#include <common/system/symplug.h>

state_t plug_join(void *handle, void *calls[], const char *names[], uint n)
{
    uint i, counter;
    char *error;

    for (counter = i = 0; i < n; ++i)
    {
        calls[i] = dlsym(handle, names[i]);
        error    = dlerror();

        if ((calls[i] != NULL) && (error == NULL)) {
            debug("symbol %s found (%p)", names[i], calls[i]);
            counter++;
        } else {
            debug("warning, symbol %s not found (%s)", names[i], error);
            calls[i] = NULL;
        }
    }
    if (counter == 0) {
        return_msg(EAR_ERROR, "No symbols loaded");
    }

    return EAR_SUCCESS;
}

static state_t static_open(char *path, void *calls[], const char *names[], uint n, int flags)
{
    void *handle;
    debug("trying to access to '%s'", path);
    if (access(path, X_OK) != 0) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    if ((handle = dlopen(path, flags)) == NULL) {
        return_msg(EAR_ERROR, dlerror());
    }
    debug("dlopen ok");
    return plug_join(handle, calls, names, n);
}

state_t symplug_open(char *path, void *calls[], const char *names[], uint n)
{
	return static_open(path, calls, names, n, RTLD_GLOBAL | RTLD_NOW);
}

state_t plug_open(char *path, void *calls[], const char *names[], uint n, int flags)
{
    return static_open(path, calls, names, n, flags);
}

state_t plug_test(void *calls[], uint n)
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
