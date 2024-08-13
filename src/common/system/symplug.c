/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <unistd.h>
#include <common/includes.h>
#include <common/output/debug.h>
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
				debug("plugin %s cannot be loaded error=%s", path, strerror(errno));
        return_msg(EAR_ERROR, strerror(errno));
    }
    if ((handle = dlopen(path, flags)) == NULL) {
				debug("plugin %s cannot be loaded by symplug error=%s", path, dlerror());
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
