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

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/system/file.h>
#include <common/system/symplug.h>
#include <common/string_enhanced.h>

int module_file_exists(char *path)
{
	return (access(path, X_OK) == 0);
}

void module_get_path_libear(char **path_lib,char **hack_lib )
{
	static char path_lib_so[4096];
	char *path = NULL;
	char *libhack = NULL;

	*path_lib = NULL;
	*hack_lib = NULL;

	// HACK_EARL_INSTALL_PATH is a hack for the whole library path, includes "lib" in the path
	path = ear_getenv(HACK_EARL_INSTALL_PATH);
	libhack = ear_getenv(HACK_LIBRARY_FILE);
	if (path == NULL){
		verbose(2, "HACK not defined %s", HACK_EARL_INSTALL_PATH);
		// ENV_PATH_EAR is the official installation path, doesn't include "lib"
		if ((path = ear_getenv(ENV_PATH_EAR)) == NULL) {
			// This last hack is to load a specific library with the whole path including library name
			if (libhack == NULL){ 
				verbose(2, "LOADER: neither installation path exists nor HACK library defined");
				return;
			}
		}else{
			snprintf(path_lib_so,sizeof(path_lib_so),"%s/%s",path,REL_PATH_LIBR);		
			*path_lib = malloc(strlen(path_lib_so)+1);
			strcpy(*path_lib,path_lib_so);
		}
	}else{
		*path_lib = malloc (strlen(path)+1);
		strcpy(*path_lib,path);
	}
	verbose(2,"LOADER path_lib_so %s",*path_lib);
	if (libhack != NULL){
		*hack_lib = malloc(strlen(libhack)+1);
		strcpy(*hack_lib,libhack);
		verbose(2,"LOADER hack file %s",*hack_lib);
	}

	return;
}

