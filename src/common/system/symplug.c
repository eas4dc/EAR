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

//#define SHOW_DEBUGS 0
#include <common/includes.h>
#include <common/system/symplug.h>

state_t symplug_join(void *handle, void *calls[], const char *names[], uint n)
{
	char *error;
	uint i;

	for (i = 0; i < n; ++i)
	{
		debug("Looking for %s",names[i]);
		calls[i] = dlsym(handle, names[i]);
		error    = dlerror();
	
		if ((calls[i] != NULL) && (error == NULL)) {
			debug("symbol %s found (%p)", names[i], calls[i]);
		} else {
			debug("symbol %s not found (%s)", names[i], error);
			calls[i] = NULL;
		}
	}
	debug("plugjoin end");

	return EAR_SUCCESS;
}

state_t symplug_open(char *path, void *calls[], const char *names[], uint n)
{
	void *handle = dlopen(path, RTLD_GLOBAL | RTLD_NOW);
	int i;
	if (!handle)
	{
		
		debug("error when loading shared object (%s)", dlerror());
		for (i=0;i<n;i++) calls[i]=NULL;	
		state_return_msg(EAR_DL_ERROR, 0, dlerror());
	}
	
	debug("dlopen returned correctly");
	return symplug_join(handle, calls, names, n);
}

state_t symplug_open_lazy(char *path, void *calls[], const char *names[], uint n)
{
  void *handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);

  if (!handle)
  {

    debug("error when loading shared object (%s)", dlerror());
    state_return_msg(EAR_DL_ERROR, 0, dlerror());
  }

  debug("dlopen returned correctly");
  return symplug_join(handle, calls, names, n);
}

