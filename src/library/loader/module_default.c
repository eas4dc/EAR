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

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/config.h>
#include <library/loader/loader.h>
#include <library/loader/module_default.h>

void (*func_con) (void);
void (*func_des) (void);

static int module_file_exists(char *path)
{
	return (access(path, X_OK) == 0);
}

static int module_constructor_dlsym(char *path_so)
{
	void *libear;
	char *hack;

	// Last chance to force a concrete library file.
	if ((hack = getenv(HACK_FILE_LIBR)) != NULL) {
		sprintf(path_so, "%s", hack);
	} else if ((hack = getenv(HACK_PATH_LIBR)) != NULL) {
		sprintf(path_so, "%s/%s.%s", hack, REL_NAME_LIBR,DEF_EXT);
	} else if ((hack = getenv(VAR_INS_PATH)) != NULL) {
		sprintf(path_so, "%s/%s/%s.%s", hack, REL_PATH_LIBR, REL_NAME_LIBR,DEF_EXT);
	} else {
		verbose(2, "LOADER: installation path not found");
		return 0;
	}

	verbose(2, "LOADER: loading library %s", path_so);

	if (!module_file_exists(path_so)) {
		verbose(0, "LOADER: impossible to find library '%s'", path_so);
		return 0;
	}
	
	//
	libear = dlopen(path_so, RTLD_NOW | RTLD_GLOBAL);
	verbose(3, "LOADER: dlopen returned %p", libear);

	if (libear == NULL) {
		verbose(0, "LOADER: dlopen failed %s", dlerror());
		return 0;
	}

	func_con = dlsym(libear, "ear_constructor");
	func_des = dlsym(libear, "ear_destructor");

	verbose(4, "LOADER: function constructor %p", func_con);
	verbose(4, "LOADER: function destructor %p", func_des);

	if (func_con == NULL && func_des == NULL) {
		dlclose(libear);
		return 0;
	}

	return 1;
}

int module_constructor()
{
	static char path_so[4096];

	verbose(3, "LOADER: loading module default (constructor)");

	if (!module_constructor_dlsym(path_so)) {
		return 0;
	}
	if (atexit(module_destructor) != 0) {
		verbose(0, "LOADER: cannot set exit function");
	}
	if (func_con != NULL) {
		func_con();
	}

	return 1;
}

void module_destructor()
{
	verbose(3, "LOADER: loading module default (destructor)");

	if (func_des != NULL) {
		func_des();
	}
}
