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
#include <common/utils/string.h>
#include <library/loader/loader.h>
#include <library/loader/module_cuda.h>
#include <library/loader/module_common.h>

static void (*func_con) (void);
static void (*func_des) (void);

extern char *program_invocation_name;

static uint cuda_on = 0;

int is_cuda_enabled()
{
        return cuda_on;
}

void ear_cuda_enabled()
{
        cuda_on = 1;
}

int module_cuda_is()
{
    void *hdl;

    hdl = dlopen(NULL, RTLD_NOW);

	void *cudaversion = dlsym(RTLD_DEFAULT, "cudaRuntimeGetVersion");
	void *cudaversion_static = dlsym(hdl, "cudaRuntimeGetVersion");
	if ((cudaversion == NULL) && (cudaversion_static == NULL)) {
		verbose(2,"cudaRuntimeGetVersion symbol is NULL");
	}else{
		verbose(2,"cudaRuntimeGetVersion symbol is NOT NULL");
	}
  return !(cudaversion == NULL);
}


static int module_constructor_dlsym(char *path_lib_so,char *libhack,char *path_so, size_t path_so_size)
{
	void *libear;
	

	if ((path_lib_so == NULL) && (libhack == NULL)){
		verbose(2,"EARL cannot be loaded because paths are not defined");
		return 0;
	}

	if (libhack == NULL){	
		xsnprintf(path_so, path_so_size, "%s/%s.%s", path_lib_so, REL_NAME_LIBR, CUDA_EXT);
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
	ear_cuda_enabled();
	#endif

	verbose(4, "LOADER: function constructor %p", func_con);
	verbose(4, "LOADER: function destructor %p", func_des);

	if (func_con == NULL && func_des == NULL) {
		dlclose(libear);
		return 0;
	}

	return 1;
}

int module_constructor_cuda(char *path_lib_so,char *libhack)
{
	static char path_so[4096];

	verbose(3, "LOADER: loading module cuda (constructor)");

	if (!module_cuda_is()){
		verbose(3, "LOADER: App is not a CUDA app");
		return 0;
	}

	if (!module_constructor_dlsym(path_lib_so,libhack,path_so, sizeof(path_so))) {
		return 0;
	}
	if (atexit(module_destructor_cuda) != 0) {
		verbose(2, "LOADER: cannot set exit function");
	}
	if (func_con != NULL) {
		func_con();
	}

	return 1;
}

void module_destructor_cuda()
{
	verbose(3, "LOADER: loading module cuda (destructor)");

	if (is_cuda_enabled()){
		if (func_des != NULL) {
			func_des();
		}	
	}
}
