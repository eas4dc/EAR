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
#include <sched.h>
#include <pthread.h>

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
#include <assert.h>
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/config/config_env.h>
#include <common/colors.h>
#include <common/environment.h>
#include <library/common/verbose_lib.h>
#include <common/types/application.h>
#include <common/types/version.h>
#include <common/types/application.h>
#include <common/types/pc_app_info.h>
#include <management/cpufreq/frequency.h>
#include <common/hardware/hardware_info.h>
#include <common/system/monitor.h>

#include <library/common/externs_alloc.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/common/library_shared_data.h>
#include <library/common/utils.h>
#include <library/api/clasify.h>
#include <library/api/mpi_support.h>

#include <library/dynais/dynais.h>
#include <library/tracer/tracer.h>
#include <library/states/states.h>
#include <library/models/models.h>
#include <library/policies/policy.h>
#include <library/metrics/metrics.h>
#include <library/metrics/signature_ext.h>

#include <daemon/local_api/eard_api.h>
#include <daemon/app_mgt.h>
#include <daemon/shared_configuration.h>

#include <metrics/io/io.h>
#include <metrics/proc/stat.h>

#include <report/report.h>

#ifdef USE_GPUS
#include <library/api/cupti.h>
#endif

#if MPI
#include <mpi.h>
#include <library/api/mpi.h>
#endif
#include <library/api/mpi_support.h>
#include <library/policies/common/mpi_stats_support.h>

extern const char *__progname;
static uint ear_lib_initialized=0;

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

cpu_set_t ear_process_mask;
int ear_affinity_is_set=0;
architecture_t arch_desc;
// Loop information
static uint mpi_calls_per_loop;
//static uint ear_iterations=0;
uint ear_iterations=0;
static uint ear_loop_size;
static uint ear_loop_level;
static int in_loop;
static short ear_status = NO_LOOP;

#if EAR_OVERHEAD_CONTROL
/* in us */
// These variables are shared
uint ear_periodic_mode=PERIODIC_MODE_OFF;
uint mpi_calls_in_period=10000;
uint mpi_calls_per_second;
static uint total_mpi_calls=0;
static uint dynais_timeout=MAX_TIME_DYNAIS_WITHOUT_SIGNATURE;
uint lib_period=PERIOD;
static uint check_every=MPI_CALLS_TO_CHECK_PERIODIC;
#endif
#define MASTERS_SYNC_VERBOSE 1
masters_info_t masters_info;
#if MPI
MPI_Comm new_world_comm,masters_comm;
#endif
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
app_mgt_t *app_mgt_info = NULL;
char app_mgt_path[GENERIC_NAME];
pc_app_info_t *pc_app_info_data = NULL;
char pc_app_info_path[GENERIC_NAME];
#endif

uint mgt_cpufreq_api;
// Dynais
static dynais_call_t dynais;

suscription_t *earl_monitor;
state_t earl_periodic_actions(void *no_arg);
state_t earl_periodic_actions_init(void *no_arg);
static ulong seconds = 0;
//static uint ear_guided = DYNAIS_GUIDED; 
uint ear_guided = DYNAIS_GUIDED; 
//static int end_periodic_th=0;
static uint synchronize_masters = 1;

/*  Sets verbose level for ear init messages */
#define VEAR_INIT 3

uint using_verb_files = 0;

static void print_local_data()
{
	char ver[64];
	if (masters_info.my_master_rank == 0 || using_verb_files) {
	version_to_str(ver);
	#if MPI
	verbose(1, "------------EAR%s MPI enabled --------------------",ver);
	#else
	verbose(1, "------------EAR%s MPI not enabled --------------------",ver);
	#endif
	verbose(1, "App/user id: '%s'/'%s'", application.job.app_id, application.job.user_id);
	verbose(1, "Node/job id/step_id: '%s'/'%lu'/'%lu'", application.node_id, application.job.id,application.job.step_id);
	//verbose(2, "App/loop summary file: '%s'/'%s'", app_summary_path, loop_summary_path);
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
	#if MPI
	int i;
	#endif
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
	debug("Connecting with other masters. Status=%d",status);
	masters_connected=0;
	if (synchronize_masters){
		PMPI_Allgather(buffer_send, 1, MPI_BYTE, buffer_recv, 1, MPI_BYTE, masters_info.masters_comm);
		for (i = 0; i < masters_info.my_master_size; ++i)
		{       
			masters_connected+=(int)buffer_recv[i];
		}
	}
	#else
	masters_connected=1;
	#endif	


  if (masters_connected != masters_info.my_master_size){
  /* Some of the nodes is not ok , setting off EARL */
    if (masters_info.my_master_rank>=0) {
      verbose_master(1,"%s:Number of nodes expected %d , number of nodes connected %d, setting EARL to off \n",node_name,masters_info.my_master_size,masters_connected);
    }
    eard_ok = 0;
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
  int total_elements = 1;
  int per_node_elements = 1;
	#if SHOW_DEBUGS
	int total_size;
	#endif


	/* This section allocates shared memory for processes in same node */
	debug("%s:Master creating shared regions for node synchro",node_name);
  xsnprintf(block_file,sizeof(block_file),"%s/.my_local_lock_2.%s",tmp,application.job.user_id);
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
		lib_shared_region->earl_on = eard_ok;
		lib_shared_region->num_processes=1;
		lib_shared_region->num_signatures=0;
		lib_shared_region->master_rank = masters_info.my_master_rank;
	}
	debug("create_shared_regions and eard=%d",eard_ok);
	debug("Node connected %u",my_node_id);
#if ONLY_MASTER == 0
	#if MPI
	/* This barrier notifies the others the shared memory region is ready */
	if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
		error("MPI_Barrier");
		return;
	}
	if (!eard_ok) return;
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
	sig_shared_region[my_node_id].master = 1;
	sig_shared_region[my_node_id].pid = getpid();
	sig_shared_region[my_node_id].mpi_info.rank = ear_my_rank;
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
	masters_info.ppn = malloc(masters_info.my_master_size*sizeof(int));
	/* The node master, shares with other masters the number of processes in the node */
	#if MPI
	if (share_global_info(masters_info.masters_comm,(char *)&lib_shared_region->num_processes,sizeof(int),(char*)masters_info.ppn,sizeof(int))!=EAR_SUCCESS){
		error("Sharing the number of processes per node");
	}
	#else
	masters_info.ppn[0] = 1;
	#endif
	int i;
	masters_info.max_ppn = masters_info.ppn[0];
	for (i=1;i<masters_info.my_master_size;i++){ 
		if (masters_info.ppn[i] > masters_info.max_ppn) masters_info.max_ppn=masters_info.ppn[i];
		if (masters_info.my_master_rank == 0) verbose_master(2,"Processes in node %d = %d",i,masters_info.ppn[i]);
	}
	verbose_master(2,"max number of ppn is %d",masters_info.max_ppn);
	/* For scalability concerns, we can compile the system sharing all the processes information (SHARE_INFO_PER_PROCESS) or only 1 per node (SHARE_INFO_PER_NODE)*/
	if (sh_sig_per_node && sh_sig_per_proces){
		error("Signatures can only be shared with node OR process granularity,not both, default node");
		sh_sig_per_node = 1;
		sh_sig_per_proces = 0;
	}
	if (sh_sig_per_proces){
		#if SHOW_DEBUGS
		debug("Sharing info at process level, reporting N per node");
		total_size = masters_info.max_ppn*masters_info.my_master_size*sizeof(shsignature_t);
		#endif
		total_elements = masters_info.max_ppn*masters_info.my_master_size;
		per_node_elements = masters_info.max_ppn;
	}
	if (sh_sig_per_node){
		#if SHOW_DEBUGS
		debug("Sharing info at node level, reporting 1 per node");
		total_size = masters_info.my_master_size*sizeof(shsignature_t);
		#endif
		total_elements = masters_info.my_master_size;
		per_node_elements = 1;
	}
	ratio_PPN = (float)lib_shared_region->num_processes/(float)masters_info.max_ppn;
	masters_info.nodes_info = (shsignature_t *)calloc(total_elements,sizeof(shsignature_t));
	if (masters_info.nodes_info == NULL){ 
		error("Allocating memory for node_info");
	}else{ 
		debug("%d Bytes (%d x %lu)  allocated for masters_info node_info",total_size,total_elements,sizeof(shsignature_t));
	}
	masters_info.my_mpi_info = (shsignature_t *)calloc(per_node_elements,sizeof(shsignature_t));
  if (masters_info.my_mpi_info == NULL){
    error("Allocating memory for my_mpi_info");
  }else{
    debug("%lu Bytes allocated for masters_info my_mpi_info",per_node_elements*sizeof(shsignature_t));
  }
	#if NODE_LB
	masters_info.nodes_mpi_summary = (mpi_summary_t *)calloc(masters_info.my_master_size,sizeof(mpi_summary_t));
	if (masters_info.nodes_mpi_summary == NULL){
		error("Allocating memory for MPI summary shared info");
	}
	#endif

}

/****************************************************************************************************************************************************************/
/****************** This function attaches the process to the shared regions to coordinate processes in same node ***********************************************/
/****************** It is executed by the NOT master processses  ************************************************************************************************/
/****************************************************************************************************************************************************************/


void attach_shared_regions()
{
	int bfd = -1;
	char *tmp = get_ear_tmp();
	/* This function is executed by processes different than masters */
	/* they first join the Node shared memory */
	xsnprintf(block_file,sizeof(block_file),"%s/.my_local_lock.%s",tmp,application.job.user_id);
	if ((bfd = file_lock_create(block_file))<0){
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
			lib_shared_region = attach_lib_shared_data_area(lib_shared_region_path);
		}while(lib_shared_region == NULL);
		while(file_lock(bfd) < 0);
		if (!lib_shared_region->earl_on){
			debug("%s:Setting earl off because of eard",node_name);
			eard_ok = 0;
			file_unlock(bfd);
			return;
		}
		my_node_id = lib_shared_region->num_processes;
    lib_shared_region->num_processes = lib_shared_region->num_processes+1;
		file_unlock(bfd);
  }
	#if MPI
  if (PMPI_Barrier(MPI_COMM_WORLD)!= MPI_SUCCESS){	
		error("In MPI_BARRIER");
	}
  if (PMPI_Barrier(MPI_COMM_WORLD)!= MPI_SUCCESS){	
		error("In MPI_BARRIER");
	}
	#endif
	if (get_shared_signatures_path(tmp,shsignature_region_path) != EAR_SUCCESS){
    error("Getting the shsignature_region_path  ");
  }else{
			sig_shared_region = attach_shared_signatures_area(shsignature_region_path,lib_shared_region->num_processes);
			if (sig_shared_region == NULL){
				error("NULL pointer returned in attach_shared_signatures_area");
			}
	}
	sig_shared_region[my_node_id].master = 0;
	sig_shared_region[my_node_id].pid = getpid();
	sig_shared_region[my_node_id].mpi_info.rank = ear_my_rank;
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

int sched_num_nodes()
{
	char *cn;
	cn = getenv(SCHED_STEP_NUM_NODES);
	if (cn != NULL) return atoi(cn);
	else return 1;
}
void attach_to_master_set(int master)
{
	int color;
	#if MPI
	int flag = 1;
	#endif
	//my_master_rank=0;
	color=0;
	#if MPI
	if (master) color=0;
	else color=MPI_UNDEFINED;
	#endif
	if (master) masters_info.my_master_rank=0;
	else masters_info.my_master_rank=-1;
	#if MPI
	/* If OpenMPI, we will not synchronize if not all the processes answer the gather */
	if (is_mpi_enabled() && is_openmpi()){
		int n = openmpi_num_nodes();
		if ((n > 0) && (n != sched_num_nodes())) flag = 0;
		debug("OpenMPI num nodes detected %d - Sched num nodes detected %d",n, sched_num_nodes());
		if (flag == 0){ 
			synchronize_masters = 0;
		}
	}
	/* We must check all nodes have EAR loaded */
	if (synchronize_masters){
		debug("Creating a new communicator");
		if (PMPI_Comm_dup(MPI_COMM_WORLD,&masters_info.new_world_comm)!=MPI_SUCCESS){
		error("Duplicating MPI_COMM_WORLD");
		}
		if (PMPI_Comm_split(masters_info.new_world_comm, color, masters_info.my_master_rank, &masters_info.masters_comm)!=MPI_SUCCESS){
    error("Splitting MPI_COMM_WORLD");
    		
  		}
		debug("New communicators created");
	}else{
		debug("Some processes are not connecting, synchronization off");
	}
	#endif
	// OpenMPI support
	if (synchronize_masters) masters_comm_created=1;
	else{ 
		if  (master){
			masters_info.my_master_size = 1;
		}
	}
	// New for OpenMPI support
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
	debug("Attach to master set end");
}
/****************************************************************************************************************************************************************/
/* returns the local id in the node, local id is 0 for the  master processes and 1 for the others  */
/****************************************************************************************************************************************************************/
static int get_local_id(char *node_name)
{
	int master = 1;

#if USE_LOCK_FILES
  sprintf(fd_lock_filename, "%s/.ear_app_lock.%d", get_ear_tmp(), create_ID(my_job_id,my_step_id));
	debug("Using lock file %s",fd_lock_filename);
  if ((fd_master_lock = file_lock_master(fd_lock_filename)) < 0) {
		if (errno != EBUSY) debug("Error creating lock file %s",strerror(errno));
    master = 1;
  } else {
    master = 0;
  }
	#if SHOW_DEBUGS
  #if MPI
  if (master) {
    debug("Rank %d is not the master in node %s", ear_my_rank, node_name);
  }else{
    debug("Rank %d is the master in node %s", ear_my_rank, node_name);
  }
	#else
  if (master) {
    debug("Process %d is not the master in node %s", getpid(), node_name);
  }else{
    debug("Process %d is the master in node %s", getpid(), node_name);
  }
  #endif
	#endif

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
	char *num_tasks=getenv(SCHED_NUM_TASKS);
	
	if (num_tasks != NULL){
		debug("%s defined %s",SCHED_NUM_TASKS,num_tasks);
	}

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
	debug("JOB ID %d STEP ID %d ACCOUNT %s",my_job_id,my_step_id,my_account);
}

static void get_app_name(char *my_name)
{
	char *app_name;

	app_name = get_ear_app_name();

	if (app_name == NULL)
	{
		strcpy(my_name, __progname);
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

void pin_processes(topology_t *t,cpu_set_t *mask,int is_set,int ppn,int idx)
{
	int first,cpus,i;
	debug("We are %d processes in this node and I'm the number %d , Total CPUS %d",ppn,idx,t->cpu_count);
	if (!is_set){
		if (ppn == 1){
			first = 0;
			cpus = 1;
		}else{
			cpus = t->cpu_count / ppn;
			first = idx *cpus;
		}
		debug("I'm the process %d: My cpu rank is [%d-%d)",idx,first,first+cpus);
		CPU_ZERO(mask);
		for (i=0;i<t->cpu_count;i++){
				if ((i >= first) && (i < (first+cpus))) CPU_SET(i,mask);
		}	
		sched_setaffinity(getpid(),sizeof(cpu_set_t),mask);
	}
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
	char *verb_path=getenv(SCHED_EARL_VERBOSE_PATH);
	char *ext_def_freq_str=getenv(SCHED_EAR_DEF_FREQ);

	if (ear_lib_initialized){
		verbose_master(1,"EAR library already initialized");
		return;
	}

    timestamp_getfast(&ear_application_time_init);

	// MPI
	#if MPI
	PMPI_Comm_rank(MPI_COMM_WORLD, &ear_my_rank);
	PMPI_Comm_size(MPI_COMM_WORLD, &my_size);
	#else
	ear_my_rank=0;
	my_size=1;
	#endif

	load_app_mgr_env();

	//debug("Reading the environment");

	// Environment initialization
	ear_lib_environment();

	tmp=get_ear_tmp();
	if (get_lib_shared_data_path(tmp,lib_shared_region_path)==EAR_SUCCESS){
		unlink(lib_shared_region_path);
	}

	if (ext_def_freq_str!=NULL){
		ext_def_freq=(unsigned long)atol(ext_def_freq_str);
		verbose_master(VEAR_INIT,"Using externally defined default freq %lu",ext_def_freq);
	}

	// Fundamental data
	gethostname(node_name, sizeof(node_name));
	strtok(node_name, ".");

	debug("Running in %s process=%d",node_name,getpid());

	verb_level = get_ear_verbose();
	verb_channel=2;
	if ((tmp = getenv(SCHED_EARL_VERBOSE)) != NULL) {
		verb_level = atoi(tmp);
		VERB_SET_LV(verb_level);
	}

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

	if (is_already_connected(my_job_id,my_step_id,getpid())){
		verbose_master(VEAR_INIT,"Warning, Job %d step %d process %d already conncted\n",my_job_id,my_step_id,getpid());
		return;
	}
	// Getting if the local process is the master or not
	/* my_id reflects whether we are the master or no my_id == 0 means we are the master *****/
	my_id = get_local_id(node_name);

	debug("attach to master %d",my_id);

	/* All masters connect in a new MPI communicator */
	attach_to_master_set(my_id==0);

	/* This option is set to 0 when application is MPI and some processes are not running the  EARL , which is a mesh */
	if (synchronize_masters == 0){
		debug("EARL initialization aborted");
		return;
	}

	// if we are not the master, we return
#if ONLY_MASTER
	if (my_id != 0) {
		return;
	}
#endif
	
	debug("Executing EAR library IDs(%d,%d)", ear_my_rank, my_id);

	get_settings_conf_path(get_ear_tmp(),system_conf_path);
	get_resched_path(get_ear_tmp(),resched_conf_path);

	debug("system_conf_path  = %s",system_conf_path);
	debug("resched_conf_path = %s",resched_conf_path);

	system_conf = attach_settings_conf_shared_area(system_conf_path);
	resched_conf = attach_resched_shared_area(resched_conf_path);

	debug("system_conf  = %p", system_conf);
	debug("resched_conf = %p", resched_conf);
	if (system_conf != NULL) debug("system_conf->id = %u", system_conf->id);
	debug("create_ID() = %u", create_ID(my_job_id,my_step_id)); // id*100+sid

	/* Updating configuration */
	if ((system_conf != NULL) && (resched_conf != NULL) && (system_conf->id == create_ID(my_job_id,my_step_id))){
		debug("Updating the configuration sent by the EARD");
		update_configuration();	
	}else{
		eard_ok = 0;
		verbose_master(2,"Shared memory with EARD not present");
#if USE_LOCK_FILES
    if (!my_id){ 
    	debug("Application master releasing the lock %d %s", ear_my_rank,fd_lock_filename);
			file_unlock_master(fd_master_lock,fd_lock_filename);
		}
#endif
		/* Only the node master will notify the problem to the other masters */
		if (!my_id) notify_eard_connection(0);
	}	
	/* my_id is 0 for the master (for each node) and 1 for the others */
	/* eard_ok reflects the eard connection status, if eard_ok is 0, earl is disabled */

#if ONLY_MASTER
	if (my_id != 0) {
		return;
	}
#endif

	// Application static data and metrics
	debug("init application");
	init_application(&application);
	debug("init loop_signature");
	init_application(&loop_signature);
	application.is_mpi   	= 1;
	loop_signature.is_mpi	= 1;
	application.is_learning	= ear_whole_app;
	application.job.def_f		= getenv_ear_p_state();
	loop_signature.is_learning	= ear_whole_app;
	debug("application data initialized");

	// Getting environment data
	get_app_name(ear_app_name);
	debug("App name is %s",ear_app_name);
	if (application.is_learning){
		verbose_master(VEAR_INIT,"Learning phase app %s p_state %lu\n",ear_app_name,application.job.def_f);
	}
	if (getenv("LOGNAME")!=NULL) strcpy(application.job.user_id, getenv("LOGNAME"));
	else strcpy(application.job.user_id,"No userid");
	debug("User %s",application.job.user_id);
	strcpy(application.node_id, node_name);
	strcpy(application.job.user_acc,my_account);
	application.job.id = my_job_id;
	application.job.step_id = my_step_id;

	debug("Starting job");
	//sets the job start_time
	start_job(&application.job);
	debug("Job time initialized");

	verbose_master(2,"EARD connection section EARD=%d",eard_ok);
	if (!my_id && eard_ok){ //only the master will connect with eard
		verbose_master(2, "%sConnecting with EAR Daemon (EARD) %d node %s%s", COL_BLU,ear_my_rank,node_name,COL_CLR);
		if (eards_connect(&application) == EAR_SUCCESS) {
			debug("%sRank %d connected with EARD%s", COL_BLU,ear_my_rank,COL_CLR);
			notify_eard_connection(1);
			mark_as_eard_connected(my_job_id,my_step_id,getpid());
		}else{   
			eard_ok=0;
			notify_eard_connection(0);
		}
	}
	/* At this point, the master has detected EARL must be set to off , we must notify the others processes about that */

	debug("End EARD connection section");


	debug("Shared region creation section");

	if (!my_id){
		debug("%sI'm the master, creating shared regions%s",COL_BLU,COL_CLR);
		create_shared_regions();
	}else{
		attach_shared_regions();
	}
	if (!eard_ok){ 
		debug("%s: EARL off because of EARD",node_name);
		strcpy(application.job.policy," "); /* Cleaning policy */
		return;
	}
	debug("END Shared region creation section");
	/* Processes in same node connectes each other*/
	if (system_conf->user_type != AUTHORIZED) 	VERB_SET_LV(ear_min(verb_level,1));
 /* create verbose file for each job */
    if (verb_path != NULL){
        if (masters_info.my_master_rank >= 0){
            if (( mkdir (verb_path, S_IRWXU|S_IRGRP|S_IWGRP) < 0) && (errno != EEXIST)) {
                error("MR[%d] Verbose files folder cannot be created (%s)",masters_info.my_master_rank,strerror(errno));
            }else{
                using_verb_files = 1;
                char file_name[128];
                sprintf(file_name, "%s/earl_log.%d.%d.%d.%d", verb_path,lib_shared_region->master_rank,my_node_id, my_step_id, my_job_id);
                verbose_master(VEAR_INIT,"Using %s as earl log file",file_name);
                int fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP);
                if (fd != -1){
                    VERB_SET_FD(fd);
                    DEBUG_SET_FD(fd);
										ERROR_SET_FD(fd);
										WARN_SET_FD(fd);
                }else{
                    verbose_master(VEAR_INIT,"Log file cannot be created (%s)",strerror(errno));
                }
            }
        }
    }


    // Initializing architecture and topology
    if ((st=get_arch_desc(&arch_desc))!=EAR_SUCCESS){
        error("Retrieving architecture description");
        /* How to proceeed here ? */
				lib_shared_region->earl_on = 0;
				strcpy(application.job.policy," "); /* Cleaning policy */
        return;
    }

    // Initializing metrics
    debug("Starting metrics init ");

    if (metrics_init(&arch_desc.top) != EAR_SUCCESS) {
        error("in EAR metrics initialization (%s), setting EARL off", state_msg);
				eard_ok = 0;
				strcpy(application.job.policy," "); /* Cleaning policy */
        return;
    }

		if (!eard_ok){ 
			lib_shared_region->earl_on = 0;
			strcpy(application.job.policy," "); /* Cleaning policy */
			return;
		}

    debug("End metrics init");

    // Initializing DynAIS
    debug("Dynais init");
    dynais = dynais_init(&arch_desc.top, get_ear_dynais_window_size(), get_ear_dynais_levels());
    debug("Dynais end");

    // Policies && models
    arch_desc.max_freq_avx512=system_conf->max_avx512_freq;
    arch_desc.max_freq_avx2=system_conf->max_avx2_freq;

    // Frequencies
    ulong *freq_list;
    uint freq_count;
    // Getting frequencies from EARD
    debug("initializing frequencies");
    get_frequencies_path(get_ear_tmp(), app_mgt_path);
    debug("looking for frequencies in '%s'", app_mgt_path);
    freq_list = attach_frequencies_shared_area(app_mgt_path, (int *) &freq_count);
    freq_count = freq_count / sizeof(ulong);
    // Initializing API in user mode	: We should move this to library/management
    if (frequency_init_user(freq_list, freq_count) == EAR_SUCCESS){
			for (uint i=0;i<freq_count;i++){
					verbose_master(2,"Freq[%d] = %lu",i,freq_list[i]);
			}
			if ((st = mgt_cpufreq_get_api(&mgt_cpufreq_api)) == EAR_SUCCESS){
				char mgt_cpufreq_api_text[64];
				apis_tostr(mgt_cpufreq_api, mgt_cpufreq_api_text, sizeof(mgt_cpufreq_api_text));
				verbose_master(2,"mgt_cpufreq API %s",mgt_cpufreq_api_text);
			}
		
		}
    dettach_frequencies_shared_area();
    // You can utilize now the frequency API
    arch_desc.pstates = frequency_get_num_pstates();
    // Other frequency things
    if (ear_my_rank == 0) {
        if (ear_whole_app == 1 && ear_use_turbo == 1) {
            verbose_master(VEAR_INIT, "turbo learning phase, turbo selected and start computing");
            if (!my_id) eards_set_turbo();
        } else {
            verbose_master(VEAR_INIT, "learning phase %d, turbo %d", ear_whole_app, ear_use_turbo);
        }
    }

    // This is not frequency any more
    if (masters_info.my_master_rank>=0){
#if POWERCAP
        get_app_mgt_path(get_ear_tmp(),app_mgt_path);
        verbose_master(VEAR_INIT,"app_mgt_path %s",app_mgt_path);
        app_mgt_info=attach_app_mgt_shared_area(app_mgt_path);
        if (app_mgt_info==NULL){
            error("Application shared area not found");
            eard_ok = 0;
						lib_shared_region->earl_on = 0;
						strcpy(application.job.policy," "); /* Cleaning policy */
            return;
        }
        fill_application_mgt_data(app_mgt_info);
        /* At this point we have notified the EARD the number of nodes in the application */
        check_large_job_use_case();
        /* This area is RW */
        get_pc_app_info_path(get_ear_tmp(),pc_app_info_path);
        verbose_master(VEAR_INIT,"pc_app_info path %s",pc_app_info_path);
        pc_app_info_data=attach_pc_app_info_shared_area(pc_app_info_path);
        if (pc_app_info_data==NULL){
            error("pc_application_info area not found");
            eard_ok = 0;
						lib_shared_region->earl_on = 0;
						strcpy(application.job.policy," "); /* Cleaning policy */
            return;

        }
#endif
    }
    if (is_affinity_set(&arch_desc.top,getpid(),&ear_affinity_is_set,&ear_process_mask)!=EAR_SUCCESS){
        error("Checking the affinity mask");
    }else{
        /* Copy mask in shared memory */
        if (ear_affinity_is_set){ 
            sig_shared_region[my_node_id].cpu_mask = ear_process_mask;
            sig_shared_region[my_node_id].affinity = 1;
#if 0
            if (masters_info.my_master_rank>=0){ 
                print_affinity_mask(&arch_desc.top);
            }
#endif
        }else{ 
            sig_shared_region[my_node_id].affinity = 0;
            verbose_master(2,"Affinity mask not defined for rank %d",masters_info.my_master_rank);
        } 
    } 



#ifdef SHOW_DEBUGS
    print_arch_desc(&arch_desc);
#endif

    debug("init_power_policy");
    init_power_policy(system_conf,resched_conf);
    debug("init_power_models");
    init_power_models(system_conf->user_type,&system_conf->installation,&arch_desc);
    verbose_master(VEAR_INIT,"Policies and models initialized");	

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

    summary_pathname = get_ear_user_db_pathname();
		if (masters_info.my_master_rank >= 0){
		// Report API
    char my_plug_path[512];
    utils_create_report_plugin_path(my_plug_path, system_conf->installation.dir_plug, getenv(SCHED_EARL_INSTALL_PATH), system_conf->user_type);

			if (summary_pathname == NULL){
				if (report_load(my_plug_path,"eard.so") != EAR_SUCCESS){
					verbose_master(0,"Error in reports API load ");
				}
			}else{
				setenv("EAR_USER_DB_PATHNAME",summary_pathname,1);
				if (report_load(my_plug_path,"eard.so:csv_ts.so") != EAR_SUCCESS){
					verbose_master(0,"Error in reports API load ");
				}
			}
			if (report_init() != EAR_SUCCESS){
        verbose_master(0,"Error in reports API initialization ");
      }
		}

    // States
    states_begin_job(my_id,  ear_app_name);
		#if 0
    // Summary files

    if (summary_pathname != NULL) {
        sprintf(app_summary_path, "%s%s.csv", summary_pathname, node_name);
        sprintf(loop_summary_path, "%s%s.loop_info.csv", summary_pathname, node_name);
    }
		#endif

    // Print summary of initialization
    print_local_data();
    // ear_print_lib_environment();
    fflush(stderr);

    // Tracing init
    if (masters_info.my_master_rank>=0){
        traces_init(system_conf,application.job.app_id,masters_info.my_master_rank, my_id, num_nodes, my_size, ppnode);

        traces_start();
        traces_frequency(ear_my_rank, my_id, EAR_default_frequency);
        traces_stop();

        traces_mpi_init();
    }

    ear_lib_initialized=1;

    // All is OK :D
    if (masters_info.my_master_rank==0) {
        verbose_master(1, "EAR initialized successfully ");
    }
    if (monitor_init() != EAR_SUCCESS) verbose_master(2,"Monitor init fails! %s",state_msg);
    earl_monitor = suscription();
    earl_monitor->call_init = earl_periodic_actions_init;
    earl_monitor->call_main = earl_periodic_actions;
    earl_monitor->time_relax = 10000;
    earl_monitor->time_burst = 1000;
    if (monitor_register(earl_monitor) != EAR_SUCCESS) verbose_master(2,"Monitor register fails! %s",state_msg);
    if (is_mpi_enabled()) monitor_relax(earl_monitor);
#if USE_GPUS
    if (is_cuda_enabled()){
        ear_cuda_init();
    }
#endif
}


/**************************************** ear_finalize ************************/

void ear_finalize()
{
    // if we are not the master, we return
    if (synchronize_masters == 0 ) return;
#if ONLY_MASTER
    if (my_id) {
        return;
    }	
#endif
    if (!eard_ok) return;
		if (!lib_shared_region->earl_on) return;
#if USE_LOCK_FILES
    if (!my_id){
        debug("Application master releasing the lock %d %s", ear_my_rank,fd_lock_filename);
        file_unlock_master(fd_master_lock,fd_lock_filename);
        mark_as_eard_disconnected(my_job_id,my_step_id,getpid());
    }
#endif

    monitor_unregister(earl_monitor);
    monitor_dispose();
    if (masters_info.my_master_rank>=0){
        verbose_master(2,"Stopping periodic earl thread");	
        verbose_master(2,"Stopping tracing");	
        traces_stop();
        traces_end(ear_my_rank, my_id, 0);

        traces_mpi_end();
    }

    // Closing and obtaining global metrics
    verbose_master(2,"metrics dispose");
    dispose=1;
    verbose_master(2,"Total resources computed %d",get_total_resources());
    metrics_dispose(&application.signature, get_total_resources());
    dynais_dispose();
    if (!my_id) frequency_dispose();

    // Writing application data
    
    if (!my_id) 
    {
				if (application.is_learning) verbose_master(1,"Application in learning phase mode");
        debug("Reporting application data");
        verbose_master(2,"Filtering DC node power with limit %.2lf",system_conf->max_sig_power);
        clean_db_signature(&application.signature, system_conf->max_sig_power);
				#if 0
        eards_write_app_signature(&application);
				#endif
				if (report_applications(&application,1) != EAR_SUCCESS){
					verbose_master(0,"Error reporting application");
				}
				#if 0
        append_application_text_file(app_summary_path, &application, 1);
				#endif
        report_mpi_application_data(&application);
    }
    // Closing any remaining loop
    if (loop_with_signature) {
        debug("loop ends with %d iterations detected", ear_iterations);
    }
    if (in_loop) {
        states_end_period(ear_iterations);
    }
    states_end_job(my_id,  ear_app_name);

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
    if (synchronize_masters == 0 ) return;
		if (!lib_shared_region->earl_on) return;
#if ONLY_MASTER
    if (my_id) return;
#endif
    if (ear_guided == TIME_GUIDED) return;
    /* The learning phase avoids EAR internals ear_whole_app is set to 1 when learning-phase is set */
    if (!ear_whole_app)
    {
        unsigned long  ear_event_l = (unsigned long)((((buf>>5)^dest)<<5)|call_type);
        //unsigned short ear_event_s = dynais_sample_convert(ear_event_l);
        if (masters_info.my_master_rank>=0){
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
                            if (check_periodic_mode == 0){  // check_periodic_mode=0 the first time a signature is computed
                                ear_mpi_call_dynais_on(call_type,buf,dest);
                            }else{ // Check every N=check_every mpi calls
                                /* Check here if we must move to periodic mode, do it every N mpicalls to reduce the overhead */
                                if ((total_mpi_calls%check_every) == 0){
                                    time(&curr_time);
                                    time_from_mpi_init = difftime(curr_time,application.job.start_time);

                                    /* In that case, the maximum time without signature has been passed, go to set ear_periodic_mode ON */
                                    if (time_from_mpi_init >= dynais_timeout){
                                        // we must compute N here
                                        ear_periodic_mode = PERIODIC_MODE_ON;
                                        assert(dynais_timeout > 0);
                                        mpi_calls_per_second = (uint)(total_mpi_calls/dynais_timeout);
                                        if (masters_info.my_master_rank >= 0) traces_start();
                                        verbose_master(3,"Going to periodic mode after %lf secs: mpi calls in period %u. ear_status = %hu\n",
                                                time_from_mpi_init,mpi_calls_in_period,ear_status);
                                        ear_iterations = 0;
                                        states_begin_period(my_id, ear_event_l, (ulong) lib_period,1);
                                        states_new_iteration(my_id, 1, ear_iterations, 1, 1, lib_period,0);
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
                if ((mpi_calls_per_second > 0 ) && (total_mpi_calls%mpi_calls_per_second) == 0){
                    ear_iterations++;
                    states_new_iteration(my_id, 1, ear_iterations, 1, 1,lib_period,0);
                }
                break;
        }
    }
#else
    if (dynais_enabled == DYNAIS_ENABLED) ear_mpi_call_dynais_on(call_type,buf,dest);
    else ear_mpi_call_dynais_off(call_type,buf,dest);
#endif
}



void ear_mpi_call_dynais_on(mpi_call call_type, p2i buf, p2i dest)
{
#if ONLY_MASTER
    if (my_id) {
        return;
    }
#endif
    // If ear_whole_app we are in the learning phase, DyNAIS is disabled then
    if (!ear_whole_app)
    {
        // Create the event for DynAIS
        ulong ear_event_l;
        uint ear_event_s;
        uint ear_size;
        uint ear_level;

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
                //debug("NEW_LOOP event %lu level %u size %u\n", ear_event_l, ear_level, ear_size);
                ear_iterations=0;
                states_begin_period(my_id, ear_event_l, (ulong) ear_size, (ulong) ear_level);
                ear_loop_size=(uint)ear_size;
                ear_loop_level=(uint)ear_level;
                in_loop=1;
                mpi_calls_per_loop=1;
                break;
            case END_NEW_LOOP:
                //debug("END_LOOP - NEW_LOOP event %lu level %u\n", ear_event_l, ear_level);
                if (loop_with_signature) {
                    //debug("loop ends with %d iterations detected", ear_iterations);
                }

                loop_with_signature=0;
                if (masters_info.my_master_rank>=0) traces_end_period(ear_my_rank, my_id);
                states_end_period(ear_iterations);
                ear_iterations=0;
                mpi_calls_per_loop=1;
                ear_loop_size=(uint)ear_size;
                ear_loop_level=(uint)ear_level;
                states_begin_period(my_id, ear_event_l, (ulong) ear_size, (ulong) ear_level);
                break;
            case NEW_ITERATION:
                ear_iterations++;

                if (loop_with_signature)
                {
                    //debug("new iteration detected for level %u, event %lu, size %u and iterations %u",
                    //		  ear_loop_level, ear_event_l, ear_loop_size, ear_iterations);
                }

                if (masters_info.my_master_rank>=0) traces_new_n_iter(ear_my_rank, my_id, ear_event_l, ear_loop_size, ear_iterations);
                states_new_iteration(my_id, ear_loop_size, ear_iterations, ear_loop_level, ear_event_l, mpi_calls_per_loop,1);
                mpi_calls_per_loop=1;
                break;
            case END_LOOP:
                //debug("END_LOOP event %lu\n",ear_event_l);
                if (loop_with_signature) {
                    //debug("loop ends with %d iterations detected", ear_iterations);
                }
                loop_with_signature=0;
                if (masters_info.my_master_rank>=0)  traces_end_period(ear_my_rank, my_id);
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
        unsigned short ear_level;
        ear_level=0;

        ear_event_l = (unsigned long)((((buf>>5)^dest)<<5)|call_type);
        //ear_event_s = dynais_sample_convert(ear_event_l);

        //debug("EAR(%s) EAR executing before an MPI Call: DYNAIS ON\n", __FILE__);

        if (masters_info.my_master_rank>=0) traces_mpi_call(ear_my_rank, my_id,
                (unsigned long) buf,
                (unsigned long) dest,
                (unsigned long) call_type,
                (unsigned long) ear_event_l);

        mpi_calls_per_loop++;

        if (last_calls_in_loop == mpi_calls_per_loop){
            ear_status = NEW_ITERATION;
        }else{
            ear_status = IN_LOOP;
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

                if (masters_info.my_master_rank>=0) traces_new_n_iter(ear_my_rank, my_id, ear_event_l, ear_loop_size, ear_iterations);
                states_new_iteration(my_id, ear_loop_size, ear_iterations, (uint)ear_level, ear_event_l, mpi_calls_per_loop,1);
                mpi_calls_per_loop=1;
                break;
            default:
                break;
        }
    } //ear_whole_app
}

#endif

/************************** MANUAL API *******************/

static unsigned long manual_loopid=0;
unsigned long ear_new_loop()
{
    if (my_id)  return 0;
    manual_loopid++;
    if (!ear_whole_app)
    {
        debug("New loop reported");
        switch (ear_status){
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
                if (masters_info.my_master_rank>=0) traces_end_period(ear_my_rank, my_id);
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
                    if (masters_info.my_master_rank>=0) traces_end_period(ear_my_rank, my_id);
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
                if (masters_info.my_master_rank>=0) traces_new_n_iter(ear_my_rank, my_id, loop_id, 1, ear_iterations);
                states_new_iteration(my_id, 1, ear_iterations, 1, loop_id, mpi_calls_per_loop,1);
                mpi_calls_per_loop=1;
                break;
            case NO_LOOP:
                debug("Error, application is not in a loop");
                break;
        }
    }
}


/**************** New for iterative code ********************/
//void *earl_periodic_actions(void *no_arg)
static uint last_mpis;
static timestamp_t periodic_time;
static ulong last_check = 0;
static uint first_exec_monitor = 1;

state_t earl_periodic_actions_init(void *no_arg)
{
    verbose_master(2,"RANK %d EARL periodic thread ON",ear_my_rank);
    timestamp_getfast(&periodic_time);
    seconds = 0;
    last_mpis = 0;
    if (!is_mpi_enabled()){
        ear_periodic_mode = PERIODIC_MODE_ON;
        if (masters_info.my_master_rank>=0) traces_start();
        ear_iterations = 0;
        states_begin_period(my_id, 1, (ulong) lib_period,1);
        mpi_calls_in_period = lib_period;
        if (!ear_whole_app) states_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period,0);
    }

    return EAR_SUCCESS;
}
state_t earl_periodic_actions(void *no_arg)
{
    uint mpi_period;
		float mpi_in_period;
    ulong period_elapsed;
    timestamp_t curr_periodic_time;
    uint new_ear_guided = ear_guided;

		if (ear_whole_app) return EAR_SUCCESS;


    ear_iterations++;
    timestamp_getfast(&curr_periodic_time);
    period_elapsed = timestamp_diff(&curr_periodic_time, &periodic_time,TIME_SECS);
		debug("elapsed %lu : first_exec_monitor %d Iterations %d MPI %d ear_guided %d",period_elapsed,first_exec_monitor,ear_iterations,is_mpi_enabled(),ear_guided);
		if (first_exec_monitor){
			first_exec_monitor = 0;
			return EAR_SUCCESS;
		}
    if (!is_mpi_enabled() || (ear_guided == TIME_GUIDED)){
        if (ear_guided == TIME_GUIDED) debug("%lu: Time guided iteration iters %d period %u",period_elapsed,ear_iterations,lib_period);
        states_new_iteration(my_id, 1, ear_iterations, 1, 1,lib_period,0);
    }
    seconds = period_elapsed - last_check;
    if (seconds >= 5){ 
        last_check = period_elapsed;
				/* total_mpi_calls is computed per process in this file, not in metrics. It is used to automatically switch to time_guided */
        mpi_period = total_mpi_calls - last_mpis;
        last_mpis = total_mpi_calls;	
        if (mpi_period) mpi_in_period = (float)mpi_period/(float)seconds;
				else mpi_in_period = 0;
				/* IO metrics are now computed in metrics.c as part of the signature */
				if (must_switch_to_time_guide(mpi_in_period, &new_ear_guided) != EAR_SUCCESS){
					verbose_master(2,"Error estimating dynais/time guided");
				}
        debug("%d:Phase time_guided %d dynais_guided %d period %u",my_node_id,new_ear_guided==TIME_GUIDED, new_ear_guided==DYNAIS_GUIDED,lib_period);
        if (new_ear_guided != DYNAIS_GUIDED){
            ear_periodic_mode = PERIODIC_MODE_ON;
            if (ear_guided != new_ear_guided){ 
                monitor_burst(earl_monitor);
                verbose_master(2,"%d:Dynais guided --> time guided at time %lu",my_node_id,period_elapsed);
								mpi_calls_in_period = lib_period;
                states_begin_period(my_id, 1, (ulong) lib_period,1);
            }
        }
        ear_guided = new_ear_guided;
    }
    return EAR_SUCCESS;
}


/************** Constructor & Destructor for NOt MPI versions **************/
#if !MPI 
void ear_constructor()
{
    debug("Calling ear_init in ear_constructor %d",getpid());
    ear_init();
}
void ear_destructor()
{
    debug("Calling ear_finalize in ear_destructor %d",getpid());
#if ONLY_MASTER
    if (my_id) {
        return;
    }
#endif
    ear_finalize();
}
#else
void ear_constructor()
{
}
void ear_destructor()
{
}
#endif


