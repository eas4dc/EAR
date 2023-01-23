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
#include <common/system/file.h>
#include <common/system/symplug.h>
#include <common/string_enhanced.h>
#include <library/loader/loader.h>
#include <library/loader/module_mpi.h>
#include <library/loader/module_common.h>
#include <library/loader/module_cuda.h>
#include <library/api/mpi_support.h>

static mpic_t next_mpic;
static mpif_t next_mpif;
mpic_t ear_mpic;
mpif_t ear_mpif;

static void (*func_con) (void);
static void (*func_des) (void);

static uint mpi_on = 0;
static uint intelmpi = 0;
static uint openmpi = 0;

static char *load_mpi_version = NULL;
uint MPI_Get_library_version_detected = 0;
static int (*my_MPI_Get_library_version)(char *version, int *resultlen);

uint force_mpi_load = 0;

int is_mpi_enabled()
{
        return mpi_on;
}

void ear_mpi_enabled()
{
        mpi_on = 1;
}

int is_intelmpi()
{
	return intelmpi;
}

int is_openmpi()
{
	return openmpi;
}

static void module_mpi_get_libear(char *path_lib_so,char *libhack,char *path_so, size_t path_so_size, int *lang_c, int *lang_f)
{
	static char buffer[4096];
	char *extension = NULL;
	char *vers = NULL;
	int len = 4096;
	int fndi = 0;
	int fndo = 0;
	int fndm = 0;

	*lang_c = 0;
	*lang_f = 0;
	if (path_lib_so != NULL) verbose(2,"LOADER path_lib_so %s",path_lib_so);
	if (libhack != NULL) verbose(2,"LOADER  using HACK %s",libhack);

	// Getting the library version
	if (MPI_Get_library_version_detected) my_MPI_Get_library_version(buffer, &len);
  else if (load_mpi_version != NULL){
    strcpy(buffer,load_mpi_version);
  }else{
    return ;
  }

	// Passing to lowercase
	strtolow(buffer);

	if (strstr(buffer, "intel") != NULL) {
		fndi = 1;
		intelmpi = 1;
	} else if ((strstr(buffer, "open mpi") != NULL) || (strstr(buffer, "ompi") != NULL)) {
		fndo = 1;
		openmpi = 1;
	} else if (strstr(buffer, "mvapich") != NULL) {
		fndm = 1;
	} else {
		verbose(2, "LOADER: no mpi version found");
		return;
	}
		
	verbose(2, "LOADER: mpi_get_version (intel: %d, open: %d, mvapich: %d)",
		fndi, fndo, fndm);

	//
	extension = INTEL_EXT;

	if (fndo) {
		extension = OPENMPI_EXT;
	}

	if ((vers = getenv(HACK_MPI_VERSION)) != NULL) {
		if (strlen(vers) > 0) {
			xsnprintf(buffer,sizeof(buffer), "%s.so", vers);		
			extension = buffer;
		}
	}
  if (path_lib_so != NULL) verbose(2,"LOADER: path %s rellibname %s extension %s",path_lib_so, REL_NAME_LIBR, extension);

	//
	if (libhack == NULL){	
		xsnprintf(path_so,path_so_size, "%s/%s.%s", path_lib_so, REL_NAME_LIBR, extension);
	}else{
		xsnprintf(path_so, path_so_size,"%s", libhack);
	}

	verbose(2,"LOADER: lib path %s",path_so);
	
	//if (!file_is_regular(path_so)) {
	if (!module_file_exists(path_so)) {
		verbose(0,"EARL cannot be found at '%s'", path_so);
		return;
	}

	//
	*lang_f = !fndi;
	*lang_c = 1;

	return;
}


static int module_mpi_dlsym(char *path_so, int lang_c, int lang_f)
{
	void **next_mpic_v = (void **) &next_mpic;
	void **next_mpif_v = (void **) &next_mpif;
	void **ear_mpic_v  = (void **) &ear_mpic;
	void **ear_mpif_v  = (void **) &ear_mpif;
	void *libear;
	int i;

	verbose(2, "LOADER: module_mpi_dlsym loading library %s (c: %d, f: %d)",
		path_so, lang_c, lang_f);

	if (MPI_Get_library_version_detected){
  #if USE_CNTD
    #ifdef COUNTDOWN_BASE
    char *disable_cntd = getenv(FLAG_DISABLE_CNTD);
    static char buffer[4096];
    snprintf(buffer, 4096, "%s/libcntd.so",COUNTDOWN_BASE);
    verbose(0, "Using countdown library path %s", buffer);
    //if ((disable_cntd != NULL) || state_fail(symplug_open(buffer, (void **) &next_mpic, mpic_names, MPIC_N))){
		if ((disable_cntd != NULL) || state_fail(symplug_open_flags(buffer, (void **) &next_mpic, mpic_names, MPIC_N, RTLD_GLOBAL | RTLD_LAZY))){
      if (disable_cntd == NULL){
        verbose(1, "Warning, countdown C symbols cannot be loaded, using NEXT symbols");
      }else {
        verbose(1, "CNTD disabled");
      }
		plug_join(RTLD_NEXT, (void **) &next_mpic, mpic_names, MPIC_N);
    }
    //if ((disable_cntd != NULL) || state_fail(symplug_open(buffer, (void **) &next_mpif, mpif_names, MPIF_N ))){
		if ((disable_cntd != NULL) || state_fail(symplug_open_flags(buffer, (void **) &next_mpif, mpif_names, MPIF_N, RTLD_GLOBAL | RTLD_LAZY))){
      if (disable_cntd == NULL){
        verbose(1, "Warning, countdown F symbols cannot be loaded, using NEXT symbols");
      }else{
        verbose(1, "CNTD disabled");
      }
		plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names, MPIF_N);
    }
    #else
	plug_join(RTLD_NEXT, (void **) &next_mpic, mpic_names, MPIC_N);
	plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names, MPIF_N);
    #endif

  #else
		plug_join(RTLD_NEXT, (void **) &next_mpic, mpic_names, MPIC_N);
		plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names, MPIF_N);
  #endif
		verbose(3, "LOADER: MPI (C) next symbols loaded, Init address is %p", next_mpic.Init);
		verbose(3, "LOADER: MPI (F) next symbols loaded, Init address is %p", next_mpif.init);
    if (next_mpif.init == NULL){
      void *Initf = dlsym(RTLD_NEXT, "mpi_init_");
      if (Initf != NULL){
        verbose(3, "LOADER: MPI (F_) mpi_init returns %p, loading again ", Initf);
        plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names_, MPIF_N);
        if (next_mpif.init != NULL){
          verbose(3, "LOADER: MPI (F_) next symbols loaded, Init address is %p", next_mpif.init);
        }
      }
    }

	}

	//
	libear = dlopen(path_so, RTLD_NOW | RTLD_GLOBAL);
	if (libear == NULL){
		verbose(0,"EARL at %s canot be loaded %s",path_so, dlerror());
	}else{
		verbose(3, "LOADER: dlopen returned %p", libear);
	}
	
	void (*ear_mpic_setnext) (mpic_t *) = NULL;
	void (*ear_mpif_setnext) (mpif_t *) = NULL;

	if (libear != NULL)
	{
		ear_mpic_setnext = dlsym(libear, "ear_mpic_setnext");
		ear_mpif_setnext = dlsym(libear, "ear_mpif_setnext");

		if (ear_mpic_setnext == NULL && lang_c) {
			lang_c = 0;
		}
		if (ear_mpif_setnext == NULL && lang_f) {
			lang_f = 0;
		}
		if (!lang_c && !lang_f) {
			verbose(2, "LOADER: the loaded library does not have MPI symbols");
			dlclose(libear);
			libear = NULL;
		}
	}

	if (libear != NULL && lang_c) plug_join(libear, (void **) &ear_mpic, ear_mpic_names, MPIC_N);
	if (libear != NULL && lang_f) plug_join(libear, (void **) &ear_mpif, ear_mpif_names, MPIF_N);

	//
	for(i = 0; i < MPIC_N; ++i)
	{
		if(ear_mpic_v[i] == NULL) {
			ear_mpic_v[i] = next_mpic_v[i];
		}
	}
	for(i = 0; i < MPIF_N; ++i)
	{
		if(ear_mpif_v[i] == NULL) {
			ear_mpif_v[i] = next_mpif_v[i];
		}
	}
	
	// Setting MPI next symbols
	if (!lang_c && !lang_f) {
		return 0;
	}
	
	if (ear_mpic_setnext != NULL && MPI_Get_library_version_detected) ear_mpic_setnext(&next_mpic);
	if (ear_mpif_setnext != NULL && MPI_Get_library_version_detected) ear_mpif_setnext(&next_mpif);
	func_con = dlsym(libear, "ear_constructor");
	func_des = dlsym(libear, "ear_destructor");
	verbose(3, "LOADER: function constructor %p", func_con);
	verbose(3, "LOADER: function destructor %p", func_des);
	
	return 1;
}

static int module_mpi_is()
{
  uint is_mpi = 0;
	load_mpi_version = NULL;
	/* We use the default approach of looking for the MPI symbol */
	my_MPI_Get_library_version = dlsym(RTLD_DEFAULT, "PMPI_Get_library_version");
  if (my_MPI_Get_library_version != NULL){
    MPI_Get_library_version_detected = 1;
    verbose(2,"MPI_Get_library_version detected");
    return 1;
	}
  verbose(2,"MPI_Get_library_version NOT detected");

	/* We check the env var only for MPI+Python */
	if (strstr(program_invocation_name,"python") != NULL){
    // The env var oversubscribes the library version detected.
    load_mpi_version = getenv(FLAG_LOAD_MPI_VERSION);
    if (load_mpi_version != NULL){ 
        verbose(2, " %s defined = %s", FLAG_LOAD_MPI_VERSION, load_mpi_version);
				return 1;
    } 
    // TODO: This check is for the transition to the new environment variables.
    // It will be removed when SCHED_LOAD_MPI_VERSION will be removed, on the next release.
    load_mpi_version = getenv(SCHED_LOAD_MPI_VERSION);            
    if (load_mpi_version != NULL) { 
    verbose(1, "LOADER: %sWARNING%s %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_LOAD_MPI_VERSION, FLAG_LOAD_MPI_VERSION);
    	verbose(2, " %s = %s", FLAG_LOAD_MPI_VERSION, load_mpi_version);
			return 1;
    }
	}

	return is_mpi;

}

int module_mpi(char *path_lib_so,char *libhack)
{
	static char path_so[4096];
	int lang_c;
	int lang_f;
	int is_mpi ;

	verbose(3, "LOADER: loading module MPI");

	is_mpi = module_mpi_is();

	if (!is_mpi) {
		verbose(2, "LOADER: no MPI detected");
		return 0;
	}

	/* If we have not detected the symbol and the load is not force, we just return 1 to mark is an mpi application */
	/* force_mpi_load is set to 1 in the MPI_Init or MPI_Init_thread function since we cannot postpone the load there */
	if (is_mpi && !MPI_Get_library_version_detected && !force_mpi_load) return 1;

	//
	module_mpi_get_libear(path_lib_so,libhack,path_so, sizeof(path_so), &lang_c, &lang_f);
	//
	if (!module_mpi_dlsym(path_so, lang_c, lang_f)){
		return is_mpi;
	}
	ear_mpi_enabled();
	/* Once EARl is loaded we can  check if the application uses CUDA and then mark as a CUDA app */
	#if USE_GPUS
	if (module_cuda_is()) ear_cuda_enabled();
	#endif
	if (atexit(module_mpi_destructor) != 0) {
    verbose(2, "LOADER: cannot set exit function");
  }
  if (func_con != NULL) {
    func_con();
  }

	return is_mpi;
	
}

void module_mpi_destructor()
{
  verbose(3, "LOADER: loading module mpi (destructor)");
	if (is_mpi_enabled()){
  	if (func_des != NULL) {
  	  func_des();
  	}
	}
}

