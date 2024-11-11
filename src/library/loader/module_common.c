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
#include <common/sizes.h>

int module_file_exists(char *path)
{
	return (access(path, X_OK) == 0);
}

void module_get_path_libear(char **path_lib, char **hack_lib)
{
	static char path_lib_so[4096];
    char *env_hack = NULL;
	char *env_path = NULL;

	*path_lib = NULL;
	*hack_lib = NULL;
	// HACK_EARL_INSTALL_PATH is a hack for the whole library path, includes "lib" in the path
	env_path = ear_getenv(HACK_EARL_INSTALL_PATH);
	env_hack = ear_getenv(HACK_LIBRARY_FILE);

	if (env_path == NULL) {
		verbose(2, "HACK not defined %s", HACK_EARL_INSTALL_PATH);
		// ENV_PATH_EAR is the official installation path, doesn't include "lib"
		if ((env_path = ear_getenv(ENV_PATH_EAR)) == NULL) {
			// This last hack is to load a specific library with the whole path including library name
			if (env_hack == NULL){
				verbose(2, "LOADER: neither installation path exists nor HACK library defined");
				return;
			}
		} else {
			snprintf(path_lib_so, sizeof(path_lib_so), "%s/%s", env_path, REL_PATH_LIBR);
			*path_lib = malloc(strlen(path_lib_so)+SZ_FILENAME);
			strcpy(*path_lib, path_lib_so);
		}
	} else {
		*path_lib = malloc(strlen(env_path)+SZ_FILENAME);
		strcpy(*path_lib, env_path);
	}
	verbose(2,"LOADER path_lib_so %s",*path_lib);
	if (env_hack != NULL){
		*hack_lib = malloc(strlen(env_hack)+SZ_FILENAME);
		strcpy(*hack_lib, env_hack);
		verbose(2,"LOADER hack file %s",*hack_lib);
	}
	return;
}
