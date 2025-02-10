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
//#define SHOW_DEBUGS 1
//
#define VPROC_INIT 3
#define VJOB_INIT  2
#define VPROC_MGT  3
#define VNTASKS_INIT 3

/* 1 means invatid shared memory or invalid id (all processes), 
 * 2 means invatid shared memory or invalid id (half processes),
 * 3 eard not present . 0 means no fake (default) */
#define FAKE_EARD_ERROR "FAKE_EARD_ERROR"
//#define FAKE_LEARNING 1

/* 1 means master error, 2 means no master error at ear_finalize i. 0 no fake (default)*/
#define FAKE_PROCESS_ERRROR "FAKE_PROCESS_ERRROR"

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
#include <signal.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include <common/config.h>
#include <common/config/config_env.h>
#include <common/colors.h>
#include <common/utils/overhead.h>
#include <common/system/folder.h>
#include <common/system/lock.h>
#include <common/system/version.h>
#include <common/environment.h>
#include <library/common/verbose_lib.h>
#include <common/types/application.h>
#include <common/types/version.h>
#include <common/types/pc_app_info.h>
#include <management/cpufreq/frequency.h>
#include <common/hardware/hardware_info.h>
#include <common/system/monitor.h>
#include <common/system/execute.h>
#include <common/utils/sched_support.h>

#include <library/common/externs_alloc.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/common/library_shared_data.h>
#include <library/common/utils.h>
#include <library/api/clasify.h>
#include <library/api/eard_dummy.h>
#include <library/api/mpi_support.h>
#include <library/loader/module_mpi.h>
#include <library/dynais/dynais.h>
#include <library/tracer/tracer.h>
#include <library/states/states.h>
#include <library/states/states_comm.h>
#include <library/models/cpu_power_model.h>
#include <library/policies/policy.h>
#include <library/metrics/metrics.h>

#include <library/policies/common/mpi_stats_support.h>

#include <daemon/local_api/eard_api.h>
#include <daemon/local_api/node_mgr.h>
#include <daemon/app_mgt.h>
#include <daemon/shared_configuration.h>

#include <metrics/io/io.h>
#include <metrics/proc/stat.h>

#include <report/report.h>

#ifdef USE_GPUS
#include <library/api/cupti.h>
#endif

// Verbose levels
#define EARL_GUIDED_LVL 2

#if MPI
#include <mpi.h>
#include <library/api/mpi.h>
#endif
#include <library/api/mpi_support.h>
#include <library/policies/common/mpi_stats_support.h>

#include <common/external/ear_external.h>

#include <library/metrics/dlb_talp_lib.h>
#include <library/metrics/fine_grain_collect.h>

/* Pretty verbose which shows the node name and the TID of the calling process.
 * Must be used after setting global variables 'node_name' and 'main_pid'. */
#define earl_early_verb(lvl, msg, ...) do {\
	verbose(lvl, "[EARL][%s][%d] " msg, node_name, main_pid, ##__VA_ARGS__);\
} while (0);


char shexternal_region_path[GENERIC_NAME];
ear_mgt_t *external_mgt;

void ear_finalize(int exit_status);
static uint cancelled = 0;
/* This var means if the appid has to report with report_application or report_misc(WF_APPLICATION) */
static uint first_application = 1;

static pthread_mutex_t earl_th_locks = PTHREAD_MUTEX_INITIALIZER;

#define MASTER_ID masters_info.my_master_rank

extern const char *__progname;
static uint ear_lib_initialized=0;
/* This is the unique Application ID */
uint   AID;
#define WF_SUPPORT_VERB 2

// Statics
#define JOB_ID_OFFSET		100
pthread_t earl_periodic_th;
unsigned long ext_def_freq=0;
int dispose=0;

#include <common/system/file.h>
static char master_lock_filename[MAX_PATH_SIZE];
static int fd_master_lock = -1;

// Process information
static int my_id = 1;
static int ear_my_size;
static int app_node_multiprocess = 0;
static int num_nodes;
static int ppnode;

sem_t *lib_shared_lock_sem;

/* To be used by report plugins */
report_id_t rep_id;
cluster_conf_t cconf;

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


static pid_t main_pid;

uint report_earl_events = 0;
static char *creport_earl_events;


/* in us */
// These variables are shared
uint 		ear_periodic_mode = PERIODIC_MODE_OFF;
uint 	mpi_calls_in_period = 10000;
uint mpi_calls_per_second;
static unsigned long long int total_mpi_calls = 0;
static uint  dynais_timeout = MAX_TIME_DYNAIS_WITHOUT_SIGNATURE;
uint             lib_period = PERIOD;

// MPI_CALLS_TO_CHECK_PERIODIC is defined to MAX_MPI_CALLS_SECOND
static uint     check_every = MPI_CALLS_TO_CHECK_PERIODIC;
#define MASTERS_SYNC_VERBOSE 1
masters_info_t masters_info;
#if MPI
MPI_Comm new_world_comm,masters_comm;
#endif
int masters_connected=0;
unsigned masters_comm_created=0;
mpi_information_t *per_node_mpi_info, *global_mpi_info;
char lib_shared_region_path[GENERIC_NAME];
char lib_shared_region_path_jobs[GENERIC_NAME];
/* This lock is used to access the lib_shared_region shared memory */
int  lib_shared_lock_fd = -1;
char sig_shared_region_path_jobs[GENERIC_NAME];
char dummy_areas_path[GENERIC_NAME];
int fd_libsh, fd_sigsh;
char shsignature_region_path[GENERIC_NAME];
char block_file[GENERIC_NAME];
char sem_file[GENERIC_NAME];
float ratio_PPN = 1.0;
app_mgt_t *app_mgt_info = NULL;
char app_mgt_path[GENERIC_NAME];
pc_app_info_t *pc_app_info_data = NULL;
char pc_app_info_path[GENERIC_NAME];

char ser_cluster_conf_path[GENERIC_NAME];
char *ser_cluster_conf;
size_t ser_cluster_conf_size;
cluster_conf_t cconf;

/* Info with jobs sharing the node */
ear_njob_t *node_mgr_data;
uint node_mgr_index;
uint node_mgr_earl_index;
node_mgr_sh_data_t *node_mgr_job_info;

// Dynais
static dynais_call_t dynais;

suscription_t *earl_monitor;
state_t earl_periodic_actions(void *no_arg);
state_t earl_periodic_actions_init(void *no_arg);

static ulong seconds = 0;

#if EAR_DEF_TIME_GUIDED
uint ear_guided = TIME_GUIDED;
#else
uint ear_guided = DYNAIS_GUIDED; 
#endif
//static int end_periodic_th=0;
static uint synchronize_masters = 1;
static uint ITERS_PER_PERIOD;
#define RELAX 0
#define BURST 1

static const uint lib_supports_gpu = USE_GPUS; // Library supports GPU

/*  Sets verbose level for ear init messages */
#define VEAR_INIT 2

#define MAX_TRIES 1

uint using_verb_files = 0;
uint per_proc_verb_file = 0;

uint exclusive = 1;
ear_classify_t phases_limits;
static ulong dynais_calls = 0;
uint limit_exceeded = 0;

extern ulong perf_accuracy_min_time;
static uint exiting = 0;


/* Checks whether the job-step folder was created.
 * This folder should be created by EARD (?).
 * If not, try to create it.
 */
static state_t check_job_folder();


static uint must_masters_synchronize();

static void create_earl_verbose_files();


/** Make a copy of pathname and store it to a file called pathname.bckp */
static state_t earl_log_backup(char *pathname, size_t pathname_size);

static state_t check_eard_earl_compatibility(state_t eard_connect_state)
{
	state_t st = EAR_SUCCESS;
	return st;
	if (state_fail(eard_connect_state)) return EAR_ERROR;

	eard_state_t eard_conf;

#if VERSION_MAJOR < 5 
	verbose_master(0, "Warning, EARD compatibility test not supported");
	return EAR_SUCCESS;
#endif

	// If eards_get_state returns EAR_ERROR, means that the connected EARD does not have the RPC
	// implemented, so we assume it is an old version and (by now) it should be compatible.
	// This is why currently if the below call returns ERROR, we return EAR_SUCCESS
	if (state_ok(st = eards_get_state(&eard_conf)))
	{
		if (eard_conf.version.major != VERSION_MAJOR)
		{
			verbose_warning_master("Major versions differ (EARD %u | EARL %u). Future versions shall consider this difference as an incompatibility.", eard_conf.version.major, VERSION_MAJOR);
		}
		if ((eard_conf.gpus_supported_count != MAX_GPUS_SUPPORTED) ||
				(eard_conf.cpus_supported_count != MAX_CPUS_SUPPORTED) ||
				(eard_conf.sock_supported_count != MAX_SOCKETS_SUPPORTED) ||
				(eard_conf.gpus_support_enabled != USE_GPUS))
		{
			st = EAR_ERROR;
		}
	}else st = EAR_SUCCESS; /* If eards_get_state is not implemented, there is a possibility that it's not compatible */
	return st;
}

static void print_local_data()
{
    char ver[64];
    if (masters_info.my_master_rank == 0 || using_verb_files) {
        version_to_str(ver);
#if MPI
        verbose(1, "---------------- EAR %s MPI enabled ----------------",ver);
#else
        verbose(1, "-------------- EAR %s MPI not enabled --------------",ver);
#endif
        verbose(1, "%30s: '%s'/'%s'", "App/user id", application.job.app_id,
                application.job.user_id);
        verbose(1, "%30s: '%s'/'%lu'/'%lu'/'%u'", "Node/job id/step_id/app_id",
                application.node_id, application.job.id,
                application.job.step_id, AID);
        //verbose(2, "App/loop summary file: '%s'/'%s'", app_summary_path, loop_summary_path);
        verbose(1, "%30s: %u/%lu (%d)", "P_STATE/frequency (turbo)",
                EAR_default_pstate, application.job.def_f, ear_use_turbo);
        verbose(1, "%30s: %u/%d/%d/%.3f", "Tasks/nodes/ppn/node_ratio", ear_my_size, num_nodes, ppnode, ratio_PPN);
        verbose(1, "%30s: %s (%d)", "Policy (learning)",
                application.job.policy, ear_whole_app);
        verbose(1, "%30s: %0.2lf/%0.2lf", "Policy threshold/Perf accuracy",
                application.job.th, get_ear_performance_accuracy());
        verbose(1, "%30s: %d/%d/%d", "DynAIS levels/window/AVX512",
                get_ear_dynais_levels(), get_ear_dynais_window_size(),
                dynais_build_type());
        verbose(1, "%30s: %s", "VAR path", get_ear_tmp());
        verbose(1, "%30s: %u", "EAR privileged user",
                system_conf->user_type==AUTHORIZED);
        verbose(1, "%30s: %d ", "EARL on", lib_shared_region->earl_on);
        verbose(1, "%30s: %lu", "Max CPU freq", system_conf->max_freq);
        verbose(1, "-------------------------------------------------------");
    }
}


/* notifies mpi process has succesfully connected with EARD */
/*********************** This function synchronizes all the node masters to guarantee all the nodes have connected with EARD, otherwise, EARL is set to OFF **************/
void notify_eard_connection(int status)
{
				char *buffer_send;
				char *buffer_recv;
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
								verbose_master(2, "[%s] Synchronizing with other masters. Status: %d. Waiting for %d masters", node_name, status, masters_info.my_master_size);
								masters_connected=0;
								if (synchronize_masters){
												PMPI_Allgather(buffer_send, 1, MPI_BYTE, buffer_recv, 1, MPI_BYTE, masters_info.masters_comm);
												for (int i = 0; i < masters_info.my_master_size; ++i)
												{       
																masters_connected+=(int)buffer_recv[i];
												}
                        int master_pid = getpid();
                        PMPI_Bcast(&master_pid, 1, MPI_INT, 0 , masters_info.masters_comm );
                        verbose(VPROC_INIT,"PID for master is %d", master_pid);
								}
#else
								masters_connected=1;
#endif


								if (masters_connected != masters_info.my_master_size){
												/* Some of the nodes is not ok , setting off EARL */
												if (masters_info.my_master_rank>=0) {
																verbose_master(1,"%s:some EARD is not running (connected nodes %d expected %d)\n",node_name, masters_connected, masters_info.my_master_size);
												}
												eard_ok = 0;
												return;
								}else{
												if (masters_connected!=num_nodes){
																num_nodes=masters_connected;
																set_ear_num_nodes(num_nodes);
																ppnode = ear_my_size / num_nodes;
												}
								}
								return;
				}
}

static uint clean_job_folder = 0;

static state_t check_job_folder()
{
	char block_file_dir[MAX_PATH_SIZE];
	char *tmp = get_ear_tmp();
	uint ID = create_ID(my_job_id, my_step_id);
	int ret;

	snprintf(block_file_dir, sizeof(block_file_dir), "%s/%u", tmp, ID);

	if (access(block_file_dir, W_OK) == 0)
	{
		verbose(3, "JOB folder (%s) exists and is writeable.", block_file_dir);
		return EAR_SUCCESS;
	}

	if (errno == ENOENT)
	{
		verbose(1, "Job folder (%s) doesn't exist (%s), creating it...", block_file_dir, strerror(errno));
		// clean_job_folder = 1; // We can't remove the job folder since we support workflows.

		ret = mkdir(block_file_dir, S_IRWXU);
		if (ret >= 0 || errno == EEXIST)
		{
			debug("Folder created or already exists.");
			return EAR_SUCCESS;
		} else
		{
			/* What can we do here EAR OFF */
			error_lib("Job folder cannot be created: %s", strerror(errno));
			return EAR_ERROR;
		}
	} else
	{
		return EAR_ERROR;
	}
}


static uint must_masters_synchronize()
{
	uint return_val = synchronize_masters; // Default value
#if MPI
	char *is_erun = ear_getenv(SCHED_IS_ERUN);

	/* If OpenMPI, we don't synchronize unless all the processes answer the gather. */
	if (module_mpi_is_enabled() && module_mpi_is_open())
	{
		int n = openmpi_num_nodes();
		int sched_num_nodes = utils_get_sched_num_nodes();

		if ((n > 0) && (n != sched_num_nodes))
		{
			if (is_erun == NULL || ((is_erun != NULL) && atoi(is_erun) == 0))
			{
				return_val = 0;
			}
			
			debug("OpenMPI num nodes detected %d - Sched num nodes detected %d", n, sched_num_nodes);
		}
	}
#endif // MPI
	
	return return_val;
}

// TODO: Since EARL provides workflows support, an application can't delete a
// job-step folder because it might be used by another application.
// As a future work, the application can detect whether it is alone and delete
// this folder. This can be implemented savely with semaphores because if another
// application of the workflow is created later, it can create again the job-step
// folder.
void cleanup_job_folder()
{
				char block_file_dir[MAX_PATH_SIZE];
				char *tmp=get_ear_tmp();
				uint ID = create_ID(my_job_id,my_step_id);
				snprintf(block_file_dir,sizeof(block_file_dir),"%s/%u",tmp, ID);
				debug("cleanup_job_folder: %s process %d\n", block_file_dir, clean_job_folder);
				verbose_master(WF_SUPPORT_VERB,"EARL[%d] removing JOB folder (%s) ",getpid(), block_file_dir);
				if (clean_job_folder){
								debug("Folder needs to be removed from the library\n");
								folder_remove(block_file_dir);
				}else{ debug("Folder doesn't need to be removed from the library\n");}
}

/*
 *
 * Functions for WF support
 *
 */

static char EID_application_path[MAX_PATH_SIZE];
static char EID_application_path_finish[MAX_PATH_SIZE];
void clean_eid_folder()
{
  while (node_mgr_info_lock() != EAR_SUCCESS){};
  verbose(WF_SUPPORT_VERB,"EID folder (%s) renamed to %s", EID_application_path, EID_application_path_finish);
  folder_rename(EID_application_path, EID_application_path_finish);
	/* Should we remove the folder ?*/
	verbose(WF_SUPPORT_VERB,"EID folder (%s) deleted", EID_application_path_finish);
	folder_remove(EID_application_path_finish);
  node_mgr_info_unlock();
  verbose(WF_SUPPORT_VERB,"EID folder released");
}

/* This function creates the new application schema tmp/ID/AIS */
state_t create_eid_folder(char *tmp, int jid, int sid, uint AID)
{
  int ret;

  verbose(WF_SUPPORT_VERB+1,"EID folder based on tmp=%s jid %d sid %d aid %u", tmp, jid, sid, AID);

  while (node_mgr_info_lock() != EAR_SUCCESS){};

	/* Application folder for WF support */
  eid_folder(EID_application_path, sizeof(EID_application_path), tmp, jid, sid, AID);

  strcpy(EID_application_path_finish, EID_application_path);
  strcat(EID_application_path,".app");

  verbose(WF_SUPPORT_VERB+1, "EARL: Creating app folder %s", EID_application_path);
  ret = mkdir(EID_application_path, S_IRWXU);
  if ((ret < 0 ) && (errno != EEXIST)){
    verbose(WF_SUPPORT_VERB+1,"EAR error EID folder cannot be created (%s) (%s)", EID_application_path, strerror(errno));
    node_mgr_info_unlock();
    return EAR_ERROR;
  }
  chmod(EID_application_path,S_IRUSR|S_IWUSR|S_IXUSR);
  verbose(WF_SUPPORT_VERB+1,"EID folder (%s) created", EID_application_path);

	/* Application folder */

	ret = mkdir(EID_application_path_finish, S_IRWXU);
	if ((ret < 0 ) && (errno != EEXIST)){
		verbose(WF_SUPPORT_VERB,"EAR error AID folder cannot be created (%s) (%s)", EID_application_path_finish, strerror(errno));
    node_mgr_info_unlock();
    return EAR_ERROR;
	}
	chmod(EID_application_path_finish, S_IRUSR|S_IWUSR|S_IXUSR);
	verbose(WF_SUPPORT_VERB+1,"AID folder (%s) created", EID_application_path_finish);
	
  node_mgr_info_unlock();
  return EAR_SUCCESS;
}

void mark_as_eard_connected(char *tmp, int jid,int sid,uint aid, int pid)
{
    char EID_path[MAX_PATH_SIZE];
    int fd;
    char pid_str[128];
   
    snprintf(pid_str, sizeof(pid_str), "/.master.%d", pid);
    strcpy(EID_path, EID_application_path);
    strcat(EID_path, pid_str);
    
    //verbose(WF_SUPPORT_VERB,"Creating %s",EID_path);
    fd = open(EID_path,O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR);
    close(fd);
    return;
}
uint is_already_connected(char *tmp, int jid,int sid,uint aid, int pid)
{
    char EID_path[MAX_PATH_SIZE];
    char pid_str[128];
    int fd;

    snprintf(pid_str, sizeof(pid_str), "/.master.%d", pid);
    strcpy(EID_path, EID_application_path);
    strcat(EID_path, pid_str);
    
    debug("Looking for %s ",EID_path);
    fd = open(EID_path,O_RDONLY);
    if (fd >= 0){
        close(fd);
        return 1;
    }
    return 0;
}

void mark_as_eard_disconnected(char *tmp, int jid,int sid,uint aid, int pid)
{
    char EID_path[MAX_PATH_SIZE];
    char pid_str[128];

    snprintf(pid_str, sizeof(pid_str), "/.master.%d", pid);
    strcpy(EID_path, EID_application_path);
    strcat(EID_path, pid_str);

    debug("Removing %s",EID_path);
    unlink(EID_path);
}




/****************************************************************************************************************************************************************/
/****************** This function creates the shared memory region to coordinate processes in same node and with other nodes  ***********************************/
/****************** It is executed by the node master ***********************************************************************************************************/
/****************************************************************************************************************************************************************/
void create_shared_regions()
{
	char *tmp=get_ear_tmp();
  int total_elements = 1;
  int per_node_elements = 1;
  uint current_jobs_in_node;
#if MPI == 0
	char path_barrier[1024];
#endif
#if SHOW_DEBUGS
  int total_size;
#endif
  uint ID = create_ID(application.job.id, application.job.step_id);


  /* Semaphore based on AID and node */
  xsnprintf(sem_file, sizeof(sem_file), "ear_semaphore_%u_%s", AID, node_name);
  lib_shared_lock_sem = sem_open(sem_file, O_CREAT, S_IRUSR|S_IWUSR, 1);
  if (lib_shared_lock_sem == SEM_FAILED){
    error("Creating sempahore %s (%s)", sem_file, strerror(errno));
  }
  verbose_master(2, "SEM(%s) created", sem_file);

	/* This section allocates shared memory for processes in same node */
  pid_t master_pid = getpid();
	verbose(VPROC_INIT, "[%s] Master (PID: %d) creating shared regions for node synchronisation...",
            node_name, master_pid);

  /* ****************************************************************************************
   * We have two shared regions, one for the node and other for all the processes in the app.
   * Depending on how EAR is compiled, only one signature per node is sent.
   * ************************************************************************************* */

  /*****************************************/
  /* Attaching/Create to lib_shared region */
  /*****************************************/

	if (get_lib_shared_data_path(tmp,ID, AID, lib_shared_region_path) != EAR_SUCCESS) {
		error("Getting the lib_shared_region_path");
	} else {
		lib_shared_region=create_lib_shared_data_area(lib_shared_region_path);
		if (lib_shared_region==NULL){
			error("Creating the lib_shared_data region");
		}
		verbose(VPROC_INIT,"EARL[%d][%d] creating libsh path %s", ear_my_rank,getpid(), lib_shared_region_path);
		my_node_id												=	0;
		lib_shared_region->node_mgr_index = NULL_NODE_MGR_INDEX;
		lib_shared_region->earl_on 				= eard_ok || EARL_LIGHT;
		lib_shared_region->num_processes	=	1;
		lib_shared_region->num_signatures	=	0;
		lib_shared_region->master_rank 		= masters_info.my_master_rank;
		lib_shared_region->node_signature.sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
		lib_shared_region->job_signature.sig_ext  = (void *) calloc(1, sizeof(sig_ext_t));
		/* If not estimate_node_metrics node metrics are not computed and policy is not applied */
		lib_shared_region->estimate_node_metrics = ((ear_getenv(FLAG_DISABLE_NODE_METRICS) != NULL)? 0:1);
		if (!lib_shared_region->estimate_node_metrics) {
			verbose_master(2,"EARL[%d] node metrics disabled", getpid());
		}
  }
	debug("create_shared_regions and eard=%d",eard_ok);
	debug("Node connected %u",my_node_id);
#if MPI
  /* This barrier notifies the others the shared memory region is ready */
  if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
    error("MPI_Barrier");
    return;
  }
  if (!eard_ok && !EARL_LIGHT) return;
  if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
    error("MPI_Barrier");
    return;
  }
#else 
	if (app_node_multiprocess){
		snprintf(path_barrier, sizeof(path_barrier),"%s/%u/%u/.barrier1",tmp,ID,AID);
		verbose(VNTASKS_INIT,"EARL[%d][%d] sched_barrier section (%s)", ear_my_rank, getpid(), path_barrier);
		sched_local_barrier(path_barrier, ear_my_size);
		if (!eard_ok && !EARL_LIGHT) return;
		snprintf(path_barrier, sizeof(path_barrier),"%s/%u/%u/.barrier2",tmp,ID,AID);
		verbose(VNTASKS_INIT,"EARL[%d][%d] sched_barrier section (%s)", ear_my_rank, getpid(), path_barrier);
		sched_local_barrier(path_barrier, ear_my_size);
	}
#endif
  verbose(VPROC_INIT,"EARL[%d][%d] My node ID is %d/%d", ear_my_rank, getpid(), my_node_id, lib_shared_region->num_processes);

  /* This region is for processes in the same node */

  /************************************************/
  /* Attaching/Create to shared_signatures region */
  /************************************************/
	if (get_shared_signatures_path(tmp,ID, AID, shsignature_region_path)!=EAR_SUCCESS) {
    error("Getting the shsignature_region_path");
  } else {

		debug("Master node creating the signatures with %d processes", lib_shared_region->num_processes);

		sig_shared_region = create_shared_signatures_area(shsignature_region_path,
                lib_shared_region->num_processes);

		if (sig_shared_region==NULL){
			error("NULL pointer returned in create_shared_signatures_area");
		}
	}

	verbose_master(2, "[%s] Master (PID: %d) Shared region for shared signatures created for %d processes",
            node_name, master_pid, lib_shared_region->num_processes);

	sig_shared_region[my_node_id].master            = 1;
	sig_shared_region[my_node_id].pid               = master_pid;
	sig_shared_region[my_node_id].mpi_info.rank     = ear_my_rank;
	clean_my_mpi_info(&sig_shared_region[my_node_id].mpi_info);

#if MPI
  if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
    error("MPI_Barrier");
    return;
  }
#else
	if (app_node_multiprocess){
		snprintf(path_barrier, sizeof(path_barrier),"%s/%u/%u/.barrier3",tmp,ID,AID);
		verbose(VNTASKS_INIT,"EARL[%d][%d] sched_barrier section (%s)", ear_my_rank, getpid(), path_barrier);
  	sched_local_barrier(path_barrier, ear_my_size);
	}

#endif
  /* This part allocates memory for sharing data between nodes */
  masters_info.ppn = malloc(masters_info.my_master_size*sizeof(int));
  /* The node master, shares with other masters the number of processes in the node */
#if MPI
  if (share_global_info(masters_info.masters_comm,(char *)&lib_shared_region->num_processes,sizeof(int),(char*)masters_info.ppn,sizeof(int))!=EAR_SUCCESS){
    error("Sharing the number of processes per node");
  }
#else
  masters_info.ppn[0] = ear_my_size;
#endif
  int i;
  masters_info.max_ppn = masters_info.ppn[0];
  for (i=1;i<masters_info.my_master_size;i++){ 
    if (masters_info.ppn[i] > masters_info.max_ppn) masters_info.max_ppn=masters_info.ppn[i];
    if (masters_info.my_master_rank == 0) verbose_master(2,"[%s] Master (PID: %d) Processes in node %d = %d",node_name, master_pid, i,masters_info.ppn[i]);
  }

  verbose_master(2,"[%s] Master (PID: %d) max number of ppn is %d", node_name, master_pid,masters_info.max_ppn);
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
  verbose_master(2,"ratio_PPN %f = procs node %f max %f", ratio_PPN, (float)lib_shared_region->num_processes, (float)masters_info.max_ppn);
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
#if SHARED_MPI_SUMMARY
  masters_info.nodes_mpi_summary = (mpi_summary_t *) calloc(masters_info.my_master_size,sizeof(mpi_summary_t));
  if (masters_info.nodes_mpi_summary == NULL) {
    error("Allocating memory for MPI summary shared info");
  }
#endif
				/********************************/
				/* Attaching to node_mgr region */
				/********************************/
				verbose_master(2,"Master (PID: %d)  Creating node_mgr region in tmp %s", getpid(), tmp);
				if (nodemgr_job_init(tmp, &node_mgr_data) != EAR_SUCCESS){
								verbose_master(2,"Error creating node_mgr region");
				}
				ear_njob_t me;
				me.jid = application.job.id;
				me.sid = application.job.step_id;
				me.creation_time     = time(NULL);
#if WF_SUPPORT
        me.num_earl_apps = 1;
				me.modification_time = me.creation_time;
#endif
				CPU_ZERO(&me.node_mask);
				verbose_master(2,"Master (PID: %d)  Attaching to node_mgr region",getpid());
				if (nodemgr_attach_job(&me, &node_mgr_index) != EAR_SUCCESS){
								verbose_master(2,"Error attaching to node_mgr region");
				}
				verbose_master(2,"Master (PID: %d)  Attached at pos %d",getpid(), node_mgr_index);
				lib_shared_region->node_mgr_index = node_mgr_index;
        if (msync(lib_shared_region, sizeof(lib_shared_data_t),MS_SYNC) < 0){
          verbose(1, "Error synchronizing EARL shared memory (%s)", strerror(errno));
        }
				verbose_master(2,"Attached! at pos %u", node_mgr_index);

				/* Creates the specific region in shared memory, for jobs sharing the node */
				init_earl_node_mgr_info();
				if (nodemgr_get_num_jobs_attached(&current_jobs_in_node) == EAR_SUCCESS){
								verbose_master(2,"There are %u jobs sharing the node ",current_jobs_in_node);
				}
				exclusive = (current_jobs_in_node == 1);
				verbose_jobs_in_node(2,node_mgr_data,node_mgr_job_info);

        /* Add here the initialization of the shared region to coordinate with external libraries such as DLB */
#if 0
        get_ear_external_path(tmp, ID, shexternal_region_path);
        external_mgt = create_ear_external_shared_area(shexternal_region_path);
        verbose_master(2, "Created shared region for external libraries! (%s)", shexternal_region_path);
				verbose(2,"Master (PID: %d) shared regions created", getpid());
#endif

}

void verbose_shared_data()
{
	if (VERB_ON(2) && (MASTER_ID >= 0) && lib_shared_region && sig_shared_region)
	{
		verbose_master(2, "List of processes in node %s", node_name);
		for (uint lid = 0; lid < lib_shared_region->num_processes; lid++)
		{
			verbose_master(2,"\t Proc[%d] PID %d", lid, sig_shared_region[lid].pid);
		}
	}
}

/****************************************************************************************************************************************************************/
/****************** This function attaches the process to the shared regions to coordinate processes in same node ***********************************************/
/****************** It is executed by the NOT master processses  ************************************************************************************************/
/****************************************************************************************************************************************************************/


void attach_shared_regions()
{
				int bfd = -1;
				char *tmp = get_ear_tmp();
				uint ID = create_ID(application.job.id, application.job.step_id);
				uint current_jobs_in_node;
				#if !MPI
				char path_barrier[1024];
				#endif

				verbose(VPROC_INIT,"%sEARL[%d][%d] Attaching task to shared region%s", COL_BLU, ear_my_rank, getpid(), COL_CLR);

				/* This function is executed by processes different than masters */
				/* they first join the Node shared memory */
				xsnprintf(block_file,sizeof(block_file),"%s/%u/.my_local_lock.%s",tmp, ID, application.job.user_id);
				if ((bfd = ear_file_lock_create(block_file))<0){
								error("Creating lock file for shared memory:%s (%s)", block_file, strerror(errno));
				}

				/* We were using ID here but that was an error */
        xsnprintf(sem_file, sizeof(sem_file), "ear_semaphore_%u_%s", AID, node_name);
        lib_shared_lock_sem = sem_open(sem_file, O_CREAT, S_IRUSR|S_IWUSR, 1);
        if (lib_shared_lock_sem == SEM_FAILED){
          error("Creating/Attaching sempahore %s (%s)", sem_file, strerror(errno));
        }
				/* This link will be used to access the lib_shared_region lock file */
				lib_shared_lock_fd = bfd;
				if (get_lib_shared_data_path(tmp,ID, AID, lib_shared_region_path)!=EAR_SUCCESS){
								error("Getting the lib_shared_region_path");
				}else{
#if MPI
								if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
												error("MPI_Barrier");
												return;
								}
#else
	if (app_node_multiprocess){
		snprintf(path_barrier, sizeof(path_barrier),"%s/%u/%u/.barrier1",tmp,ID,AID);
		verbose(VNTASKS_INIT,"EARL[%d][%d] sched_barrier section (%s)", ear_my_rank, getpid(), path_barrier);
  	sched_local_barrier(path_barrier, ear_my_size);
	}
#endif
								do{
									verbose(VPROC_INIT,"EARL[%d][%d] attaching to libsh path %s", ear_my_rank,getpid(), lib_shared_region_path);
								  lib_shared_region = attach_lib_shared_data_area(lib_shared_region_path, &fd_libsh);
									if (lib_shared_region == NULL){
										usleep(1000000);
									}
								}while(lib_shared_region == NULL);
								while(ear_file_lock(bfd) < 0);
								if (!lib_shared_region->earl_on){
												debug("%s:Setting earl off because of eard",node_name);
												eard_ok = 0;
												ear_file_unlock(bfd);
												return;
								}
								my_node_id 												= lib_shared_region->num_processes;
								lib_shared_region->num_processes 	= lib_shared_region->num_processes+1;
								ear_file_unlock(bfd);
				}
#if MPI
				if (PMPI_Barrier(MPI_COMM_WORLD)!= MPI_SUCCESS){	
								error("In MPI_BARRIER");
				}
				if (PMPI_Barrier(MPI_COMM_WORLD)!= MPI_SUCCESS){	
								error("In MPI_BARRIER");
				}
#else
	if (app_node_multiprocess){
		snprintf(path_barrier, sizeof(path_barrier),"%s/%u/%u/.barrier2",tmp,ID,AID);
		verbose(VNTASKS_INIT,"EARL[%d][%d] sched_barrier section (%s)", ear_my_rank , getpid(), path_barrier);
  	sched_local_barrier(path_barrier, ear_my_size);
		snprintf(path_barrier, sizeof(path_barrier),"%s/%u/%u/.barrier3",tmp,ID,AID);
		verbose(VNTASKS_INIT,"EARL[%d][%d] sched_barrier section (%s)", ear_my_rank ,getpid(), path_barrier);
  	sched_local_barrier(path_barrier, ear_my_size);
	}

#endif
				verbose(VPROC_INIT,"EARL[%d ][%d] My node ID is %d/%d", ear_my_rank, getpid(), my_node_id, lib_shared_region->num_processes);
				if (get_shared_signatures_path(tmp,ID, AID, shsignature_region_path) != EAR_SUCCESS){
								error("Getting the shsignature_region_path  ");
				}else{
								verbose(VPROC_INIT,"EARL[%d ][%d] attaching to shared regions %s", ear_my_rank, getpid(), shsignature_region_path);
								sig_shared_region = attach_shared_signatures_area(shsignature_region_path,lib_shared_region->num_processes, &fd_sigsh);
								if (sig_shared_region == NULL){
												error("NULL pointer returned in attach_shared_signatures_area");
								}
				}
				verbose(VPROC_INIT,"EARL[%d ][%d] Attached!", ear_my_rank, getpid());
				memset(&sig_shared_region[my_node_id].sig, 0, sizeof(ssig_t));
				sig_shared_region[my_node_id].master            = 0;
				sig_shared_region[my_node_id].pid               = getpid();
				sig_shared_region[my_node_id].mpi_info.rank     = ear_my_rank;
				clean_my_mpi_info(&sig_shared_region[my_node_id].mpi_info);
				/* No masters processes don't allocate memory for data sharing between nodes */	

				/****************************************/
				/* Node mgr info in no-master processes */
				/****************************************/
				//verbose(2,"Creating node_mgr region for no masters");
				if (nodemgr_job_init(tmp, &node_mgr_data) != EAR_SUCCESS){
								verbose_master(2,"Error creating node_mgr region");
				}
				ear_njob_t me;
				me.jid = application.job.id;
				me.sid = application.job.step_id;
				me.creation_time = time(NULL);
				CPU_ZERO(&me.node_mask);
				verbose(VPROC_INIT,"%d: Waiting for master to be attached to node_mgr region", my_node_id);
				if (msync(lib_shared_region, sizeof(lib_shared_data_t),MS_SYNC) < 0){
					verbose(VPROC_INIT, "Error synchronizing EARL shared memory (%s)", strerror(errno));
				}
				/* Synchro: This value is set by the master */
				while (lib_shared_region->node_mgr_index == NULL_NODE_MGR_INDEX){
								usleep(100);
								if (msync(lib_shared_region, sizeof(lib_shared_data_t),MS_SYNC) < 0){
									verbose(VPROC_INIT, "Error synchronizing EARL shared memory (%s)", strerror(errno));
								}
				}
				verbose(3,"%d:We are attached at position %u", my_node_id, lib_shared_region->node_mgr_index);
				if (nodemgr_find_job(&me, &node_mgr_index) != EAR_SUCCESS){
								verbose_master(2,"Error attaching to node_mgr region");
								node_mgr_index = NULL_NODE_MGR_INDEX;
				}else{
          verbose_master(2,"Attached in node_mgr rerion pos %d", node_mgr_index);
        }
				//verbose(3,"Attached no-master! at pos %u", node_mgr_index);
				/* node_mgr_job_info maps other jobs data sharing the node */
				//if (node_mgr_index != NULL_NODE_MGR_INDEX) node_mgr_job_info = calloc(MAX_CPUS_SUPPORTED,sizeof(node_mgr_sh_data_t));
				init_earl_node_mgr_info();
				if (nodemgr_get_num_jobs_attached(&current_jobs_in_node) == EAR_SUCCESS){
								verbose_master(2,"There are %u jobs sharing the node ",current_jobs_in_node);
				}
				exclusive = (current_jobs_in_node == 1);

				verbose(VPROC_INIT,"%sEARL[%d][%d] Attached task to shared region%s", COL_BLU, ear_my_rank, getpid(), COL_CLR);
}

static void release_shared_memory_areas()
{
  
  verbose(WF_SUPPORT_VERB, "EARL[%d] releasing shared memory regions", getpid());
  return;

#if WF_SUPPORT

  /* WARNING */

  verbose(WF_SUPPORT_VERB, "EARL[%d] sig_shared_region", getpid());
  munmap(sig_shared_region, lib_shared_region->num_processes * sizeof(shsignature_t));
  verbose(WF_SUPPORT_VERB, "EARL[%d] lib_shared_region", getpid());
  munmap(lib_shared_region, sizeof(lib_shared_data_t));
  verbose(WF_SUPPORT_VERB, "EARL[%d] external_mgt", getpid());
  munmap(external_mgt, sizeof(ear_mgt_t));
  verbose(WF_SUPPORT_VERB, "EARL[%d] end releasing shared memory regions", getpid());
#endif
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

void attach_to_master_set(int master, uint can_synch)
{
				int color = 0;
#if MPI
				if (master) color = 0;
				else color        = MPI_UNDEFINED;
#endif
				if (master) masters_info.my_master_rank = 0;
				else 				masters_info.my_master_rank = -1;
				masters_info.my_master_size = 1;
#if MPI
				/* We must check all nodes have EAR loaded */
				if (can_synch)
				{
								debug("Creating a new communicator");
								if (PMPI_Comm_dup(MPI_COMM_WORLD,&masters_info.new_world_comm)!=MPI_SUCCESS){
												error("Duplicating MPI_COMM_WORLD");
								}
								if (PMPI_Comm_split(masters_info.new_world_comm, color, masters_info.my_master_rank, &masters_info.masters_comm)!=MPI_SUCCESS){
												error("Splitting MPI_COMM_WORLD");

								}
								debug("New communicators created");
				} else
				{
								debug("Some processes are not connecting, synchronization off");
				}
#endif
				// OpenMPI support
				if (can_synch) masters_comm_created = 1;
				else
				{
								if  (master)
								{
												masters_info.my_master_size = 1;
								}
				}

				// New for OpenMPI support
				if ((masters_comm_created) && (!color)){
#if MPI
								PMPI_Comm_rank(masters_info.masters_comm,&masters_info.my_master_rank);
								PMPI_Comm_size(masters_info.masters_comm,&masters_info.my_master_size);
#endif
								debug("New master communicator created with %d masters. My master rank %d\n",masters_info.my_master_size,masters_info.my_master_rank);
				}
				verbose(VPROC_INIT, "EARL[%d][%d] Attach to master set end, rank %d size %d", ear_my_rank, getpid(), masters_info.my_master_rank, masters_info.my_master_size);
}


/****************************************************************************************************************************************************************/
/* returns the local id in the node, local id is 0 for the  master processes and 1 for the others  */
/****************************************************************************************************************************************************************/
static int get_local_id(char *node_name)
{
	int lid = 1;
	char EID_folder[MAX_PATH_SIZE];

	strcpy(EID_folder, EID_application_path);

	if (app_node_multiprocess)
	{
		lid = ear_my_rank;
	} else
	{
		/* This lock file is used to select the master */
		xsnprintf(master_lock_filename, sizeof(master_lock_filename), "%s/.master_lock", EID_folder);

		// verbose(WF_SUPPORT_VERB, "Using EID lock path %s", master_lock_filename);
		debug("Using lock file %s",master_lock_filename);

		if ((fd_master_lock = ear_file_lock_master(master_lock_filename)) < 0)
		{
			if (errno != EBUSY) debug("Error creating lock file %s", strerror(errno));
			lid = 1;
		} else
		{
			lid = 0;
		}
	}
#if SHOW_DEBUGS
	if (lid) {
		debug("EARL[%d] Rank %d is the master in node %s", getpid(), ear_my_rank, node_name);
	}else
	{
		debug("EARL[%d] Rank %d is not the master in node %s", getpid(), ear_my_rank, node_name);
	}
#endif
		return lid;
}

/****************************************************************************************************************************************************************/
/**** Getting the job identification (job_id * 100 + job_step_id) */
/****************************************************************************************************************************************************************/

static void get_job_identification()
{
				char *job_id  = ear_getenv(SCHED_JOB_ID);
				char *step_id = ear_getenv(SCHED_STEP_ID);
				char *account_id=ear_getenv(SCHED_JOB_ACCOUNT);
				char *num_tasks=ear_getenv(SCHED_NUM_TASKS);

				if (num_tasks != NULL){
								debug("%s defined %s",SCHED_NUM_TASKS,num_tasks);
				}

				// It is missing to use JOB_ACCOUNT


				if (job_id != NULL) {
								my_job_id=atoi(job_id);
								if (step_id != NULL) {
												my_step_id=atoi(step_id);
								} else {
												step_id = ear_getenv(SCHED_STEPID);
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

/****************************************************************************************************************************************************************/
/*** We update EARL configuration based on shared memory information, ignoring potential malicious definition **/
/****************************************************************************************************************************************************************/

void update_configuration()
{
	char *cdynais_window;
	int dynais_window=system_conf->lib_info.dynais_window;

	// print_settings_conf(system_conf);

	set_ear_power_policy(system_conf->policy);
	set_ear_power_policy_th(system_conf->settings[0]);

	set_ear_p_state(system_conf->def_p_state);

	set_ear_coeff_db_pathname(system_conf->lib_info.coefficients_pathname);

	set_ear_dynais_levels(system_conf->lib_info.dynais_levels);

	/* Included for dynais tunning */
	cdynais_window = ear_getenv(HACK_DYNAIS_WINDOW_SIZE);
	if ((cdynais_window != NULL) && (system_conf->user_type == AUTHORIZED)){
		dynais_window = atoi(cdynais_window);
	}
	set_ear_dynais_window_size(dynais_window);

	set_ear_learning(system_conf->learning);

	dynais_timeout = system_conf->lib_info.dynais_timeout;

	lib_period = system_conf->lib_info.lib_period;

	check_every = system_conf->lib_info.check_every;

#if FAKE_LEARNING
	ear_whole_app = 1;
#else
	ear_whole_app = system_conf->learning;
#endif
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


void ear_lib_sig_handler(int s)
{
        //verbose(0, "%d: EAR finalized by signal %d", getpid(), s);
        /* This is new based to avoid the extra thread to enter (again) in the sig_handler */

        verbose(VPROC_MGT,"EAR[%d] signal %d received", getpid(), s);

        if (s == SIGCHLD){
					int pid, ret;
					while(( pid = waitpid(-1, &ret, WNOHANG)) > 0){
						if (WIFEXITED(ret)){ verbose(VPROC_MGT,"Child %d finish with exitcode %d", pid, WEXITSTATUS(ret));}
						else               { verbose(VPROC_MGT,"Child %d finish with signal %d",   pid, WTERMSIG(ret));}
					}
          return;
        }

        if (s == SIGPIPE){
					verbose(VPROC_MGT,"SIGPIPE received in process %d", getpid());
          mark_as_eard_disconnected(get_ear_tmp(), my_job_id,my_step_id,AID, getpid());
          eards_connection_failure();
          return;
        }

				if ((s == SIGBUS) || (s == SIGSEGV)){
					/* Should we cann ear_finalize?? */
					if (ear_getenv(FLAG_EAR_DEBUG) != NULL) print_stack(verb_channel);
					verbose(VPROC_MGT,"EARL[%d] exited by %s", getpid(), ((s==SIGBUS)?"SIGBUS":"SIGSEGV"));
          mark_as_eard_disconnected(get_ear_tmp(), my_job_id,my_step_id,AID, getpid());
          eards_connection_failure();
					clean_eid_folder();
					nodemgr_job_end(node_mgr_index);
				}

        if (s == SIGSEGV){ 
          exit(SIGSEGV);
        }

        if (cancelled || sig_shared_region[my_node_id].exited) return;
        if (syscall(SYS_gettid) != main_pid) return;

				cancelled = 1;

				ear_finalize(EAR_CANCELLED);
				exit(0);
}

/** Reads and updates tmp and etc hack variables.
 * \param[out] tmp The new tmp if hacked. If not, it is untouched. */
static void read_hack_env(char **tmp)
{
	char *hack_tmp = ear_getenv(HACK_EAR_TMP);
	char *hack_etc = ear_getenv(HACK_EAR_ETC);

	if (hack_tmp != NULL)
	{
		set_ear_tmp(hack_tmp);
		*tmp = get_ear_tmp(); // Update the passed tmp path
		setenv("EAR_TMP", hack_tmp, 1);

		verbose(VPROC_INIT,"[%s] Using HACK tmp %s ", node_name, get_ear_tmp());
	}

	if (hack_etc != NULL)
	{
		setenv("EAR_ETC", hack_etc, 1);
		verbose(VPROC_INIT,"[%s] Using HACK etc %s ", node_name, hack_etc);
	}
}

static int get_ear_application_id_multiprocess(int app_multi)
{
  int master_pid = getpid();
	// If multiprocess, we must get a single AID
	if (app_multi)
	{
		char AID_number_file[1024];	
		int fd, tries = 0;

		uint ID = create_ID(my_job_id,my_step_id);

		snprintf(AID_number_file, sizeof(AID_number_file), "%s/%u/AID", get_ear_tmp(), ID);

		if (ear_my_rank == 0)
		{
			// verbose(VJOB_INIT, "EARL[%d] generating AID %d in (%s)", ear_my_rank, master_pid, AID_number_file);
			earl_early_verb(VJOB_INIT, "Rank %d generating AID file %s. Writing PID %d", ear_my_rank, AID_number_file, master_pid);

			fd = open(AID_number_file, O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
			if (fd >= 0)
			{
				if (write(fd, &master_pid, sizeof(int)) < 0)
				{
					error_lib("Writing the master PID to fd %d: %s", fd, strerror(errno));
				}
				close(fd);
			}
		} else {
			// verbose(VPROC_INIT, "EARL[%d] reading AID from (%s)", ear_my_rank, AID_number_file);
			earl_early_verb(VPROC_INIT, "Rank %d reading AID from %s", ear_my_rank, AID_number_file);
			state_t st;
			do {
				st = ear_file_read(AID_number_file, (char *)&master_pid, sizeof(master_pid));
				if (st != EAR_SUCCESS) {
					usleep(100);
					tries++;
				}
			} while((st != EAR_SUCCESS) && (tries < 1000));
			/* AID read or timeout. Timeout can generated blocks later. */
			if (st == EAR_SUCCESS) {
				earl_early_verb(VPROC_INIT, "Rank %d master_id detected: %d", ear_my_rank, master_pid);
			} else {
				earl_early_verb(0, "Rank %d synchronization for AID failed.", ear_my_rank);
			}
		}
	}
	return master_pid;
}

static int get_ear_application_id(uint can_synch)
{
  int master_pid;
#if MPI
	if (can_synch)
	{
		master_pid = getpid();
		PMPI_Bcast(&master_pid, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}else{
		// This case applies to MPI apps where there is some issue with
		// the synchronization
		master_pid = get_ear_application_id_multiprocess(app_node_multiprocess);
	}
#else
	master_pid = get_ear_application_id_multiprocess(app_node_multiprocess);
#endif
	return master_pid;
}

static struct sigaction term_action, int_action, segv_action;
static void configure_sigactions()
{
	/* Now EARL can be configured to not configure sighandlers to avoid conflicts
	 * in the case the application wants to handle them. */

	char *py_multiproc_str = ear_getenv(FLAG_PYTHON_MULTIPROC);
	char *no_sighandle_str = ear_getenv(FLAG_NO_SIGHANDLER);

	long py_multiproc = 0;
	if (py_multiproc_str) {
		py_multiproc = strtol(py_multiproc_str, NULL, 10);
	}

	long no_sighandle = 0;
	if (no_sighandle_str) {
		no_sighandle = strtol(no_sighandle_str, NULL, 10);
	}

	/* In the case some of the above env vars were exported,
	 * we return and no signal handling is performed. */

	if (py_multiproc || no_sighandle) {
		earl_early_verb(1, "Sigactions disabled.");
		return;
	}

	// SIGTERM action for job cancellation
	sigemptyset(&term_action.sa_mask);
	term_action.sa_handler = ear_lib_sig_handler;
	term_action.sa_flags = 0;
	sigaction(SIGTERM, &term_action, NULL);

	// SIGINT
	sigemptyset(&int_action.sa_mask);
	int_action.sa_handler = ear_lib_sig_handler;
	int_action.sa_flags = 0;
	sigaction(SIGINT, &int_action, NULL);

  sigaction(SIGCHLD, &int_action, NULL);
  sigaction(SIGPIPE, &int_action, NULL);


  verbose(VPROC_INIT, "EARL[%d] SIGSEGV and SIGBUS sig handler defined", getpid());
  // SIGSEGV
  sigemptyset(&segv_action.sa_mask);
  segv_action.sa_handler = ear_lib_sig_handler;
  segv_action.sa_flags = 0;
  sigaction(SIGSEGV, &segv_action, NULL);
  sigaction(SIGBUS, &segv_action, NULL);
}


/*****************************************************************************************************************************************************/
/*****************************************************************************************************************************************************/
/*****************************************************************************************************************************************************/
/**************************************** ear_init ************************/
/*****************************************************************************************************************************************************/
/*****************************************************************************************************************************************************/
/*****************************************************************************************************************************************************/
/*****************************************************************************************************************************************************/
/*****************************************************************************************************************************************************/

#define START_END_OVH 1
uint fake_force_shared_node = 0;
void ear_init()
{
  char *summary_pathname;
  char *tmp;
  state_t st;
  char *ext_def_freq_str=ear_getenv(HACK_DEF_FREQ);
  int fd_dummy = -1;
  topology_t earl_topo; // Just used for printing

#if EAR_OFF
  return;
#endif

  uint fake_eard_error = 0;
  char *cfake_eard_error = ear_getenv(FAKE_EARD_ERROR);
  char *cfake_force_shared_node = ear_getenv(FLAG_FORCE_SHARED_NODE);
  if (cfake_eard_error != NULL) fake_eard_error = atoi(cfake_eard_error);
  if (cfake_force_shared_node != NULL) fake_force_shared_node = atoi(cfake_force_shared_node);

	set_no_files_limit(ULONG_MAX);
	set_stack_size_limit(ULONG_MAX);

#if START_END_OVH
  ullong elap_eard_init;
  timestamp start_eard_init, end_eard_init;
  timestamp_getfast(&start_eard_init);
#endif

  topology_init(&earl_topo);

  // Environment initialization
  ear_lib_environment();

	verb_level 		= get_ear_verbose();
	verb_channel 	= 2; // Default: stderr
  char * hack_verb_lv;
	if ((hack_verb_lv = ear_getenv(HACK_EARL_VERBOSE)) != NULL)
	{
		verb_level = atoi(hack_verb_lv);
		VERB_SET_LV(verb_level);
	}

	// Hacking environment goes after verbosity configuration since it has
	// verb 3 messages
	read_hack_env(&tmp);

	// Process identification (TID)
  main_pid = syscall(SYS_gettid);

	// External defined variable
  gethostname(node_name, sizeof(node_name));
  strtok(node_name, ".");

	earl_early_verb(VPROC_INIT, "--- EARL init ---");

  if (ear_lib_initialized)
	{
		earl_early_verb(1, "%sWarning%s EARL already initialized. Exiting...", COL_YLW, COL_CLR);
    return;
  }
	exiting = 0; // TODO: Document what is variable for

  timestamp_getfast(&ear_application_time_init);

  // MPI
#if MPI
  PMPI_Comm_rank(MPI_COMM_WORLD, &ear_my_rank);
  PMPI_Comm_size(MPI_COMM_WORLD, &ear_my_size);
#else
	if (ear_getenv(FLAG_NTASK_WORK_SHARING) != NULL)
	{
  	ear_my_rank     = get_my_sched_node_rank();
  	ear_my_size     = get_my_sched_group_node_size();
		if (ear_my_size > 1) app_node_multiprocess = 1;
		if (app_node_multiprocess)
		{
			earl_early_verb(1, "Node multiprocess enabled. Rank: %d Tasks: %d",
					ear_my_rank, ear_my_size);
		}
	}else{
		ear_my_rank = 0;
		ear_my_size = 1;
		if (get_my_sched_node_rank() > 0) first_application = 0;
	}
#endif
	earl_early_verb(VPROC_INIT, "Rank %d. Tasks %d.", ear_my_rank, ear_my_size);

	// synchronize_masters is declared statically
	// Provides support for OpenMPI applications launched through mpirun,
	// where the master node is not connected.
	synchronize_masters = must_masters_synchronize();
	if (!synchronize_masters)
	{
		earl_early_verb(VEARL_INFO2, "Node detection failed in this OpenMPI workload: masters will not synchronize.")
	}

	// This function gets info from the sched env var
	get_job_identification();

  // Test whether ear tmp folder exists
	tmp = get_ear_tmp();
	folder_t tmp_test;
	if (folder_open(&tmp_test, tmp) != EAR_SUCCESS)
	{
		// TODO: Try to create first the EAR_TMP.

		/* If tmp does not exist ear will be off but we need to set some tmp until we contact with other nodes */
		/* WARNING The content of this EAR_TMP directory won't be removed. */
		earl_early_verb(1, "%sWarning%s EAR_TMP directory (%s) does not exist in the node. Setting /tmp as the EAR_TMP for this application.", COL_YLW, COL_CLR, tmp);
		set_ear_tmp("/tmp");
		tmp = get_ear_tmp();
	}

  debug("[%s] Using tmp %s: ", node_name, tmp);

  // Test whether SID folder exists. If not, create it.
	state_t tmp_exists = check_job_folder();
  if (state_fail(tmp_exists))
	{
		earl_early_verb(1, "%sWarning%s Job-Step directory doesn't exist. Node multiprocess disabled.", COL_YLW, COL_CLR);

		app_node_multiprocess = 0; // disable "multiprocess" use case.
	}

  /* This is new. It is a collective operation to get the PID of rank 0. */
 	AID = get_ear_application_id(synchronize_masters);
  /* For non-MPI there is no problem. For MPI, it is possible the case of having two applications
   * at the same time with same jid/stepid where the rank 0 have the same PID (two nodes).
   * If that happen, then we will have a conflict. The probability would be close to 0. */
  earl_early_verb(WF_SUPPORT_VERB, "Application ID: %d", AID);

  load_app_mgr_env(); // TODO: This function is using my_master_rank, and it is not set yet.

  // Fundamental data

  pid_t my_pid = getpid();
  debug("Running in %s process = %d", node_name, my_pid);

  // TODO: We could update the default freq and avoid the macro DEF_FREQ on each policy.
  if (ext_def_freq_str)
	{
    ext_def_freq = (ulong) atol(ext_def_freq_str);
    earl_early_verb(VEAR_INIT, "%sWarning%s Externally defined default freq (kHz): %lu", COL_YLW, COL_CLR, ext_def_freq);
  }

  set_ear_total_processes(ear_my_size);
#if FAKE_LEARNING
  ear_whole_app = 1;
#else
  ear_whole_app = get_ear_learning_phase();
#endif
#if MPI
  num_nodes = get_ear_num_nodes();
  ppnode 		= ear_my_size / num_nodes;
#else
  num_nodes	= 1;
  ppnode		= ear_my_size;
#endif

	uint ID = create_ID(my_job_id, my_step_id);

	if (state_ok(tmp_exists))
	{
		// Create SID/AID folder
		nodemgr_lock_init(get_ear_tmp());

		create_eid_folder(get_ear_tmp(), my_job_id, my_step_id, AID);

		if (is_already_connected(get_ear_tmp(), my_job_id, my_step_id, AID, my_pid))
		{
			earl_early_verb(1, "%sWarning%s (Job/step/application) (%d/%d/%d) seems to be already conncted. Did the process mutate? Recovering EARD connection info...",
					COL_YLW, COL_CLR, my_job_id, my_step_id, AID);

			eards_recover_connection(get_ear_tmp(), my_job_id, my_step_id, AID, ear_my_rank);
			my_id = 0;
		} else {
			// Getting if the local process is the master or not
			/* my_id reflects whether we are the master or no my_id == 0 means we are the master *****/
			/* This function uses a lock only gathered by the master */
			my_id = get_local_id(node_name);
		}
	} else
	{
		my_id = 0;
	}

	int i_am_master = (my_id == 0);
	int verb_pid_lvl = i_am_master ? 1 : 2; // UPDATED
	earl_early_verb(verb_pid_lvl, "--- EARL INIT [PID: %d] [MASTER: %d] ---", my_pid, i_am_master);

	/* All masters connect in a new MPI communicator */
	attach_to_master_set(i_am_master, synchronize_masters);

	create_earl_verbose_files();

	earl_early_verb(VPROC_INIT, "Rank %d attached to master set. Synch %d MP %d.",
					ear_my_rank, synchronize_masters, app_node_multiprocess);

	if (state_fail(tmp_exists))
	{
		if (!my_id)
		{
			notify_eard_connection(0);
		}
		eard_ok = 0;
		cancelled = 0;
		earl_early_verb(1, "EAR_TMP doesn't exists. EARL off.");
		return;
	}

	/* This option is set to 0 when application is MPI and some processes are not running the EARL,
	 * which is a mess */
	if ((synchronize_masters == 0) && (!app_node_multiprocess))
	{
		verbose(1, "Rank %d: Masters can't synchronize and not a multiprocess app. Initialization aborted.", ear_my_rank);
		return;
	}

  ear_lib_initialized = 1; // Here EARL will be initialized, even it can't connect with EARD

  debug("Executing EAR library IDs(%d,%d)", ear_my_rank, my_id);
	earl_early_verb(VPROC_INIT, "Rank %d attaching to EARD shared regions...", ear_my_rank);

	/* At this point we haven't connected with EARD yet, but, if these regions don't exist, we can not connect.
	 * Three potential reasons:
	 *	1) EARD is not running,
	 *	2) there is some version mistmatch between the earl-earplug-eard,
	 *	3) Paths are not correct and we are looking at incorrect ear_tmp path */

	get_settings_conf_path(get_ear_tmp(),ID,system_conf_path);

	/* resched path must be migrated to EID folder */
	get_resched_path(get_ear_tmp(), ID, resched_conf_path);
	debug("[%s.%d] system_conf_path  = %s",node_name, ear_my_rank, system_conf_path);
	debug("[%s.%d] resched_conf_path = %s",node_name, ear_my_rank, resched_conf_path);

#if FAKE_EAR_NOT_INSTALLED
	uint first_dummy = 1;
#endif
	do
	{
		earl_early_verb(VPROC_INIT, "Rank %d settings path: %s", ear_my_rank, system_conf_path);
		system_conf = attach_settings_conf_shared_area(system_conf_path);
		resched_conf = attach_resched_shared_area(resched_conf_path);

#if FAKE_EAR_NOT_INSTALLED
    if (first_dummy){
      system_conf  = NULL;
      resched_conf = NULL;
      verbose_master(0, "FAKE_EAR_NOT_INSTALLED in shared areas");
      first_dummy = 0;
    }
#endif

    /* If I'm the master, I will create the dummy regions */
    create_dummy_path_lock(get_ear_tmp(), ID, dummy_areas_path, GENERIC_NAME);

    if ((system_conf == NULL) && (resched_conf == NULL) && !my_id)
    {
      earl_early_verb(VPROC_INIT, "Shared memory not present, creating dummy areas...");
      create_eard_dummy_shared_regions(get_ear_tmp(), create_ID(my_job_id,my_step_id));
      fd_dummy = ear_file_lock_create(dummy_areas_path);
    } else if ((system_conf == NULL) && (resched_conf == NULL) && my_id)
    {
      /* If I'm not the master, I will wait */
      while(!ear_file_is_regular(dummy_areas_path));
    }
  } while((system_conf == NULL) || (resched_conf == NULL));

  debug("[%s.%d] system_conf  = %p", node_name, ear_my_rank, system_conf);
  debug("resched_conf = %p", resched_conf);
  if (system_conf != NULL) debug("system_conf->id = %u", system_conf->id);
  debug("create_ID() = %u", create_ID(my_job_id,my_step_id)); // id*100+sid
  if (fake_eard_error) verbose_master(0, "Warning, executing fake eard errors: %u", fake_eard_error);

	earl_early_verb(VPROC_INIT, "Rank %d testing valid ID %u (settings %p resched %p)", ear_my_rank, create_ID(my_job_id, my_step_id), system_conf, resched_conf);

				uint valid_ID;
				if ((system_conf != NULL) && (resched_conf != NULL)){
					valid_ID = (system_conf->id == create_ID(my_job_id,my_step_id));
					if (fake_eard_error == 1){
						/* This is synthetic error to simulated the case the shared memory is not present OR invalid ID */
						valid_ID = 0;	
					}
					if (fake_eard_error == 2){
						if (ear_my_rank % 2) valid_ID = 0;
					}
				}else {
					valid_ID = 0;
				}

				earl_early_verb(VPROC_INIT, "Rank %d JOB/STEP valid: %u", ear_my_rank, valid_ID);

				/* Updating configuration */
				if (valid_ID){
								debug("Updating the configuration sent by the EARD");
								update_configuration();	
				}else{
								eard_ok = 0;
								verbose_master(2, "%sWarning%s EARD not detected.", COL_YLW, COL_CLR);
								earl_early_verb(VPROC_INIT, "Rank %d: EARD not detected.", ear_my_rank);
								if (!my_id) {
                        /* This lock file is created in get_local_id function, and it is used to select the master */
												debug("Application master releasing the lock %d %s", ear_my_rank,master_lock_filename);
												ear_file_unlock_master(fd_master_lock,master_lock_filename);
								}
    /* Only the node master will notify the problem to the other masters */
    if (!my_id) notify_eard_connection(0);
  }
  /* my_id is 0 for the master (for each node) and 1 for the others */
  /* eard_ok reflects the eard connection status, if eard_ok is 0, earl is disabled */

				// Application static data and metrics
				init_application(&application);
				application.is_mpi   	  = 1;
        #if FAKE_LEARNING
        application.is_learning = 0;
        #else
				application.is_learning	= ear_whole_app;
        #endif
				application.job.def_f   = getenv_ear_p_state();
				// debug("init loop_signature");
				// init_application(&loop_signature);
				// loop_signature.is_mpi	= 1;
				// loop_signature.is_learning	= ear_whole_app;

				// Getting environment data
				get_app_name(ear_app_name);
				verbose(VPROC_INIT, "EARL[%d][%d] App name is %s",ear_my_rank, getpid(),ear_app_name);
				if (application.is_learning){
								verbose_master(VEAR_INIT,"Learning phase app %s p_state %lu\n",ear_app_name,application.job.def_f);
				}
				if (ear_getenv("LOGNAME")!=NULL) strcpy(application.job.user_id, ear_getenv("LOGNAME"));
				else strcpy(application.job.user_id,"No userid");
				debug("User %s",application.job.user_id);
				strcpy(application.node_id, node_name);
				strcpy(application.job.user_acc,my_account);
				application.job.id        = my_job_id;
				application.job.step_id   = my_step_id;
#if WF_SUPPORT
        application.job.local_id  = AID;
#endif

				debug("Starting job");
				// sets the job start_time
				start_job(&application.job);
				start_mpi(&application.job);
				debug("Job time initialized");

				verbose(VPROC_INIT-1,"EARL[%d][%d] EARD connection section EARD=%d MASTER=%u",ear_my_rank, getpid(), eard_ok, my_id==0);
       if (eard_ok) { // all processes will connect with eard

                    char *col;

                    if (!my_id) {
                        col = COL_BLU;
                    } else {
                        col = COL_GRE;
                    }

                    // Connect process with EARD
                    state_t ret = eards_connect(&application, ear_my_rank);

                    verbose(VPROC_INIT-1, "%sProcess %d PID %d Connecting with EAR Daemon (EARD) %d node %s status=%s %s",
                            col, my_node_id, my_pid, ear_my_rank, node_name, ((ret == EAR_SUCCESS)?"Connected":"Not Connected"),COL_CLR);

										state_t eard_comp_state = check_eard_earl_compatibility(ret);

                    // Only for testing purposes
                    if (fake_eard_error == 3) {
                        ret = EAR_ERROR;
                    }

                    // Checking GPU support compatibility with EARD
                    uint eard_supports_gpu = gpu_is_supported();
                    if (lib_supports_gpu != eard_supports_gpu) {
                        verbose(1, "%sERROR%s EARL (%u) and EARD (%u) differ on GPU support. Please, inform to your system admin about this issue.",
                                COL_YLW, COL_CLR, lib_supports_gpu, eard_supports_gpu);
                        ret = EAR_ERROR;
                    }

      							if ((ret == EAR_SUCCESS) && (eard_comp_state == EAR_SUCCESS)) {

                        verbose(3, "%sRank %d connected with EARD%s", COL_BLU, ear_my_rank, COL_CLR);

                        if (!my_id) {
                            notify_eard_connection(1);
                            mark_as_eard_connected(get_ear_tmp(), my_job_id, my_step_id, AID, my_pid);
                        }

                    } else {
                        verbose(VPROC_INIT-1, "%sRank %d NOT connected with EARD%s %s",
                                COL_BLU, ear_my_rank, COL_CLR, state_msg);
                        eard_ok = 0;
                        if (!my_id) notify_eard_connection(0);
                    }
       }
       /* At this point, the master has detected EARL must be set to off , we must notify the others processes about that */

				debug("Shared region creation section");
				if (!my_id) {
								verbose(VPROC_INIT,"EARL[%d][%d] creating", ear_my_rank, getpid());
								debug("%sI'm the master, creating shared regions%s",COL_BLU,COL_CLR);
								create_shared_regions();
				} else {
								verbose(VPROC_INIT,"EARL[%d][%d] attaching", ear_my_rank, getpid());
								attach_shared_regions();
				}
				verbose(VPROC_INIT,"EARL[%d][rank = %d]  Created/Attached shared regions Total procs %d", getpid(), ear_my_rank, lib_shared_region->num_processes);
				verbose_shared_data();

        if (!eard_ok) {

                    /* Dummy areas path is used in case the EARD was not present */
                    if (!my_id && (fd_dummy >= 0)) {
                        int err;
                        err = ear_file_unlock_master(fd_dummy, dummy_areas_path);
                        if (err) verbose_master(2, "Error cleaning dummy area %s (%s)", dummy_areas_path, strerror(err));
                    }

                    if (!EARL_LIGHT) {
                        verbose_master(2,"%s: EARL off because of EARD",node_name);
                        strcpy(application.job.policy," "); /* Cleaning policy */
												ear_lib_initialized = 0;
                        return;
                    } else {
                        verbose_master(2, "EARD is not running but EARL light version is enabled");
                    }
        }

				/* Cluster conf */
				get_ser_cluster_conf_path(get_ear_tmp(), ser_cluster_conf_path);

        debug("Using ser cconf path %s", ser_cluster_conf_path);
				ser_cluster_conf = attach_ser_cluster_conf_shared_area(ser_cluster_conf_path, &ser_cluster_conf_size);
				debug("Serialized cluster conf requires %lu bytes", ser_cluster_conf_size);
				deserialize_cluster_conf(&cconf, ser_cluster_conf, ser_cluster_conf_size);

				debug("END Shared region creation section");
				/* Processes in same node connectes each other*/

				if (system_conf->user_type != AUTHORIZED) 	VERB_SET_LV(ear_min(verb_level,1));

			// We print here the topology in order to do it only at verbose 2
		  if (VERB_ON(2))
  		{
    		if (ear_my_rank == 0)
    		{
      		topology_print(&earl_topo, verb_channel);
    		}
  		}

				// Initializing architecture and topology
				if ((st=get_arch_desc(&arch_desc))!=EAR_SUCCESS){
								error("Retrieving architecture description");
								/* How to proceeed here ? */
								lib_shared_region->earl_on = 0;
								strcpy(application.job.policy," "); /* Cleaning policy */
								debug("Process %d not completes the initialization", my_node_id);
								return;
				}

                /* Based on family and model we set limits for this node */
                classify_init(&arch_desc.top, system_conf);
                /* Getting limits for this node */
                get_classify_limits(&phases_limits);


		// Initializing metrics
		verbose_master(2, "EARL Metrics initialization");
                if (metrics_load(&arch_desc.top) != EAR_SUCCESS) {
                    verbose(1, "%sError%s EAR metrics initialization not succeed (%s), turning off EARL...", COL_RED, COL_CLR, state_msg);
                    eard_ok = 0;
                    strcpy(application.job.policy," "); /* Cleaning policy */
                    debug("Process %d not completes the initialization", my_node_id);
                    return;
				}


				if (!eard_ok && !EARL_LIGHT){ 
								lib_shared_region->earl_on = 0;
								strcpy(application.job.policy," "); /* Cleaning policy */
								debug("Process %d not completes the initialization", my_node_id);
								return;
				}

				debug("End metrics init");

				// Initializing DynAIS
				debug("Dynais init");
				dynais = dynais_init(&arch_desc.top, get_ear_dynais_window_size(), get_ear_dynais_levels());
				if (dynais_build_type() == DYNAIS_NONE){ 
          ear_guided = TIME_GUIDED;
          verbose_master(EARL_GUIDED_LVL, "Switching to time guided becuase dynais is no supported");
        }
				debug("Dynais end");

				arch_desc.max_freq_avx512=system_conf->max_avx512_freq;
				arch_desc.max_freq_avx2=system_conf->max_avx2_freq;

				// Getting frequencies from EARD
				uint freq_count;

				debug("initializing frequencies");
				get_frequencies_path(get_ear_tmp(), app_mgt_path);
				debug("looking for frequencies in '%s'", app_mgt_path);
				attach_frequencies_shared_area(app_mgt_path, (int *) &freq_count);
				dettach_frequencies_shared_area();
				// You can utilize now the frequency API
				arch_desc.pstates = frequency_get_num_pstates();
				// Other frequency things
				if (ear_my_rank == 0) {
								if (ear_whole_app == 1 && ear_use_turbo == 1) {
												verbose_master(VEAR_INIT, "turbo learning phase, turbo selected and start computing");
												if (!my_id) { mgt_cpufreq_set_governor(no_ctx, Governor.performance); }
								} else {
												verbose_master(VEAR_INIT, "learning phase %d, turbo %d", ear_whole_app, ear_use_turbo);
								}
				}

				// This is not frequency any more
				if (masters_info.my_master_rank >= 0){
                /* This region must be migrated after the eard_new_application */
								get_app_mgt_path(get_ear_tmp(),ID, app_mgt_path);
								verbose_master(VEAR_INIT,"app_mgt_path %s",app_mgt_path);
								app_mgt_info = attach_app_mgt_shared_area(app_mgt_path);
								if (app_mgt_info == NULL){
												error("Application shared area not found");
												eard_ok = 0;
												lib_shared_region->earl_on = 0;
												strcpy(application.job.policy," "); /* Cleaning policy */
												debug("Process %d not completes the initialization", my_node_id);
												return;
								}
								fill_application_mgt_data(app_mgt_info);
								/* At this point we have notified the EARD the number of nodes in the application */
								check_large_job_use_case();
								/* This area is RW */
                /* This region must be migrated after the eard_new_application */
								get_pc_app_info_path(get_ear_tmp(),ID, pc_app_info_path);
								verbose_master(VEAR_INIT,"pc_app_info path %s",pc_app_info_path);
								pc_app_info_data=attach_pc_app_info_shared_area(pc_app_info_path);
								if (pc_app_info_data==NULL){
												error("pc_application_info area not found");
												eard_ok = 0;
												lib_shared_region->earl_on = 0;
												strcpy(application.job.policy," "); /* Cleaning policy */
												return;

								}
				}
#if 0
                if (is_affinity_set(&arch_desc.top, my_pid, &ear_affinity_is_set, &ear_process_mask) != EAR_SUCCESS)
                {
                    error("Checking the affinity mask");
                }
                else
                {
                    /* Copy mask in shared memory */
                    if (ear_affinity_is_set) {
                        sig_shared_region[my_node_id].cpu_mask = ear_process_mask;
                        sig_shared_region[my_node_id].affinity = 1;
                        sig_shared_region[my_node_id].num_cpus = cpumask_count(&ear_process_mask);
                    } else { 
                        sig_shared_region[my_node_id].affinity = 0;
                        verbose_master(2,"Affinity mask not defined for rank %d",masters_info.my_master_rank);
                    }
                }
#endif
#ifdef SHOW_DEBUGS
				print_arch_desc(&arch_desc);
#endif
#if 1
				if (state_ok(cpumask_get_processmask(&ear_process_mask, my_pid)))
#else
        if (!sched_getaffinity(my_pid, sizeof(cpu_set_t), &ear_process_mask))
#endif
				{
					int proc_mask_cpu_count = cpumask_count(&ear_process_mask);
					if (proc_mask_cpu_count > 0)
					{
						ear_affinity_is_set = 1;
						sig_shared_region[my_node_id].cpu_mask = ear_process_mask;
						sig_shared_region[my_node_id].num_cpus = proc_mask_cpu_count;
					} else
					{
						verbose(VPROC_INIT, "%sWARNING%s CPU mask for thread ID %d has no CPUs.",
								COL_YLW, COL_CLR, my_pid);
					}

					sig_shared_region[my_node_id].affinity = ear_affinity_is_set;
				} else
				{
					verbose(1, "%sERROR%s Getting affinity mask for thread ID %d: %s",
							COL_RED, COL_CLR, my_pid, strerror(errno));
				}

                sig_shared_region[my_node_id].ready = 1;

                /* This functions needs to be synchronized (pending), to be done at first node
                 * signature computation. */
                /* Computes the total number of cpus for this job */
                if (masters_info.my_master_rank >= 0) {
                    debug("Waiting for processes to fill shared signatures");
#if 0
                    /* Synchro : Wait the PID is set by all the processes. */
                    while(!all_signatures_initialized(lib_shared_region, sig_shared_region)) usleep(100);
#endif
                    /* Synchro: Wait for slaves CPU masks set. */
                    while (!are_signatures_ready(lib_shared_region, sig_shared_region, NULL)) usleep(100);

                    cpu_set_t jmask;
                    aggregate_all_the_cpumasks(lib_shared_region, sig_shared_region, &jmask);

                    /* lib_shared_region is shared by all the processes in the same node */
                    lib_shared_region->node_mask = jmask;

                    /* node_mgr_data is shared with the eard */
                    node_mgr_data[node_mgr_index].node_mask = jmask;	

                    /* app_mgt_info is shared with the eard through the powermon_app_t */
                    app_mgt_info->node_mask = jmask;

                    debug("Node mask computed");

                    debug("Computing number of cpus per job and node mask");
                    compute_job_cpus(lib_shared_region, &lib_shared_region->num_cpus);
                } else {
                    // Slaves: wait for master to set up node mask
                    while (!lib_shared_region->num_cpus){ 
                      usleep(100);
                      msync(lib_shared_region, sizeof(lib_shared_data_t), MS_SYNC);
                    }
                }

                // Master: Resets ready state of shared signatures
                if (masters_info.my_master_rank >= 0)
                {
                    free_node_signatures(lib_shared_region, sig_shared_region);
                }

                verbose_master(2, "EARL Init power policy");
                // init_power_models(system_conf->user_type, &system_conf->installation, &arch_desc);

                if (state_ok(init_power_policy(system_conf, resched_conf))) {

                  verbose_master(VEAR_INIT, "Policies and models initialized.");

				} else {

					verbose_master(VEAR_INIT, "%sWARNING%s An error ocurred initializing the power policy.",
							COL_YLW, COL_CLR);	
				}


        verbose_master(2,"Init CPU power models for node sharing");
				if (cpu_power_model_load(system_conf, &arch_desc, API_NONE) == EAR_SUCCESS){
								verbose_master(VEAR_INIT, "CPU power models loaded.");
				}

				if (cpu_power_model_init() == EAR_SUCCESS){
								verbose_master(VEAR_INIT, "CPU power model initialized.");
				}


				if (ext_def_freq==0){
								EAR_default_frequency=system_conf->def_freq;
								EAR_default_pstate=system_conf->def_p_state;
				}else{
								EAR_default_frequency=ext_def_freq;
								EAR_default_pstate=frequency_closest_pstate(EAR_default_pstate);
#if 0
								if (EAR_default_frequency != system_conf->def_freq) {
												if (!my_id) {
																uint pstate_index;
																if (state_ok(mgt_cpufreq_get_index(no_ctx, (ullong) EAR_default_frequency, &pstate_index, 0))) {
																				mgt_cpufreq_set_current(no_ctx, pstate_index, all_cpus);
																}
												}
								}
#endif
				}

				strcpy(application.job.policy,system_conf->policy_name);
        verbose(3,"Initializing APP name %s", ear_app_name);
				strcpy(application.job.app_id, ear_app_name);

				// Passing the frequency in KHz to MHz
				application.signature.def_f   = application.job.def_f = EAR_default_frequency;
				application.job.procs         = ear_my_size;
				application.job.th            = system_conf->settings[0];
        application.signature.sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
        #if DCGMI
        if (metrics_dcgmi_enabled()){
          sig_ext_t *sig_ext = (sig_ext_t *)application.signature.sig_ext;
          sig_ext->dcgmis.set_cnt = 0;
        }
        #endif


				// Copying static application info into the loop info
				// memcpy(&loop_signature, &application, sizeof(application_t));

				summary_pathname = get_ear_user_db_pathname();
				// if (masters_info.my_master_rank >= 0){
								// Report API
								char my_plug_path[512];
								utils_create_plugin_path(my_plug_path, system_conf->installation.dir_plug, ear_getenv(HACK_EARL_INSTALL_PATH), system_conf->user_type);

								if (summary_pathname == NULL){
												if (report_load(my_plug_path,system_conf->lib_info.plugins) != EAR_SUCCESS){
																verbose_master(1,"Error in reports API load ");
												}
								}else{
												char plugins_ext[SZ_PATH_INCOMPLETE];
												setenv("EAR_USER_DB_PATHNAME",summary_pathname,1);
												xsnprintf(plugins_ext,sizeof(plugins_ext),
                                                        "%s:csv_ts.so",system_conf->lib_info.plugins);
												if (report_load(my_plug_path,plugins_ext) != EAR_SUCCESS){
																verbose_master(1,"Error in reports API load ");
												}
								}
								/* local_rank, global_rank, master_rank */
								report_create_id(&rep_id, my_node_id, ear_my_rank, masters_info.my_master_rank);
								if (report_init(&rep_id,&cconf) != EAR_SUCCESS){
												verbose_master(1,"Error in reports API initialization ");
								}
				// }
        creport_earl_events = ear_getenv(FLAG_REPORT_EARL_EVENTS);
        if (creport_earl_events != NULL) report_earl_events = atoi(creport_earl_events);
        if (report_earl_events) verbose_master(1," EARL events reported to DB %s", (report_earl_events?"enabled":"disabled"));

				// States
				states_begin_job(my_id,  ear_app_name);

				// Print summary of initialization
				print_local_data();
				// ear_print_lib_environment();
				fflush(stderr);

				// Tracing init
				traces_init(system_conf, application.job.app_id, ear_my_rank,
						my_node_id, num_nodes, ear_my_size, ppnode);

				traces_start();
				traces_frequency(ear_my_rank, my_node_id, EAR_default_frequency);
				traces_stop();

				// All is OK :D
				if (masters_info.my_master_rank==0) {
								verbose_master(1, "EAR initialized successfully.");
				}

        configure_sigactions();


       #if !EAR_OFF
				/* Monitor is used for applications with no mpi of few mpi calss per second. Gets control of signature computation
         * if neded*/

        if (!module_mpi_is_enabled())
        {
            ear_guided = TIME_GUIDED;
        }
#if 0
        if (ear_guided == TIME_GUIDED){
#endif
				  if (monitor_init() != EAR_SUCCESS) verbose(0,"Monitor init fails! %s",state_msg);
				  earl_monitor = suscription();
				  earl_monitor->call_init = earl_periodic_actions_init;
				  earl_monitor->call_main = earl_periodic_actions;
				  earl_monitor->time_relax = 10000;
				  earl_monitor->time_burst = 1000;

				  if (monitor_register(earl_monitor) != EAR_SUCCESS) verbose(0,"Monitor register fails! %s",state_msg);
				  if (ear_guided == DYNAIS_GUIDED){
				    ITERS_PER_PERIOD = earl_monitor->time_relax/1000;
            monitor_relax(earl_monitor);
          }else{
				    ITERS_PER_PERIOD = earl_monitor->time_burst/1000;
            monitor_burst(earl_monitor, 0);
          }
          verbose_master(EARL_GUIDED_LVL,"Monitor ON relax %d burst %d", earl_monitor->time_relax, earl_monitor->time_burst);
#if 0
        }
#endif

#if MPI_OPTIMIZED
				fine_grain_metrics_init();
#endif

				if (DLB_SUPPORT)
				{
					// TODO: Add support for environment variable
					if (state_fail(earl_dlb_talp_init()))
					{
						verbose_error("EAR DLB TALP module init: %s", state_msg);
					}
				}
#endif // !EAR_OFF

#if START_END_OVH
  timestamp_getfast(&end_eard_init);
        elap_eard_init = timestamp_diff(&end_eard_init, &start_eard_init, TIME_USECS);
        verbose_master(EARL_GUIDED_LVL,"Initialization cost: %llu usec dynais_guided %u ", elap_eard_init, ear_guided != TIME_GUIDED);
#endif

#if USE_GPUS
        if (is_cuda_enabled()){
            ear_cuda_init();
        }
#endif
        verbose(VPROC_INIT, "%s EAR [%d] end .......................................%s", COL_BLU, main_pid, COL_CLR);
}


/**************************************** ear_finalize ************************/

/*
static void print_resource_usage()
{
  struct rusage my_usage;
  getrusage(RUSAGE_SELF, &my_usage);

  verbose(0,"%s: User cpu time %llu system cpu time %llu page faults %ld block input %ld block output %ld voluntary ctx %ld involuntary ctx %ld", node_name, my_usage.ru_utime.tv_sec*1000+my_usage.ru_utime.tv_usec/1000, my_usage.ru_stime.tv_sec*1000+my_usage.ru_stime.tv_usec/1000, my_usage.ru_majflt, my_usage.ru_inblock, my_usage.ru_oublock, my_usage.ru_nvcsw, my_usage.ru_nivcsw);

}
*/
#define MAX_TRIES_FINALIZE 100
void ear_finalize(int exit_status)
{
				uint fake_error = 0;
				state_t st;
				uint attemps = 0;
				char *cfake = ear_getenv(FAKE_PROCESS_ERRROR);
				if (cfake != NULL) fake_error = atoi(cfake);
				/* This lock is to avoid the earl_periodic_action to be excuted at the same time than ear_finalize */
				do{
					st = ear_trylock(&earl_th_locks);
					attemps++;
					usleep(1000);
				}while ((st != EAR_SUCCESS) && (attemps < MAX_TRIES_FINALIZE)); 
				if (state_fail(st)){ 
					verbose(2, "%sEARL[%d][%d] at %s[WARNING]%s ear_finalize without lock, continuing to avoid app block", COL_YLW, ear_my_rank, getpid(), node_name, COL_CLR);
				}
				exiting = 1;
				if (state_ok(st)){
					ear_unlock(&earl_th_locks);
				}
				/* We don't execute the whole app with the lock */

	if (!ear_lib_initialized)
	{
		verbose(1, "%s[%d][%s][WARNING]%s EARL not initialized. Returning...", COL_YLW, main_pid, node_name, COL_CLR);
		return;
	}
       

				if (synchronize_masters == 0) return; // TODO: why?

         #if EAR_OFF
     //    print_resource_usage();
         return;
         #endif

        #if START_END_OVH
        ullong elap_eard_init;
        timestamp start_eard_init, end_eard_init;
        timestamp_getfast(&start_eard_init);
        #endif

        verbose_master(2, "ear_finalize on=%d status %d PID %d", lib_shared_region->earl_on, exit_status, getpid());
        #if !MPI
        if (getpid() != main_pid){
          //verbose(WF_SUPPORT_VERB,"Finalizing pid %d" , getpid());
          exit(0);
        }
        #endif

				if (fake_error) verbose_master(0, "Warning, executing fake errors in ear_finalize : %u", fake_error);
		
				if (fake_error)
				{
				/* This use case forces sintheticaly a master proces exiting because an error. The
 				 * objective is to test the EARL is not blocking. We just mark all the masters as 
 				 * cancelled */ 	
				if (masters_info.my_master_rank >= 0) cancelled = 1;
				}

				if (fake_error == 2){
				/* This use case forces sintheticaly a NO master proces exiting because an error. The
         * objective is to test the EARL is not blocking. We just mark all the NO masters as 
         * cancelled */
        if (masters_info.my_master_rank < 0) cancelled = 1;
				}


				// if we are not the master, we return
				if ((cancelled && (masters_info.my_master_rank < 0)) || (synchronize_masters == 0 )) {
								cleanup_job_folder();
								sig_shared_region[my_node_id].exited = 1;
								return;
				}
#if ONLY_MASTER
				if (my_id) {
								sig_shared_region[my_node_id].exited = 1;
								return;
				}
#endif
				if (!eard_ok && !EARL_LIGHT)
				{ 
                verbose(WF_SUPPORT_VERB, "EARL[%d]: Cleaning folders and returning", getpid());
								cleanup_job_folder();
								return;
				}
				if (!lib_shared_region->earl_on){
                verbose(WF_SUPPORT_VERB, "EARL[%d]: EARL disabled, returning", getpid());
								return;
				}
				if (!eards_connected() && !EARL_LIGHT && (masters_info.my_master_rank>=0)){
								lib_shared_region->earl_on = 0;
								cleanup_job_folder();
								sig_shared_region[my_node_id].exited = 1;
                
								return;
				}
				verbose(WF_SUPPORT_VERB+1, "EARL[%d] node id %d: Finalizing", getpid(), my_node_id);
				if (!my_id){
                /* Releasing master lock file. Used to select the master */
								debug("Application master releasing the lock %d %s", ear_my_rank, master_lock_filename);
								ear_file_unlock_master(fd_master_lock,master_lock_filename);
								mark_as_eard_disconnected(get_ear_tmp(), my_job_id,my_step_id,AID, getpid());
				}

        traces_stop();
        traces_end(ear_my_rank, my_node_id, 0);
        traces_mpi_end();

				if (DLB_SUPPORT)
				{
					earl_dlb_talp_dispose();
				}

        if (ear_guided == TIME_GUIDED){
          verbose(WF_SUPPORT_VERB, "EARL[%d]  stopping  EARL monitor", getpid());
          monitor_unregister(earl_monitor);
				  monitor_dispose();
				  debug("%d: monitor_disposed", my_node_id);
        }
        verbose(WF_SUPPORT_VERB+1, "EARL[%d]  report dispose", getpid());

        report_dispose(&rep_id);

				// Closing and obtaining global metrics
				dispose=1;
				verbose_master(2,"Total resources computed %d",get_total_resources());
				debug("%d: before metrics_dispose", my_node_id);
				metrics_dispose(&application.signature, get_total_resources());
				sig_shared_region[my_node_id].exited = 1;
				debug("%d: after metrics_dispose" ,my_node_id);
				dynais_dispose();
				if (!my_id) frequency_dispose();

				// Writing application data

				if (!my_id) 
				{
								if (application.is_learning) verbose_master(1,"Application in learning phase mode");
								debug("Reporting application data");
								verbose_master(2,"Filtering DC node power with limit %.2lf",system_conf->max_sig_power);
								signature_clean_before_db(&application.signature, system_conf->max_sig_power);
								verbose_master(2, "Reporting data");
#if USE_GPUS
								verbose_master(2, " Application with %d GPUS", application.signature.gpu_sig.num_gpus);
#endif
								end_job(&application.job);
								end_mpi(&application.job);
								verbose(2, "EARL[%d][%d] Application report (type %s)", ear_my_rank,getpid(),( first_application? "Regular App":"WF App"));
								if (first_application)
								{
									if (state_fail(report_applications(&rep_id, &application, 1)))
									{
												verbose_master(1, "Error reporting application");
									}
								} else
								{
									fill_basic_sig(&application);
									report_misc(&rep_id, WF_APPLICATION, (const char *) &application,1);
								}

                if (masters_info.my_master_rank == 0){
								  report_mpi_application_data(1, &application);
									// This version prints more details
									// verbose_application_data(1, &application);
                }else{
								  report_mpi_application_data(2, &application);
                }
				}
				// Closing any remaining loop
				if (loop_with_signature) {
								debug("loop ends with %d iterations detected", ear_iterations);
				}
                                #if 0
				if (in_loop) {
								states_end_period(ear_iterations);
				}
                                #endif
				states_end_job(my_id,  ear_app_name);

				policy_end();

				dettach_settings_conf_shared_area();
				dettach_resched_shared_area();

				if (masters_info.my_master_rank >= 0) nodemgr_job_end(node_mgr_index);
				verbose_master(2,"Dettached from node_mgr region id %u",node_mgr_index);

        sem_unlink(sem_file);
        sem_close(lib_shared_lock_sem);


				// C'est fini
				if (eards_connected()){ 
					verbose(3, "Process %d disconnecting from EARD", my_node_id);
					eards_disconnect();
				}

        if (masters_info.my_master_rank >= 0) clean_eid_folder();
        //print_resource_usage();

        #if START_END_OVH
        timestamp_getfast(&end_eard_init);
        elap_eard_init = timestamp_diff(&end_eard_init, &start_eard_init, TIME_USECS);
        verbose_master(EARL_GUIDED_LVL,"Finalization cost (%s): %llu usec dynais calls %lu dynais_enabled %s  periodic mode %s guided by %s mpi_with_stats %llu total mpi %llu", \
          node_name,\
          elap_eard_init, dynais_calls, \
          (dynais_enabled == DYNAIS_ENABLED)?"ON":"OFF", \
          (ear_periodic_mode == PERIODIC_MODE_ON)?"ON":"OFF",\
          (ear_guided == TIME_GUIDED)?"TIME":"Dynais",total_mpi_call_statistics(), total_mpi_calls);
        #endif
        if (exit_status != EAR_SUCCESS) exit(exit_status);

}

#if MPI

void ear_mpi_call_dynais_on(mpi_call call_type, p2i buf, p2i dest);
void ear_mpi_call_dynais_off(mpi_call call_type, p2i buf, p2i dest);

void ear_mpi_call(mpi_call call_type, p2i buf, p2i dest)
{

    if (!ear_lib_initialized) {
				return;
				// Some day, we had trouble with an application. As a last chance to
				// solve it we decide to do the below approach. But currently we don't know
				// whether it was the solution.
				// Now, ear_lib_initialized is used to mark a correct initiliazation of
				// commuication files (EAR_TMP), so a false value is meaningful.
				// Moreover, an application can't call an MPI call without calling
				// MPI_Init, which causes the EARL wrapper call ear_init method.
        // verbose_master(2, "%sWARNING%s EAR not initialized.", COL_RED, COL_CLR);
        // ear_init();
    }

    if (!eard_ok && !EARL_LIGHT) return;
    if (synchronize_masters == 0) return;
    if (!lib_shared_region->earl_on) return;

    total_mpi_calls++;

#if ONLY_MASTER
    if (my_id) return;
#endif

    if (ear_guided == TIME_GUIDED) return;

    /* The learning phase avoids EAR internals. ear_whole_app is set to 1 when learning-phase is set */
    if (!ear_whole_app)
    {
        ulong  ear_event_l = (ulong)((((buf>>5)^dest)<<5)|call_type);

        //unsigned short ear_event_s = dynais_sample_convert(ear_event_l);

        if (masters_info.my_master_rank>=0)
        {
            traces_mpi_call(ear_my_rank, my_id,
                    (ulong) ear_event_l,
                    (ulong) buf,
                    (ulong) dest,
                    (ulong) call_type);
        }

        /* EAR can be driven by Dynais or periodically in those cases where dynais can not detect any period. 
         * ear_periodic_mode can be ON or OFF 
         */
        switch(ear_periodic_mode)
        {
            case PERIODIC_MODE_OFF:
            {
                switch(dynais_enabled)
                {
                    case DYNAIS_ENABLED:
                    {
                        /* First time EAR computes a signature using dynais, check_periodic_mode is set to 0 */
                        if (check_periodic_mode == 0)
                        {
                            ear_mpi_call_dynais_on(call_type, buf, dest);
                        } else
                        {
                            /* Check here if we must move to periodic mode, do it every N mpicalls to reduce the overhead */
                            if ((total_mpi_calls % check_every) == 0)
                            {
                                /* We add a minimum number of mpi calls to allow the application to start a "normal" computation */
                                if (avg_mpi_calls_per_second() >= MAX_MPI_CALLS_SECOND)
                                {
#if 0
                                    verbose(0, "Going to periodic mode because limit exceeded avg mpi_per_sec %f limit %u", avg_mpi_calls_per_second(),
                                            MAX_MPI_CALLS_SECOND);
#endif
                                    limit_exceeded = 1;
                                }

                                time_t curr_time;
                                time(&curr_time);

                                double time_from_mpi_init = difftime(curr_time, application.job.start_time);

                                /* In that case, the maximum time without signature has been passed, go to set ear_periodic_mode ON */
                                if (limit_exceeded || time_from_mpi_init >= dynais_timeout)
                                {
                                    // we must compute N here
                                    ear_periodic_mode = PERIODIC_MODE_ON;

                                    assert(dynais_timeout > 0);

                                    //mpi_calls_per_second = (uint)(total_mpi_calls/dynais_timeout);
                                    mpi_calls_per_second = (uint)avg_mpi_calls_per_second();

                                    traces_start();

                                    verbose_master(EARL_GUIDED_LVL, "Going to periodic mode after %lf secs: mpi calls in period %u.\n",
                                                   time_from_mpi_init,mpi_calls_per_second);

                                    ear_iterations = 0;

                                    states_begin_period(my_id, ear_event_l, (ulong) lib_period, 1);
                                    states_new_iteration(my_id, 1, ear_iterations, 1, 1, lib_period, 0);
                                } else
                                {
                                    /* We continue using dynais */
                                    ear_mpi_call_dynais_on(call_type, buf, dest);
                                }
                            }else
                            {	// We check the periodic mode every check_every mpi calls
                                ear_mpi_call_dynais_on(call_type, buf, dest);
                            }
                        }
                    }
                        break;
                    case DYNAIS_DISABLED:
                        /** That case means we have computed some signature and we have decided to set dynais disabled */
                        ear_mpi_call_dynais_off(call_type,buf,dest);
                        break;
                }
                }
                break;
            case PERIODIC_MODE_ON:
                /* EAR energy policy is called periodically */
                if ((mpi_calls_per_second > 0) && (total_mpi_calls%mpi_calls_per_second) == 0){
                    ear_iterations++;
                    //verbose_master(0, "Periodic mode on calling states_new_iteration");
                    states_new_iteration(my_id, 1, ear_iterations, 1, 1,lib_period,0);
                    #if MPI
                    /* We add a minimum number of iterations to guarantee the application has run for some time */
                    if (!limit_exceeded && (avg_mpi_calls_per_second() >= MAX_MPI_CALLS_SECOND)) limit_exceeded = 1;
                    #endif

                }
                break;
        }
    }
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

								ear_event_l = (ulong) ((((buf>>5)^dest)<<5)|call_type);
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
                dynais_calls++;

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
                                traces_end_period(ear_my_rank, my_node_id);
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
                                traces_new_n_iter(ear_my_rank, my_node_id, ear_event_l, ear_loop_size, ear_iterations);
																states_new_iteration(my_id, ear_loop_size, ear_iterations, ear_loop_level, ear_event_l, mpi_calls_per_loop,1);
																mpi_calls_per_loop=1;
																break;
												case END_LOOP:
																//debug("END_LOOP event %lu\n",ear_event_l);
																if (loop_with_signature) {
																				//debug("loop ends with %d iterations detected", ear_iterations);
																}
																loop_with_signature=0;
                                traces_end_period(ear_my_rank, my_node_id);
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
                                traces_new_n_iter(ear_my_rank, my_node_id, ear_event_l, ear_loop_size, ear_iterations);
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
                    traces_end_period(ear_my_rank, my_node_id);
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
                        traces_end_period(ear_my_rank, my_node_id);
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
                    traces_new_n_iter(ear_my_rank, my_node_id, loop_id, 1, ear_iterations);
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
static unsigned long long int last_mpis;
static timestamp_t periodic_time;
static ulong last_check = 0;
static uint first_exec_monitor = 1;


uint id_ovh_earl_monitor;

state_t earl_periodic_actions_init(void *no_arg)
{
    debug("RANK %d EARL periodic thread ON",ear_my_rank);
    timestamp_getfast(&periodic_time);
    seconds = 0;
    last_mpis = 0;
    if (!module_mpi_is_enabled() || (ear_guided == TIME_GUIDED)) {
        ear_periodic_mode = PERIODIC_MODE_ON;
        traces_start();
        ear_iterations = 0;
        states_begin_period(my_id, 1, (ulong) lib_period,1);
        mpi_calls_in_period = lib_period;
        if (!ear_whole_app) states_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period,0);
    }
    pthread_setname_np(pthread_self(), "EARL_monitor");

    overhead_suscribe("earl_monitor", &id_ovh_earl_monitor);

		configure_sigactions();

    return EAR_SUCCESS;
}

extern timestamp_t time_last_signature;

state_t earl_periodic_actions(void *no_arg)
{
	ulong period_elapsed, last_signature_elapsed;
	timestamp_t curr_periodic_time;
	uint new_ear_guided = ear_guided;

	// exiting is true when earl is in the ear_finalize function
	if (exiting) return EAR_SUCCESS;

	overhead_start(id_ovh_earl_monitor);

	if (ear_guided == TIME_GUIDED)
	{
		ear_iterations += ITERS_PER_PERIOD;
	}

	timestamp_getfast(&curr_periodic_time);
	period_elapsed = timestamp_diff(&curr_periodic_time, &periodic_time,TIME_SECS);
	last_signature_elapsed = timestamp_diff(&curr_periodic_time, &time_last_signature, TIME_SECS);

	debug("%s [%d] elapsed %lu: first_exec_monitor %d Iterations %d MPI %d time_guided %d periodic %u dynais(%d) calls %lu",
				node_name, my_node_id, period_elapsed, first_exec_monitor, ear_iterations, module_mpi_is_enabled(), ear_guided == TIME_GUIDED, ear_periodic_mode, dynais_enabled, dynais_calls);

	if (first_exec_monitor)
	{
		first_exec_monitor = 0;
		overhead_stop(id_ovh_earl_monitor);
		return EAR_SUCCESS;
	}

	if (!module_mpi_is_enabled() || (ear_guided == TIME_GUIDED))
	{
		//if (ear_guided == TIME_GUIDED) verbose_master(1, "(Time guided) elapsed %lu iteration iters %d period %u", period_elapsed, ear_iterations, lib_period);
		/* This lock is to avoid this function to be called at the same time than ear_finalize */
		state_t st = ear_trylock(&earl_th_locks);
		if (state_ok(st)){
			if (!exiting)
				states_new_iteration(my_id, ITERS_PER_PERIOD, ear_iterations, 1, 1, lib_period, 0);
			ear_unlock(&earl_th_locks);
		}else return EAR_SUCCESS;
		if (avg_mpi_calls_per_second() >= MAX_MPI_CALLS_SECOND) limit_exceeded = 1;
	}

	seconds = period_elapsed - last_check;
	if ((seconds >= 5) && (ear_periodic_mode != PERIODIC_MODE_ON))
	{
		last_check = period_elapsed;

		/* total_mpi_calls is computed per process in this file, not in metrics. It is used to automatically switch to time_guided */
		/* IO metrics are now computed in metrics.c as part of the signature */
		if (must_switch_to_time_guide(last_signature_elapsed, &new_ear_guided) != EAR_SUCCESS)
		{
			verbose_master(2, "Error estimating dynais/time guided");
		}

		verbose_master(EARL_GUIDED_LVL, "%d: Phase time_guided %d dynais_guided %d period %u", my_node_id, new_ear_guided==TIME_GUIDED, new_ear_guided==DYNAIS_GUIDED, lib_period);

		if (new_ear_guided != DYNAIS_GUIDED)
		{
			ear_periodic_mode = PERIODIC_MODE_ON;

			if (ear_guided != new_ear_guided)
			{
				if (earl_monitor->time_burst < earl_monitor->time_relax)
				{
					ITERS_PER_PERIOD = earl_monitor->time_burst / 1000;
					monitor_burst(earl_monitor, 0);
				}

				//verbose(0,"[%d]: Dynais guided --> time guided at time %lu",
				//        my_node_id, period_elapsed);

				states_comm_configure_performance_accuracy(&cconf, &perf_accuracy_min_time, &lib_period);

				mpi_calls_in_period = lib_period;
				ear_iterations = 1;
				states_begin_period(my_id, 1, (ulong) lib_period, 1);
			}
		}
		ear_guided = new_ear_guided;
	}

	overhead_stop(id_ovh_earl_monitor);
	return EAR_SUCCESS;
}


/************** Constructor & Destructor for NOt MPI versions **************/
#if !MPI 
void ear_fork_prolog()
{
}
void ear_fork_epilog()
{
}

static void  reset_earl_environment()
{
	verbose(0, "[%d] Resetting env", getpid());
	if (ear_lib_initialized)
	{
		ear_lib_initialized = 0;
		/* This lock is used per-process, we unlock just in case it was already locked */
		ear_unlock(&earl_th_locks);
		first_application = 0;
		verbose(0, "[%d] monitor dispose", getpid());
		monitor_dispose();
		verbose(0, "[%d] metrics lib reset", getpid());
		metrics_lib_reset();
		verbose(0, "[%d] report dispose", getpid());
		report_dispose(&rep_id);
		verbose(0, "[%d] release shared mem", getpid());
		release_shared_memory_areas();
		verbose(0, "[%d] disconnect", getpid());
		eards_new_process_disconnect();
	}
}


void ear_fork_new_child()
{
	pid_t pid = getpid();
	pid_t ppid = getppid();
  verbose(0, "EAR[%d]: New process %d!", ppid, pid);
  reset_earl_environment();

#if 0
	Disabled as this approach does not work
	char *gpu_master_opt_str = ear_getenv(FLAG_GPU_MASTER_OPTIMIZE);
	if (gpu_master_opt_str)
	{
		long gpu_master_opt = strtol(gpu_master_opt_str, NULL, 10);
		if (gpu_master_opt)
		{
			/* Set a 0 value to the environment variable. Last argument means 'overwrite'. */
			verbose(0, "EAR[%d] Process %d Setting %s to 0", ppid, pid, FLAG_GPU_MASTER_OPTIMIZE);
			setenv(FLAG_GPU_MASTER_OPTIMIZE, "0", 1);
		}
	}
#endif

  ear_init();

}

void ear_constructor()
{
    debug("Calling ear_init in ear_constructor %d", getpid());
    verbose(WF_SUPPORT_VERB,"Calling ear_init in ear_constructor %d", getpid());
    ear_init();
    pthread_atfork(ear_fork_prolog, ear_fork_epilog, ear_fork_new_child);
}
static int ear_already_finalize = 0;
void ear_destructor()
{
		if (ear_already_finalize) return;

    debug("Calling ear_finalize in ear_destructor %d",getpid());
    verbose(WF_SUPPORT_VERB,"Calling ear_finalize in ear_destructor %d",getpid());

#if ONLY_MASTER
    if (my_id) {
        return;
    }
#endif
		ear_already_finalize = 1;
    ear_finalize(EAR_SUCCESS);
}
#else
void ear_constructor()
{
}
void ear_destructor()
{
}
#endif


static state_t earl_log_backup(char *pathname, size_t pathname_size)
{
	state_t st = EAR_SUCCESS;
	ssize_t file_size = ear_file_size(pathname);
	if (file_size <= 0)
	{
		return st;
	}

	char *file_buff = (char *) calloc((size_t) file_size, sizeof(char));
	if (!file_buff)
	{
		return_msg(EAR_ERROR, Generr.alloc_error);
	}

	st = ear_file_read(pathname, file_buff, (size_t) file_size);
	if (state_fail(st)) {
		verbose_error("Reading file %s (%d): %s", pathname, intern_error_num, intern_error_str);
		free(file_buff);
		return_msg(st, "Backing up: File could not be read.");
	}

	// Create a copy of the file in <pathname>.bckp
	char *bckp_pathname = (char *) calloc((size_t) pathname_size + 6, sizeof(char));
	if (!bckp_pathname)
	{
		free(file_buff);
		return_msg(EAR_ERROR, Generr.alloc_error);
	}

	snprintf(bckp_pathname, (size_t) pathname_size + 5, "%s.bckp", pathname);
	st = ear_file_write(bckp_pathname, file_buff, (size_t) file_size);
	if (state_fail(st)) {
		verbose_error("Writing to file %s (%d): %s", bckp_pathname, intern_error_num, intern_error_str);
		free(file_buff);
		free(bckp_pathname);
		return_msg(st, "Backing up: File could not be read.");
	}

	free(file_buff);
	free(bckp_pathname);

	return st;
}

static void create_earl_verbose_files()
{
	char *verb_path = ear_getenv(FLAG_EARL_VERBOSE_PATH);
	if (verb_path) {
		/* Use a verbose file per process instead of only master */
		char *per_proc_v_file = ear_getenv(HACK_PROCS_VERB_FILES);

		if (per_proc_v_file) {
			per_proc_verb_file = atoi(per_proc_v_file);
			verbose_master(VEAR_INIT, "Per-process log: %u", per_proc_verb_file);	
		}

		if (masters_info.my_master_rank >= 0 || per_proc_verb_file) {
			if (( mkdir (verb_path, S_IRWXU|S_IRGRP|S_IWGRP) < 0) && (errno != EEXIST)) {
				per_proc_verb_file = 0;
				error("MR[%d] Verbose files folder cannot be created (%s)",
						masters_info.my_master_rank, strerror(errno));
			} else {
				using_verb_files = 1;

				char file_name[SZ_PATH];
				// Format: <verbose_path>/earl_log.<master_rank>.<local_rank>.<step>.<job>.<appid>
				snprintf(file_name, sizeof(file_name) - 1, "%s/earl-%d-%d-%u-%s-%d.log",
						verb_path, my_job_id, my_step_id, AID, node_name, main_pid);
				verbose_master(VEAR_INIT, "EARL log file: %s", file_name);

				int fd = open(file_name, O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP);
				if (fd != -1) {
					VERB_SET_FD(fd);
					DEBUG_SET_FD(fd);
					ERROR_SET_FD(fd);
					WARN_SET_FD(fd);
				} else {
					verbose_error_master("Log file cannot be created (%s).", strerror(errno));
				}
			}
		}
	}
}
