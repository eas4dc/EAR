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

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <papi.h>

#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/config/config_env.h>
#include <common/colors.h>
#include <common/environment.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/types/version.h>
#include <common/types/application.h>
#if POWERCAP
#include <common/types/pc_app_info.h>
#endif
#include <common/hardware/frequency.h>
#include <common/hardware/hardware_info.h>

#include <library/common/externs_alloc.h>
#include <library/common/global_comm.h>
#include <library/common/library_shared_data.h>

#include <library/dynais/dynais.h>
#include <library/tracer/tracer.h>
#include <library/states/states.h>
#include <library/states/states_periodic.h>
#include <library/models/models.h>
#include <library/policies/policy.h>
#include <library/metrics/metrics.h>

#include <daemon/eard_api.h>
#include <daemon/app_mgt.h>
#include <daemon/shared_configuration.h>

#include <metrics/common/papi.h>
#if MPI
#include <mpi.h>
#include <library/api/mpi.h>
#endif

// Statics
#define BUFFSIZE 			128
#define JOB_ID_OFFSET		100
#define USE_LOCK_FILES 		1
pthread_t earl_periodic_th;
unsigned long ext_def_freq=0;
int dispose=0;

#if USE_LOCK_FILES
#include <common/system/file.h>
static char fd_lock_filename[BUFFSIZE];
static int fd_master_lock;
#endif

// Process information
static int my_id = 1;
static int my_size;
static int num_nodes;
static int ppnode;

// Loop information
static uint mpi_calls_per_loop;
static uint ear_iterations=0;
static uint ear_loop_size;
static uint ear_loop_level;
static int in_loop;

#if EAR_OVERHEAD_CONTROL
/* in us */
// These variables are shared
uint ear_periodic_mode=PERIODIC_MODE_OFF;
uint mpi_calls_in_period=10000;
uint total_mpi_calls=0;
static uint dynais_timeout=MAX_TIME_DYNAIS_WITHOUT_SIGNATURE;
static uint lib_period=PERIOD;
static uint check_every=MPI_CALLS_TO_CHECK_PERIODIC;
#endif
#define MASTERS_SYNC_VERBOSE 1
masters_info_t masters_info;
MPI_Comm new_world_comm,masters_comm;
int num_masters;
int my_master_size;
int masters_connected=0;
unsigned masters_comm_created=0;
mpi_information_t *per_node_mpi_info, *global_mpi_info;
char lib_shared_region_path[GENERIC_NAME];
char shsignature_region_path[GENERIC_NAME];
char block_file[GENERIC_NAME];
float ratio_PPN;
#if POWERCAP
app_mgt_t *app_mgt_info;
char app_mgt_path[GENERIC_NAME];
pc_app_info_t *pc_app_info_data;
char pc_app_info_path[GENERIC_NAME];
#endif

void *earl_periodic_actions(void *no_arg);
//
static void print_local_data()
{
	char ver[64];
	if (masters_info.my_master_rank==0) {
	version_to_str(ver);
	verbose(1, "------------EAR%s--------------------",ver);
	verbose(1, "App/user id: '%s'/'%s'", application.job.app_id, application.job.user_id);
	verbose(1, "Node/job id/step_id: '%s'/'%lu'/'%lu'", application.node_id, application.job.id,application.job.step_id);
	verbose(2, "App/loop summary file: '%s'/'%s'", app_summary_path, loop_summary_path);
	verbose(1, "P_STATE/frequency (turbo): %u/%lu (%d)", EAR_default_pstate, application.job.def_f, ear_use_turbo);
	verbose(1, "Tasks/nodes/ppn: %u/%d/%d", my_size, num_nodes, ppnode);
	verbose(1, "Policy (learning): %s (%d)", application.job.policy, ear_whole_app);
	verbose(1, "Policy threshold/Perf accuracy: %0.2lf/%0.2lf", application.job.th, get_ear_performance_accuracy());
	verbose(1, "DynAIS levels/window/AVX512: %d/%d/%d", get_ear_dynais_levels(), get_ear_dynais_window_size(), dynais_build_type());
	verbose(1, "VAR path: %s", get_ear_tmp());
	verbose(1, "EAR privileged user %u",system_conf->user_type==AUTHORIZED);
	verbose(1, "--------------------------------");
	}
}


/* notifies mpi process has succesfully connected with EARD */
/*********************** This function synchronizes all the node masters to guarantee all the nodes have connected with EARD, otherwise, EARL is set to OFF **************/
void notify_eard_connection(int status)
{
	char *buffer_send;
	char *buffer_recv;
	int i;
	if (masters_comm_created){
	buffer_send = calloc(    1, sizeof(char));
	buffer_recv = calloc(masters_info.my_master_size, sizeof(char));
	if ((buffer_send==NULL) || (buffer_recv==NULL)){
			error("Error, memory not available for synchronization\n");
			my_id=1;
			masters_comm_created=0;
			return;
	}
	buffer_send[0] = (char)status; 

	/* Not clear the error values */
	if (masters_info.my_master_rank==0) {
		debug("Number of nodes expected %d\n",masters_info.my_master_size);
	}
	#if MPI
	PMPI_Allgather(buffer_send, 1, MPI_BYTE, buffer_recv, 1, MPI_BYTE, masters_info.masters_comm);
	for (i = 0; i < masters_info.my_master_size; ++i)
	{       
		masters_connected+=(int)buffer_recv[i];
	}
	#else
	masters_connected=1;
	#endif	


	if (masters_info.my_master_rank==0) {
		debug("Total number of masters connected %d",masters_connected);
	}
  if (masters_connected!=masters_info.my_master_size){
  /* Some of the nodes is not ok , setting off EARL */
    if (masters_info.my_master_rank==0) {
      verbose(1,"Number of nodes expected %d , number of nodes connected %d, setting EAR to off \n",masters_info.my_master_size,masters_connected);
    }
    my_id=1;
    return;
  }else{
    if (masters_connected!=num_nodes){
      num_nodes=masters_connected;
      set_ear_num_nodes(num_nodes);
      ppnode = my_size / num_nodes;
    }
	}
	return;
	}
}

/****************************************************************************************************************************************************************/
/****************** This function creates the shared memory region to coordinate processes in same node and with other nodes  ***********************************/
/****************** It is executed by the node master ***********************************************************************************************************/
/****************************************************************************************************************************************************************/
void create_shared_regions()
{
	char *tmp=get_ear_tmp();
	int bfd=-1;

	/* This section allocates shared memory for processes in same node */
	debug("Master creating shared regions for node synchro");
  sprintf(block_file,"%s/.my_local_lock_2.%s",tmp,application.job.user_id);
  if ((bfd=file_lock_create(block_file))<0){
    error("Creating lock file for shared memory %s",block_file);
  }

	/********************* We have two shared regions, one for the node and other for all the processes in the app *****************/
	/******************************* Depending on how EAR is compiled, only one signature per node is sent *************************/
	if (get_lib_shared_data_path(tmp,lib_shared_region_path)!=EAR_SUCCESS){
		error("Getting the lib_shared_region_path");
	}else{
		lib_shared_region=create_lib_shared_data_area(lib_shared_region_path);
		if (lib_shared_region==NULL){
			error("Creating the lib_shared_data region");
		}
		my_node_id=0;
		lib_shared_region->num_processes=1;
		lib_shared_region->num_signatures=0;
	}
	debug("Node connected %u",my_node_id);
#if ONLY_MASTER == 0
	#if MPI
	if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
		error("MPI_Barrier");
		return;
	}
	if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
		error("MPI_Barrier");
		return;
	}
	#endif
#endif
	//print_lib_shared_data(lib_shared_region);
	/* This region is for processes in the same node */
	if (get_shared_signatures_path(tmp,shsignature_region_path)!=EAR_SUCCESS){
    error("Getting the shsignature_region_path	");
	}else{
		debug("Master node creating the signatures with %d processes",lib_shared_region->num_processes);
		sig_shared_region=create_shared_signatures_area(shsignature_region_path,lib_shared_region->num_processes);
		if (sig_shared_region==NULL){
			error("NULL pointer returned in create_shared_signatures_area");
		}
	}
	sig_shared_region[my_node_id].master=1;
	sig_shared_region[my_node_id].mpi_info.rank=ear_my_rank;
	clean_my_mpi_info(&sig_shared_region[my_node_id].mpi_info);

#if ONLY_MASTER == 0
	#if MPI
	if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
		error("MPI_Barrier");
		return;
	}
	#endif
#endif
	/* This part allocates memory for sharing data between nodes */
	masters_info.ppn=malloc(masters_info.my_master_size*sizeof(int));
	/* The node master, shares with other masters the number of processes in the node */
	#if MPI
	if (share_global_info(masters_info.masters_comm,(char *)&lib_shared_region->num_processes,sizeof(int),(char*)masters_info.ppn,sizeof(int))!=EAR_SUCCESS){
		error("Sharing the number of processes per node");
	}
	#else
	masters_info.ppn[0]=1;
	#endif
	int i;
	masters_info.max_ppn=masters_info.ppn[0];
	for (i=1;i<masters_info.my_master_size;i++){ 
		if (masters_info.ppn[i]>masters_info.max_ppn) masters_info.max_ppn=masters_info.ppn[i];
		if (masters_info.my_master_rank==0){ debug("Processes in node %d = %d",i,masters_info.ppn[i]);}
	}
	debug("max number of ppn is %d",masters_info.max_ppn);
	/* For scalability concerns, we can compile the system sharing all the processes information (SHARE_INFO_PER_PROCESS) or only 1 per node (SHARE_INFO_PER_NODE)*/
	#if SHARE_INFO_PER_PROCESS
	debug("Sharing info at process level, reporting N per node");
	int total_size=masters_info.max_ppn*masters_info.my_master_size*sizeof(shsignature_t);
	int total_elements=masters_info.max_ppn*masters_info.my_master_size;
	int per_node_elements=masters_info.max_ppn;
	ratio_PPN=(float)lib_shared_region->num_processes/(float)masters_info.max_ppn;
	#endif
	#if SHARE_INFO_PER_NODE
	debug("Sharing info at node level, reporting 1 per node");
	int total_size=masters_info.my_master_size*sizeof(shsignature_t);
	int total_elements=masters_info.my_master_size;
	int per_node_elements=1;
	#endif
	masters_info.nodes_info=(shsignature_t *)calloc(total_elements,sizeof(shsignature_t));
	if (masters_info.nodes_info==NULL){ 
		error("Allocating memory for node_info");
	}else{ 
		debug("%d Bytes (%d x %lu)  allocated for masters_info node_info",total_size,total_elements,sizeof(shsignature_t));
	}
	masters_info.my_mpi_info=(shsignature_t *)calloc(per_node_elements,sizeof(shsignature_t));
  if (masters_info.my_mpi_info==NULL){
    error("Allocating memory for my_mpi_info");
  }else{
    debug("%lu Bytes allocated for masters_info my_mpi_info",per_node_elements*sizeof(shsignature_t));
  }

}

/****************************************************************************************************************************************************************/
/****************** This function attaches the process to the shared regions to coordinate processes in same node ***********************************************/
/****************** It is executed by the NOT master processses  ************************************************************************************************/
/****************************************************************************************************************************************************************/


void attach_shared_regions()
{
	int bfd=-1,i;
	char *tmp=get_ear_tmp();
	/* This function is executed by processes different than masters */
	/* they first join the Node shared memory */
	sprintf(block_file,"%s/.my_local_lock.%s",tmp,application.job.user_id);
	if ((bfd=file_lock_create(block_file))<0){
		error("Creating lock file for shared memory");
	}
	if (get_lib_shared_data_path(tmp,lib_shared_region_path)!=EAR_SUCCESS){
    error("Getting the lib_shared_region_path");
  }else{
		#if MPI
		if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
			error("MPI_Barrier");
			return;
		}
		#endif
		do{
			lib_shared_region=attach_lib_shared_data_area(lib_shared_region_path);
		}while(lib_shared_region==NULL);
		while(file_lock(bfd)<0);
		my_node_id=lib_shared_region->num_processes;
    lib_shared_region->num_processes=lib_shared_region->num_processes+1;
		file_unlock(bfd);
  }
	#if MPI
  if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){	
		error("In MPI_BARRIER");
	}
  if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){	
		error("In MPI_BARRIER");
	}
	#endif
	if (get_shared_signatures_path(tmp,shsignature_region_path)!=EAR_SUCCESS){
    error("Getting the shsignature_region_path  ");
  }else{
			sig_shared_region=attach_shared_signatures_area(shsignature_region_path,lib_shared_region->num_processes);
			if (sig_shared_region==NULL){
				error("NULL pointer returned in attach_shared_signatures_area");
			}
	}
	sig_shared_region[my_node_id].master=0;
	sig_shared_region[my_node_id].mpi_info.rank=ear_my_rank;
	clean_my_mpi_info(&sig_shared_region[my_node_id].mpi_info);
	/* No masters processes don't allocate memory for data sharing between nodes */	
}

/****************************************************************************************************************************************************************/
/******************  This data is shared with EARD, it's per-application info************************************************************************************/
/****************************************************************************************************************************************************************/

void fill_application_mgt_data(app_mgt_t *a)
{
	uint total=0,i;
	a->master_rank=masters_info.my_master_rank;
  for (i=0;i<masters_info.my_master_size;i++){
    total+=masters_info.ppn[i];
  }
	a->ppn=lib_shared_region->num_processes;
	a->nodes=masters_info.my_master_size;
	a->total_processes=total;
	a->max_ppn=masters_info.max_ppn;
}
/****************************************************************************************************************************************************************/
/* Connects the mpi process to a new communicator composed by masters, only processes with master=1 belongs to the new communicator */
/****************************************************************************************************************************************************************/
void attach_to_master_set(int master)
{
	int color;
	//my_master_rank=0;
	if (master) color=0;
	else color=MPI_UNDEFINED;
	if (master) masters_info.my_master_rank=0;
	else masters_info.my_master_rank=-1;
	#if MPI
	if (PMPI_Comm_dup(MPI_COMM_WORLD,&masters_info.new_world_comm)!=MPI_SUCCESS){
		error("Duplicating MPI_COMM_WORLD");
	}
	if (PMPI_Comm_split(masters_info.new_world_comm, color, masters_info.my_master_rank, &masters_info.masters_comm)!=MPI_SUCCESS){
    error("Splitting MPI_COMM_WORLD");
  }
	#endif
	masters_comm_created=1;
	if ((masters_comm_created) && (!color)){
		#if MPI
		PMPI_Comm_rank(masters_info.masters_comm,&masters_info.my_master_rank);
		PMPI_Comm_size(masters_info.masters_comm,&masters_info.my_master_size);
		#else
		masters_info.my_master_rank=0;
		masters_info.my_master_size=1;
		#endif
		debug("New master communicator created with %d masters. My master rank %d\n",masters_info.my_master_size,masters_info.my_master_rank);
	}
}
/****************************************************************************************************************************************************************/
/* returns the local id in the node, local id is 0 for the  master processes and 1 for the others  */
/****************************************************************************************************************************************************************/
static int get_local_id(char *node_name)
{
	int master = 1;

#if USE_LOCK_FILES
	#if MPI
	debug("MPI activated");
	sprintf(fd_lock_filename, "%s/.ear_app_lock.%d", get_ear_tmp(), create_ID(my_job_id,my_step_id));

	if ((fd_master_lock = file_lock_master(fd_lock_filename)) < 0) {
		master = 1;
	} else {
		master = 0;
	}
	#else
	debug("MPI not activated");
	master=0;
	#endif

	if (master) {
		debug("Rank %d is not the master in node %s", ear_my_rank, node_name);
	}else{
		debug("Rank %d is the master in node %s", ear_my_rank, node_name);
		verbose(2, "Rank %d is the master in node %s", ear_my_rank, node_name);
	}
#else
	#if MPI
	master = get_ear_local_id();
	#else 
	master=0;
	#endif
	

	if (master < 0) {
		master = (ear_my_rank % ppnode);
	}
#endif

	return master;
}

/****************************************************************************************************************************************************************/
/**** Getting the job identification (job_id * 100 + job_step_id) */
/****************************************************************************************************************************************************************/

static void get_job_identification()
{
	char *job_id  = getenv(SCHED_JOB_ID);
	char *step_id = getenv(SCHED_STEP_ID);
	char *account_id=getenv(SCHED_JOB_ACCOUNT);

	// It is missing to use JOB_ACCOUNT
	

	if (job_id != NULL) {
		my_job_id=atoi(job_id);
		if (step_id != NULL) {
			my_step_id=atoi(step_id);
		} else {
			step_id = getenv(SCHED_STEPID);
			if (step_id != NULL) {
				my_step_id=atoi(step_id);
			} else {
				warning("Neither %s nor %s are defined, using stepid=0\n",SCHED_STEP_ID,SCHED_STEPID);
				my_step_id=NULL_STEPID;
			}
		}
	} else {
		my_job_id = getppid();
		my_step_id=0;
	}
	if (account_id==NULL) strcpy(my_account,NULL_ACCOUNT);	
	else strcpy(my_account,account_id);
	debug("JOB ID %d STEP ID %d ",my_job_id,my_step_id);
}

static void get_app_name(char *my_name)
{
	char *app_name;

	app_name = get_ear_app_name();

	if (app_name == NULL)
	{
		if (PAPI_is_initialized() == PAPI_NOT_INITED) {
			strcpy(my_name, "unknown");
		} else {
			metrics_get_app_name(my_name);
		}
		set_ear_app_name(my_name);
	} else {
		strcpy(my_name, app_name);
	}
}
#if POWERCAP
/************** This function  checks if application is a "large" job *******************/
void check_large_job_use_case()
{
	if (LIMIT_LARGE_JOBS == 0) return; /* Disabled */
	if (LIMIT_LARGE_JOBS < 1 ){ /* It is a percentage of the system size */
		debug("Pending to compute the system size");
		return;
	}
	if (app_mgt_info->nodes > LIMIT_LARGE_JOBS){
		debug("We ar a large job and we will start at a lower freq: Size %u limit %u",app_mgt_info->nodes,LIMIT_LARGE_JOBS);
	}
}
#endif

/****************************************************************************************************************************************************************/
/*** We update EARL configuration based on shared memory information, ignoring potential malicious definition **/
/****************************************************************************************************************************************************************/

void update_configuration()
{
	char *cdynais_window;
	int dynais_window=system_conf->lib_info.dynais_window;
	/* print_settings_conf(system_conf);*/
	set_ear_power_policy(system_conf->policy);
	set_ear_power_policy_th(system_conf->settings[0]);
	set_ear_p_state(system_conf->def_p_state);
	set_ear_coeff_db_pathname(system_conf->lib_info.coefficients_pathname);
	set_ear_dynais_levels(system_conf->lib_info.dynais_levels);
	/* Included for dynais tunning */
	cdynais_window=getenv(SCHED_EAR_DYNAIS_WINDOW_SIZE);
  if ((cdynais_window!=NULL) && (system_conf->user_type==AUTHORIZED)){
		dynais_window=atoi(cdynais_window);
	}
	set_ear_dynais_window_size(dynais_window);
	set_ear_learning(system_conf->learning);
	dynais_timeout=system_conf->lib_info.dynais_timeout;
	lib_period=system_conf->lib_info.lib_period;
	check_every=system_conf->lib_info.check_every;
	ear_whole_app=system_conf->learning;
}
/****************************************************************************************************************************************************************/
/****************************************************************************************************************************************************************/
/****************************************************************************************************************************************************************/
/**************************************** ear_init ************************/
/****************************************************************************************************************************************************************/
/****************************************************************************************************************************************************************/
/****************************************************************************************************************************************************************/
/****************************************************************************************************************************************************************/
/****************************************************************************************************************************************************************/


void ear_init()
{
	char *summary_pathname;
	char *tmp;
	state_t st;
	char *ext_def_freq_str=getenv(SCHED_EAR_DEF_FREQ);
	architecture_t arch_desc;

	// MPI
	#if MPI
	PMPI_Comm_rank(MPI_COMM_WORLD, &ear_my_rank);
	PMPI_Comm_size(MPI_COMM_WORLD, &my_size);
	#else
	ear_my_rank=0;
	my_size=1;
	#endif


	//debug("Reading the environment");

	// Environment initialization
	ear_lib_environment();

	tmp=get_ear_tmp();
	if (get_lib_shared_data_path(tmp,lib_shared_region_path)==EAR_SUCCESS){
		unlink(lib_shared_region_path);
	}
	

	if (ext_def_freq_str!=NULL){
		ext_def_freq=(unsigned long)atol(ext_def_freq_str);
		verbose(1,"Using externally defined default freq %lu",ext_def_freq);
	}

	// Fundamental data
	gethostname(node_name, sizeof(node_name));
	strtok(node_name, ".");

	debug("Running in %s",node_name);
	#if MPI
	verb_level = get_ear_verbose();
	#else
	verb_level=1;
	#endif
	verb_channel=2;
	set_ear_total_processes(my_size);
	ear_whole_app = get_ear_learning_phase();
	#if MPI
	num_nodes = get_ear_num_nodes();
	ppnode = my_size / num_nodes;
	#else
	num_nodes=1;
	ppnode=1;
	#endif

	get_job_identification();
	// Getting if the local process is the master or not
	/* my_id reflects whether we are the master or no my_id == 0 means we are the master *****/
	my_id = get_local_id(node_name);

	//debug("attach to master %d",my_id);

	/* All masters connect in a new MPI communicator */
	attach_to_master_set(my_id==0);

	// if we are not the master, we return
#if ONLY_MASTER
	if (my_id != 0) {
		return;
	}
#endif
	//debug("Executing EAR library IDs(%d,%d)",ear_my_rank,my_id);
	get_settings_conf_path(get_ear_tmp(),system_conf_path);
	//debug("system_conf_path %s",system_conf_path);
	system_conf = attach_settings_conf_shared_area(system_conf_path);
	get_resched_path(get_ear_tmp(),resched_conf_path);
	//debug("resched_conf_path %s",resched_conf_path);
	resched_conf = attach_resched_shared_area(resched_conf_path);

	/* Updating configuration */
	if ((system_conf!=NULL) && (resched_conf!=NULL) && (system_conf->id==create_ID(my_job_id,my_step_id))){
		debug("Updating the configuration sent by the EARD");
		update_configuration();	
	}else{
		eard_ok=0;
		//debug("Shared memory not present");
#if USE_LOCK_FILES
    if (!my_id){ 
    	debug("Application master releasing the lock %d %s", ear_my_rank,fd_lock_filename);
			file_unlock_master(fd_master_lock,fd_lock_filename);
		}
#endif
		/* Only the node master will notify the problem to the other masters */
		if (!my_id) notify_eard_connection(0);
		my_id=1;
	}	

	if (!eard_ok) return;

#if ONLY_MASTER
	if (my_id != 0) {
		return;
	}
#endif


	// Application static data and metrics
	debug("init application");
	init_application(&application);
	init_application(&loop_signature);
	application.is_mpi=1;
	loop_signature.is_mpi=1;
	application.is_learning=ear_whole_app;
	application.job.def_f=getenv_ear_p_state();
	loop_signature.is_learning=ear_whole_app;

	// Getting environment data
	get_app_name(ear_app_name);
	if (application.is_learning){
		verbose(1,"Learning phase app %s p_state %lu\n",ear_app_name,application.job.def_f);
	}
	strcpy(application.job.user_id, getenv("LOGNAME"));
	strcpy(application.node_id, node_name);
	strcpy(application.job.user_acc,my_account);
	application.job.id = my_job_id;
	application.job.step_id = my_step_id;

	//sets the job start_time
	start_job(&application.job);

	if (!my_id){ //only the master will connect with eard
		verbose(2, "%sConnecting with EAR Daemon (EARD) %d%s", COL_BLU,ear_my_rank,COL_CLR);
		if (eards_connect(&application) == EAR_SUCCESS) {
			debug("%sRank %d connected with EARD%s", COL_BLU,ear_my_rank,COL_CLR);
			notify_eard_connection(1);
		}else{   
			eard_ok=0;
			notify_eard_connection(0);
		}
	}

	if (!eard_ok) return;

	if (!my_id){
		debug("%sI'm the master, creating shared regions%s",COL_BLU,COL_CLR);
		create_shared_regions();
	}else{
		attach_shared_regions();
	}

	/* Processes in same node connectes each other*/

	debug("Dynais init");	
	// Initializing sub systems
	dynais_init(get_ear_dynais_window_size(), get_ear_dynais_levels());
	if (metrics_init()!=EAR_SUCCESS){
		    my_id=1;
				verbose(0,"Error in EAR metrics initialization, setting EARL off");
				return;
	}
	debug("frequency_init");
	frequency_init(metrics_get_node_size()); //Initialize cpufreq info

	if (ear_my_rank == 0)
	{
		if (ear_whole_app == 1 && ear_use_turbo == 1) {
			verbose(2, "turbo learning phase, turbo selected and start computing");
			if (!my_id) eards_set_turbo();
		} else {
			verbose(2, "learning phase %d, turbo %d", ear_whole_app, ear_use_turbo);
		}
	}


	// Policies && models
	if ((st=get_arch_desc(&arch_desc))!=EAR_SUCCESS){
    error("Retrieving architecture description");
		/* How to proceeed here ? */
		my_id=1;
		verbose(0,"Error in EAR metrics initialization, setting EARL off");
		return;
  }
	arch_desc.max_freq_avx512=system_conf->max_avx512_freq;
	arch_desc.max_freq_avx2=system_conf->max_avx2_freq;

	if (masters_info.my_master_rank>=0){
	  #if POWERCAP
  	get_app_mgt_path(get_ear_tmp(),app_mgt_path);
  	debug("app_mgt_path %s",app_mgt_path);
  	app_mgt_info=attach_app_mgt_shared_area(app_mgt_path);
  	if (app_mgt_info==NULL){
    	error("Application shared area not found");
  	}
		fill_application_mgt_data(app_mgt_info);
		/* At this point we have notified the EARD the number of nodes in the application */
		check_large_job_use_case();
		/* This area is RW */
		get_pc_app_info_path(get_ear_tmp(),pc_app_info_path);
		debug("pc_app_info path %s",pc_app_info_path);
		pc_app_info_data=attach_pc_app_info_shared_area(pc_app_info_path);
		if (pc_app_info_data==NULL){
			error("pc_application_info area not found");
		}
  	#endif

		//print_affinity_mask(&arch_desc.top);
		int is_set;
		if (is_affinity_set(&arch_desc.top,getpid(),&is_set)!=EAR_SUCCESS){
			error("Checking the affinity mask");
		}else{
			if (is_set)	verbose(2,"Affinity mask defined for rank %d",masters_info.my_master_rank);
		}
	}

	#if SHOW_DEBUGS
	print_arch_desc(&arch_desc);
	#endif

	debug("init_power_policy");
	init_power_policy(system_conf,resched_conf);
	debug("init_power_models");
	init_power_models(system_conf->user_type,&system_conf->installation,&arch_desc);
	if (masters_info.my_master_rank>=0) verbose(2,"Policies and models initialized");	

	if (ext_def_freq==0){
		EAR_default_frequency=system_conf->def_freq;
		EAR_default_pstate=system_conf->def_p_state;
	}else{
		EAR_default_frequency=ext_def_freq;
		EAR_default_pstate=frequency_closest_pstate(EAR_default_pstate);
		if (EAR_default_frequency!=system_conf->def_freq) {
			if (!my_id) eards_change_freq(EAR_default_frequency);
		}
	}

	strcpy(application.job.policy,system_conf->policy_name);
	strcpy(application.job.app_id, ear_app_name);

	// Passing the frequency in KHz to MHz
	application.signature.def_f=application.job.def_f = EAR_default_frequency;
	application.job.procs = my_size;
	application.job.th =system_conf->settings[0];
	

	// Copying static application info into the loop info
	memcpy(&loop_signature, &application, sizeof(application_t));

	// States
	states_begin_job(my_id,  ear_app_name);
	states_periodic_begin_job(my_id, NULL, ear_app_name);

	if (ear_periodic_mode==PERIODIC_MODE_ON){
		states_periodic_begin_period(my_id, NULL, 1, 1);
	}
	// Summary files
	summary_pathname = get_ear_user_db_pathname();

	if (summary_pathname != NULL) {
		sprintf(app_summary_path, "%s%s.csv", summary_pathname, node_name);
		sprintf(loop_summary_path, "%s%s.loop_info.csv", summary_pathname, node_name);
	}

	// Print things
	print_local_data();
	// ear_print_lib_environment();
	fflush(stderr);

	// Tracing init
	if (!my_id){
	traces_init(system_conf,application.job.app_id,masters_info.my_master_rank, my_id, num_nodes, my_size, ppnode);

	traces_start();
	traces_frequency(ear_my_rank, my_id, EAR_default_frequency);
	traces_stop();

	traces_mpi_init();
	}
	
	// All is OK :D
	if (masters_info.my_master_rank==0) {
		verbose(1, "EAR initialized successfully ");
	}
	#if !MPI
	if (pthread_create(&earl_periodic_th,NULL,earl_periodic_actions,NULL)){
		error("error creating server thread for non-earl api\n");
	}
	#endif
}


/**************************************** ear_finalize ************************/

void ear_finalize()
{
	// if we are not the master, we return
#if ONLY_MASTER
	if (my_id) {
		return;
	}	
#endif
	if (!eard_ok) return;
#if USE_LOCK_FILES
	if (!my_id){
		debug("Application master releasing the lock %d %s", ear_my_rank,fd_lock_filename);
		file_unlock_master(fd_master_lock,fd_lock_filename);
	}
#endif

	// Tracing
	if (!my_id){
	traces_stop();
	traces_end(ear_my_rank, my_id, 0);

	traces_mpi_end();
	}

	// Closing and obtaining global metrics
	debug("metrics dispose");
	dispose=1;
	if (masters_info.my_master_rank>=0) verbose(2,"Total resources computed %lu",get_total_resources());
	metrics_dispose(&application.signature, get_total_resources());
	dynais_dispose();
	if (!my_id) frequency_dispose();

	// Writing application data
	if (!my_id) 
	{
		debug("Reporting application data");
		eards_write_app_signature(&application);
		append_application_text_file(app_summary_path, &application, 1);
		report_mpi_application_data(&application);
	}
	// Closing any remaining loop
	if (loop_with_signature) {
		debug("loop ends with %d iterations detected", ear_iterations);
	}
#if EAR_OVERHEAD_CONTROL
	switch(ear_periodic_mode){
		case PERIODIC_MODE_OFF:
			if (in_loop) {
				states_end_period(ear_iterations);
			}
			states_end_job(my_id,  ear_app_name);
			break;
		case PERIODIC_MODE_ON:
			states_periodic_end_period(ear_iterations);
			states_periodic_end_job(my_id, NULL, ear_app_name);
			break;
	}
#else
	if (in_loop) {
		states_end_period(ear_iterations);
	}
	states_end_job(my_id, NULL, ear_app_name);
#endif
	policy_end();

	dettach_settings_conf_shared_area();
	dettach_resched_shared_area();

	// C'est fini
	if (!my_id){ 
		debug("Disconnecting");
		eards_disconnect();
	}
}

#if MPI

void ear_mpi_call_dynais_on(mpi_call call_type, p2i buf, p2i dest);
void ear_mpi_call_dynais_off(mpi_call call_type, p2i buf, p2i dest);

void ear_mpi_call(mpi_call call_type, p2i buf, p2i dest)
{
#if EAR_OVERHEAD_CONTROL
	time_t curr_time;
	double time_from_mpi_init;
	if (!eard_ok) return;
#if ONLY_MASTER
	if (my_id) return;
#endif
	/* The learning phase avoids EAR internals ear_whole_app is set to 1 when learning-phase is set */
	if (!ear_whole_app)
	{
		unsigned long  ear_event_l = (unsigned long)((((buf>>5)^dest)<<5)|call_type);
		//unsigned short ear_event_s = dynais_sample_convert(ear_event_l);
	if (!my_id){
	    traces_mpi_call(ear_my_rank, my_id,
                        (ulong) ear_event_l,
                        (ulong) buf,
                        (ulong) dest,
                        (ulong) call_type);
		}

		total_mpi_calls++;
		/* EAR can be driven by Dynais or periodically in those cases where dynais can not detect any period. 
		 * ear_periodic_mode can be ON or OFF 
		 */
	switch(ear_periodic_mode){
		case PERIODIC_MODE_OFF:
			switch(dynais_enabled){
				case DYNAIS_ENABLED:
					{
					/* First time EAR computes a signature using dynais, check_periodic_mode is set to 0 */
					if (check_periodic_mode==0){  // check_periodic_mode=0 the first time a signature is computed
						ear_mpi_call_dynais_on(call_type,buf,dest);
					}else{ // Check every N=check_every mpi calls
						/* Check here if we must move to periodic mode, do it every N mpicalls to reduce the overhead */
						if ((total_mpi_calls%check_every)==0){
							time(&curr_time);
							time_from_mpi_init=difftime(curr_time,application.job.start_time);
						
							/* In that case, the maximum time without signature has been passed, go to set ear_periodic_mode ON */
							if (time_from_mpi_init >=dynais_timeout){
								// we must compute N here
								ear_periodic_mode=PERIODIC_MODE_ON;
								mpi_calls_in_period=(uint)(total_mpi_calls/dynais_timeout)*lib_period;
								if (!my_id) traces_start();
								debug("Going to periodic mode after %lf secs: mpi calls in period %u\n",
									time_from_mpi_init,mpi_calls_in_period);
								states_periodic_begin_period(my_id, NULL, 1, 1);
								ear_iterations=0;
								states_periodic_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period);
							}else{

								/* We continue using dynais */
								ear_mpi_call_dynais_on(call_type,buf,dest);	
							}
						}else{	// We check the periodic mode every check_every mpi calls
							ear_mpi_call_dynais_on(call_type,buf,dest);
						}
					}
					}
					break;
				case DYNAIS_DISABLED:
					/** That case means we have computed some signature and we have decided to set dynais disabled */
					ear_mpi_call_dynais_off(call_type,buf,dest);
					break;
		
			}
			break;
		case PERIODIC_MODE_ON:
				/* EAR energy policy is called periodically */
				if ((total_mpi_calls%mpi_calls_in_period)==0){
					ear_iterations++;
					states_periodic_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period);
				}
				break;
	}
	}
#else
    if (dynais_enabled==DYNAIS_ENABLED) ear_mpi_call_dynais_on(call_type,buf,dest);
    else ear_mpi_call_dynais_off(call_type,buf,dest);
#endif
}



void ear_mpi_call_dynais_on(mpi_call call_type, p2i buf, p2i dest)
{
	short ear_status;
#if ONLY_MASTER
	if (my_id) {
		return;
	}
#endif
	// If ear_whole_app we are in the learning phase, DyNAIS is disabled then
	if (!ear_whole_app)
	{
		// Create the event for DynAIS
		unsigned long  ear_event_l;
		udyn_t ear_event_s;
		udyn_t ear_size;
		udyn_t ear_level;

		ear_event_l = (unsigned long)((((buf>>5)^dest)<<5)|call_type);
		ear_event_s = dynais_sample_convert(ear_event_l);

		//debug("EAR(%s) EAR executing before an MPI Call: DYNAIS ON\n",__FILE__);

#if 0
		traces_mpi_call(ear_my_rank, my_id,
						(ulong) ear_event_l,
						(ulong) buf,
						(ulong) dest,
						(ulong) call_type);
#endif
		mpi_calls_per_loop++;
		// This is key to detect periods
		ear_status = dynais(ear_event_s, &ear_size, &ear_level);

		switch (ear_status)
		{
			case NO_LOOP:
			case IN_LOOP:
				break;
			case NEW_LOOP:
				//debug("NEW_LOOP event %lu level %hu size %hu\n",ear_event_l,ear_level,ear_size);
				ear_iterations=0;
				states_begin_period(my_id, ear_event_l, ear_size,ear_level);
				ear_loop_size=(uint)ear_size;
				ear_loop_level=(uint)ear_level;
				in_loop=1;
				mpi_calls_per_loop=1;
				break;
			case END_NEW_LOOP:
				//debug("END_LOOP - NEW_LOOP event %lu level %hu\n",ear_event_l,ear_level);
				if (loop_with_signature) {
					//debug("loop ends with %d iterations detected", ear_iterations);
				}

				loop_with_signature=0;
				if (!my_id) traces_end_period(ear_my_rank, my_id);
				states_end_period(ear_iterations);
				ear_iterations=0;
				mpi_calls_per_loop=1;
				ear_loop_size=(uint)ear_size;
				ear_loop_level=(uint)ear_level;
				states_begin_period(my_id, ear_event_l, ear_size,ear_level);
				break;
			case NEW_ITERATION:
				ear_iterations++;

				if (loop_with_signature)
				{
					//debug("new iteration detected for level %u, event %lu, size %u and iterations %u",
					//		  ear_loop_level, ear_event_l, ear_loop_size, ear_iterations);
				}

				if (!my_id) traces_new_n_iter(ear_my_rank, my_id, ear_event_l, ear_loop_size, ear_iterations);
				states_new_iteration(my_id, ear_loop_size, ear_iterations, ear_loop_level, ear_event_l, mpi_calls_per_loop);
				mpi_calls_per_loop=1;
				break;
			case END_LOOP:
				//debug("END_LOOP event %lu\n",ear_event_l);
				if (loop_with_signature) {
					//debug("loop ends with %d iterations detected", ear_iterations);
				}
				loop_with_signature=0;
				if (!my_id)  traces_end_period(ear_my_rank, my_id);
				states_end_period(ear_iterations);
				ear_iterations=0;
				in_loop=0;
				mpi_calls_per_loop=0;
				break;
			default:
				break;
		}
	} //ear_whole_app
}

void ear_mpi_call_dynais_off(mpi_call call_type, p2i buf, p2i dest)
{
	short ear_status;
#if ONLY_MASTER
	if (my_id) {
		return;
	}
#endif

	// If ear_whole_app we are in the learning phase, DyNAIS is disabled then
	if (!ear_whole_app)
	{
		// Create the event for DynAIS: we will report anyway
		unsigned long  ear_event_l;
		unsigned short ear_size;
		unsigned short ear_level;
		ear_level=0;

		ear_event_l = (unsigned long)((((buf>>5)^dest)<<5)|call_type);
		//ear_event_s = dynais_sample_convert(ear_event_l);

		//debug("EAR(%s) EAR executing before an MPI Call: DYNAIS ON\n", __FILE__);

		if (!my_id) traces_mpi_call(ear_my_rank, my_id,
						(unsigned long) buf,
						(unsigned long) dest,
						(unsigned long) call_type,
						(unsigned long) ear_event_l);

		mpi_calls_per_loop++;

		if (last_calls_in_loop==mpi_calls_per_loop){
				ear_status=NEW_ITERATION;
		}else{
				ear_status=IN_LOOP;
		}


		switch (ear_status)
		{
			case NO_LOOP:
			case IN_LOOP:
				break;
			case NEW_ITERATION:
				ear_iterations++;

				if (loop_with_signature)
				{
					//debug("new iteration detected for level %hu, event %lu, size %u and iterations %u",
							  //ear_level, ear_event_l, ear_loop_size, ear_iterations);
				}

				if (!my_id) traces_new_n_iter(ear_my_rank, my_id, ear_event_l, ear_loop_size, ear_iterations);
				states_new_iteration(my_id, ear_loop_size, ear_iterations, (uint)ear_level, ear_event_l, mpi_calls_per_loop);
				mpi_calls_per_loop=1;
				break;
      default:
       break;
      }
    } //ear_whole_app
}

#endif

/************************** MANUAL API *******************/

static short ear_status=NO_LOOP;
static unsigned long manual_loopid=0;
unsigned long ear_new_loop()
{
  if (my_id)  return 0;
	manual_loopid++;
  if (!ear_whole_app)
  {
		switch (ear_status){
			debug("New loop reported");
			case NO_LOOP:
				ear_status=IN_LOOP;
				ear_iterations=0;
				states_begin_period(my_id, manual_loopid,1,1);
				ear_loop_size=1;
				ear_loop_level=1;
				in_loop=1;
				mpi_calls_per_loop=1;
				break;
			case IN_LOOP:
				debug("END_NEW Loop");
				if (loop_with_signature) {
					debug("loop ends with %d iterations detected", ear_iterations);
				}
				loop_with_signature=0;
				if (!my_id) traces_end_period(ear_my_rank, my_id);
				states_end_period(ear_iterations);
				ear_iterations=0;
				mpi_calls_per_loop=1;
				ear_loop_size=1;
				ear_loop_level=1;
				states_begin_period(my_id, manual_loopid, 1,1);
				break;
		}
  }
	return manual_loopid;
}

void ear_end_loop(unsigned long loop_id)
{
	if (my_id) return;
  if (!ear_whole_app)
  {
		switch(ear_status){
			case IN_LOOP:
				if (loop_id==manual_loopid){
					//debug("END_LOOP event %u\n",ear_event_l);
					if (loop_with_signature) {
						debug("loop ends with %d iterations detected", ear_iterations);
					}
					loop_with_signature=0;
					states_end_period(ear_iterations);
					if (!my_id) traces_end_period(ear_my_rank, my_id);
					ear_iterations=0;
					in_loop=0;
					mpi_calls_per_loop=0;
					ear_status=NO_LOOP;
				}else{ // Loop id is different from current one
					debug("Error END_LOOP and current loop id %lu different from active one %lu",manual_loopid,loop_id);
					ear_status=NO_LOOP;
				}
				break;
			case NO_LOOP:debug("Error, application was not in a loop");
				break;
		}
  }
}

void ear_new_iteration(unsigned long loop_id)
{
	if (my_id) return;
  if (!ear_whole_app)
  {

		switch (ear_status){
			case IN_LOOP:
				ear_iterations++;
				if (loop_with_signature)
				{
				//debug("new iteration detected for level %u, event %lu, size %u and iterations %u",
  				//1, loop_id,1, ear_iterations);
				}
				if (!my_id) traces_new_n_iter(ear_my_rank, my_id, loop_id, 1, ear_iterations);
				states_new_iteration(my_id, 1, ear_iterations, 1, loop_id, mpi_calls_per_loop);
				mpi_calls_per_loop=1;
				break;
			case NO_LOOP:
				debug("Error, application is not in a loop");
				break;
		}
  }
}


/**************** New for iterative code ********************/
void *earl_periodic_actions(void *no_arg)
{
		verbose(1,"EARL periodic thread ON");
		pthread_setname_np(pthread_self(), "EARL_periodic_th");
		if (!my_id) traces_start();
    states_periodic_begin_period(my_id, NULL, 1, 1);
    ear_iterations=0;
		mpi_calls_in_period=1;
    states_periodic_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period);
		do{
			sleep(lib_period);
      ear_iterations++;
      states_periodic_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period);
		}while(1);
}


/************** Constructor & Destructor for NOt MPI versions **************/
#if !MPI 
void ear_constructor()
{
	ear_init();
}
void ear_destructor()
{
	ear_finalize();
}
#endif


