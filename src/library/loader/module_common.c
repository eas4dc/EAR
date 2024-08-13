/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/config.h>
#include <common/output/verbose.h>

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

