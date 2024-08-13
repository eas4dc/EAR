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
#include <link.h>
#include <stdio.h>

#include <common/config.h>
#include <common/utils/string.h>
#include <library/loader/loader.h>
#include <library/loader/module_oneapi.h>
#include <library/loader/module_common.h>

static void (*func_con) (void);
static void (*func_des) (void);

extern char *program_invocation_name;

static uint gpu_oneapi_on = 0;

int is_oneapi_enabled()
{
	return gpu_oneapi_on;
}

void ear_oneapi_enabled()
{
	gpu_oneapi_on = 1;
}

static int callback(struct dl_phdr_info *info, size_t size, void *data);
int module_oneapi_is()
{
	dl_iterate_phdr(callback, NULL);
	return is_oneapi_enabled();
}


static int module_constructor_dlsym(char *path_lib_so,char *libhack,char *path_so, size_t path_so_size)
{
	void *libear;


	if ((path_lib_so == NULL) && (libhack == NULL)){
		verbose(2,"EARL cannot be loaded because paths are not defined");
		return 0;
	}

	if (libhack == NULL){	
		xsnprintf(path_so, path_so_size, "%s/%s.%s", path_lib_so, REL_NAME_LIBR, ONEAPI_EXT);
	}else{
		xsnprintf(path_so, path_so_size, "%s", libhack);
	}

	verbose(2, "LOADER: loading library %s", path_so);

	if (!module_file_exists(path_so)) {
		verbose(0, "EARL cannot be found at '%s'", path_so);
		return 0;
	}

	//
	libear = dlopen(path_so, RTLD_NOW | RTLD_GLOBAL);
	verbose(3, "LOADER: dlopen returned %p", libear);

	if (libear == NULL) {
		verbose(0, "EARL at %s cannot be loaded %s",path_so, dlerror());
		return 0;
	}

	func_con = dlsym(libear, "ear_constructor");
	func_des = dlsym(libear, "ear_destructor");
#if USE_GPUS
	ear_oneapi_enabled();
#endif

	verbose(4, "LOADER: function constructor %p", func_con);
	verbose(4, "LOADER: function destructor %p", func_des);

	if (func_con == NULL && func_des == NULL) {
		dlclose(libear);
		return 0;
	}

	return 1;
}

int module_constructor_oneapi(char *path_lib_so,char *libhack)
{
	static char path_so[4096];

	verbose(3, "LOADER: loading module oneapi (constructor)");

	if (!module_oneapi_is()){
		verbose(3, "LOADER: App is not a oneapi app");
		return 0;
	}

	if (!module_constructor_dlsym(path_lib_so,libhack,path_so, sizeof(path_so))) {
		return 0;
	}
	if (atexit(module_destructor_oneapi) != 0) {
		verbose(2, "LOADER: cannot set exit function");
	}
	if (func_con != NULL) {
		func_con();
	}

	return 1;
}

void module_destructor_oneapi()
{
	verbose(3, "LOADER: loading module oneapi (destructor)");

	if (is_oneapi_enabled()){
		if (func_des != NULL) {
			func_des();
		}	
	}
}

const int   libs_oneapi_n = 1;
const char *libs_oneapi_nam[] = {
	"libsycl"
};

static int callback(struct dl_phdr_info *info, size_t size, void *data)
{
	// char *type;
	// int p_type, j;
	uint lib_found = 0;

	verbose(2, "LOADER: Name: \"%s\" (%d segments)", info->dlpi_name,
			info->dlpi_phnum);
	for (uint l = 0; (l < libs_oneapi_n) && !lib_found ; l++){
		if (strstr(info->dlpi_name, libs_oneapi_nam[l]) != NULL){
			verbose(2,"LOADER: Library %s detected", libs_oneapi_nam[l]);
			ear_oneapi_enabled();
			lib_found = 1;
		}
	}
#if 0 
	for (j = 0; j < info->dlpi_phnum; j++) {
		p_type = info->dlpi_phdr[j].p_type;
		type =  (p_type == PT_LOAD) ? "PT_LOAD" :
			(p_type == PT_DYNAMIC) ? "PT_DYNAMIC" :
			(p_type == PT_INTERP) ? "PT_INTERP" :
			(p_type == PT_NOTE) ? "PT_NOTE" :
			(p_type == PT_INTERP) ? "PT_INTERP" :
			(p_type == PT_PHDR) ? "PT_PHDR" :
			(p_type == PT_TLS) ? "PT_TLS" :
			(p_type == PT_GNU_EH_FRAME) ? "PT_GNU_EH_FRAME" :
			(p_type == PT_GNU_STACK) ? "PT_GNU_STACK" :
			(p_type == PT_GNU_RELRO) ? "PT_GNU_RELRO" : NULL;

		printf("    %2d: [%14p; memsz:%7lx] flags: 0x%x; ", j,
				(void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr),
				info->dlpi_phdr[j].p_memsz,
				info->dlpi_phdr[j].p_flags);
		if (type != NULL)
			printf("%s\n", type);
		else
			printf("[other (0x%x)]\n", p_type);
	}
#endif

	return 0;
}
