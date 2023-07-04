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
// #define SHOW_DEBUGS 1

/* 1 means invatid shared memory or invalid id (all processes), 
 * 2 means invatid shared memory or invalid id (half processes),
 * 3 eard not present . 0 means no fake (default) */
#define FAKE_EARD_ERROR "FAKE_EARD_ERROR"
//#define FAKE_LEARNING 1

/* 1 means master error, 2 means no master error at ear_finalize i. 0 no fake (default)*/
#define FAKE_PROCESS_ERRROR "FAKE_PROCESS_ERRROR"

#include <inttypes.h>
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
#include <sys/resource.h>



#include <common/config.h>
#include <common/config/config_env.h>
#include <common/colors.h>
#include <common/utils/overhead.h>
#include <common/system/folder.h>
#include <common/environment.h>
#include <library/common/verbose_lib.h>
#include <common/types/application.h>
#include <common/types/version.h>
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
#include <library/api/eard_dummy.h>
#include <library/api/mpi_support.h>

#include <library/dynais/dynais.h>
#include <library/tracer/tracer.h>
#include <library/states/states.h>
#include <library/models/models.h>
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
char shexternal_region_path[GENERIC_NAME];
ear_mgt_t *external_mgt;

#if DLB
#include <dlb_talp.h>

#define TALP_INFO  0 // TALP management verbose level

static suscription_t* earl_talp;
static FILE *talp_report = NULL;

static dlb_monitor_t *dlb_loop_monitor = NULL;
static dlb_monitor_t *dlb_app_monitor = NULL;

static uint talp_monitor = 1;

#endif // DLB


void ear_finalize(int exit_status);
static uint cancelled = 0;


#define MASTER_ID masters_info.my_master_rank

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

sem_t *lib_shared_lock_sem;
#if MPI_OPTIMIZED
uint ear_mpi_opt = 0;
uint ear_mpi_opt_num_nodes = 1;
mpi_opt_policy_t mpi_opt_symbols;
#endif

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
node_mgr_sh_data_t *node_mgr_job_info;

// Dynais
static dynais_call_t dynais;

suscription_t *earl_monitor;
state_t earl_periodic_actions(void *no_arg);
state_t earl_periodic_actions_init(void *no_arg);

#if MPI_OPTIMIZED
suscription_t *earl_mpi_opt;
state_t earl_periodic_actions_mpi_opt(void *no_arg);
state_t earl_periodic_actions_mpi_opt_init(void *no_arg);
#endif

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

#if DLB
/** Creates a subscription to the EARL periodic monitor. Burst and relax timing is the same (10 seconds).
 * Init subscription function is left to NULL, and the main rutine is earl_periodic_actions_talp.
 * If an error occurred, state_msg can be used.
 *
 * \param[out] sus The address of the subscription created.
 *
 * \return EAR_ERROR if an error ocurred registering the subscription.
 * \return EAR_SUCCESS otherwise. */
static state_t earl_create_talp_monitor(suscription_t *sus);

static state_t earl_periodic_actions_talp(void *p);
static state_t earl_periodic_actions_talp_init(void *p);

/** Verboses DLB's TALP API node collected metrics.
 * Just the master process prints something.
 *
 * \param verb_lvl Set the minimum verbose level for printing (still not used).
 * \param node_metrics The address where target data is stored. */
static void verbose_talp_node_metrics(int verb_lvl, dlb_node_metrics_t *node_metrics);


static state_t earl_periodic_actions_talp_finalize();
#endif // DLB


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
        verbose(1, "%30s: '%s'/'%lu'/'%lu'", "Node/job id/step_id",
                application.node_id, application.job.id,
                application.job.step_id);
        //verbose(2, "App/loop summary file: '%s'/'%s'", app_summary_path, loop_summary_path);
        verbose(1, "%30s: %u/%lu (%d)", "P_STATE/frequency (turbo)",
                EAR_default_pstate, application.job.def_f, ear_use_turbo);
        verbose(1, "%30s: %u/%d/%d/%.3f", "Tasks/nodes/ppn/node_ratio", my_size, num_nodes, ppnode, ratio_PPN);
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
                        verbose(2,"PID for master is %d", master_pid);
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
																ppnode = my_size / num_nodes;
												}
								}
								return;
				}
}

static uint clean_job_folder = 0;
void check_job_folder()
{
				char block_file_dir[MAX_PATH_SIZE];
				char *tmp=get_ear_tmp();
				uint ID = create_ID(my_job_id,my_step_id);
				int ret;


				snprintf(block_file_dir,sizeof(block_file_dir),"%s/%u",tmp, ID);
				// printf("check_job_folder:%s\n", block_file_dir);
				if (access(block_file_dir, W_OK) == 0){ 
								debug("JOB folder exists\n");
								return;
				}
				if (errno == ENOENT){
								debug("Job folder %s doesn't exist (%s), creating it", block_file_dir, strerror(errno));
								clean_job_folder = 1;
								ret = mkdir(block_file_dir, S_IRWXU);
								if ((ret < 0 ) && (errno != EEXIST)){
												/* What can we do here EAR OFF */
												// error("Create: Job folder cannot be create %s ", strerror(errno));		
								}else if (ret == 0){ debug("Folder created\n");}
				}
}

void cleanup_job_folder()
{
				char block_file_dir[MAX_PATH_SIZE];
				char *tmp=get_ear_tmp();
				uint ID = create_ID(my_job_id,my_step_id);
				snprintf(block_file_dir,sizeof(block_file_dir),"%s/%u",tmp, ID);
				debug("cleanup_job_folder: %s process %d\n", block_file_dir, clean_job_folder);
				if (clean_job_folder){
								debug("Folder needs to be removed from the library\n");
								folder_remove(block_file_dir);
				}else{ debug("Folder  doesn't need to be removed from the library\n");}
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
	uint current_jobs_in_node;
	#if SHOW_DEBUGS
	int total_size;
	#endif
	uint ID = create_ID(application.job.id, application.job.step_id);

	/* This file is going to be used to access the lib_shared region in exclusive mode */
	xsnprintf(block_file,sizeof(block_file),"%s/%u/.my_local_lock.%s",tmp, ID, application.job.user_id);
  if ((lib_shared_lock_fd = file_lock_create(block_file))<0){
  	error("Creating lock file for lib_shared_region hared memory:%s (%s)", block_file, strerror(errno));
  }

  xsnprintf(sem_file, sizeof(sem_file), "ear_semaphore_%u_%s", ID, node_name);
  lib_shared_lock_sem = sem_open(sem_file, O_CREAT, S_IRUSR|S_IWUSR, 1);
  if (lib_shared_lock_sem == SEM_FAILED){
    error("Creating sempahore %s (%s)", sem_file, strerror(errno));
  }
  verbose_master(2, "SEM(%s) created", sem_file);

	/* This section allocates shared memory for processes in same node */
    pid_t master_pid = getpid();
	verbose(2, "[%s] Master (PID: %d) creating shared regions for node synchronisation...",
            node_name, master_pid);

  xsnprintf(block_file,sizeof(block_file),"%s/%u/.my_local_lock_2.%s",tmp, ID, application.job.user_id);
  if ((bfd=file_lock_create(block_file))<0){
    error("Creating lock file for shared memory: %s (%s)",block_file, strerror(errno));
  }

  /* ****************************************************************************************
   * We have two shared regions, one for the node and other for all the processes in the app.
   * Depending on how EAR is compiled, only one signature per node is sent.
   * ************************************************************************************* */

  /*****************************************/
  /* Attaching/Create to lib_shared region */
  /*****************************************/

	if (get_lib_shared_data_path(tmp,ID, lib_shared_region_path) != EAR_SUCCESS) {
		error("Getting the lib_shared_region_path");
	} else {
		lib_shared_region=create_lib_shared_data_area(lib_shared_region_path);
		if (lib_shared_region==NULL){
			error("Creating the lib_shared_data region");
		}
		my_node_id=0;
		lib_shared_region->node_mgr_index = NULL_NODE_MGR_INDEX;
		lib_shared_region->earl_on = eard_ok || EARL_LIGHT;
		lib_shared_region->num_processes=1;
		lib_shared_region->num_signatures=0;
		lib_shared_region->master_rank = masters_info.my_master_rank;
		lib_shared_region->node_signature.sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
		lib_shared_region->job_signature.sig_ext  = (void *) calloc(1, sizeof(sig_ext_t));
#if DLB
    lib_shared_region->must_call_libdlb = 1;
#endif
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
				if (!eard_ok && !EARL_LIGHT) return;
				if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
								error("MPI_Barrier");
								return;
				}
#endif
#endif

	/* This region is for processes in the same node */

  /************************************************/
  /* Attaching/Create to shared_signatures region */
  /************************************************/
	if (get_shared_signatures_path(tmp,ID, shsignature_region_path)!=EAR_SUCCESS) {
    error("Getting the shsignature_region_path");
	} else {

		debug("Master node creating the signatures with %d processes", lib_shared_region->num_processes);

		sig_shared_region = create_shared_signatures_area(shsignature_region_path,
                lib_shared_region->num_processes);

		if (sig_shared_region==NULL){
			error("NULL pointer returned in create_shared_signatures_area");
		}
	}

	verbose_master(2, "Shared region for shared signatures created for %d processes",
            lib_shared_region->num_processes);

	sig_shared_region[my_node_id].master            = 1;
	sig_shared_region[my_node_id].pid               = master_pid;
	sig_shared_region[my_node_id].mpi_info.rank     = ear_my_rank;
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

				verbose_master(2,"max number of ppn is %d", masters_info.max_ppn);
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
				verbose_master(2,"Creating node_mgr region in tmp %s", tmp);
				if (nodemgr_job_init(tmp, &node_mgr_data) != EAR_SUCCESS){
								verbose_master(2,"Error creating node_mgr region");
				}
				ear_njob_t me;
				me.jid = application.job.id;
				me.sid = application.job.step_id;
				me.creation_time = time(NULL);
				CPU_ZERO(&me.node_mask);
				verbose_master(2,"Attaching to node_mgr region");
				if (nodemgr_attach_job(&me, &node_mgr_index) != EAR_SUCCESS){
								verbose_master(2,"Error attaching to node_mgr region");
				}
				lib_shared_region->node_mgr_index = node_mgr_index;
        if (msync(lib_shared_region, sizeof(lib_shared_data_t),MS_SYNC) < 0){
          verbose(1, "Error synchronizing EARL shared memory (%s)", strerror(errno));
        }
				verbose_master(2,"Attached! at pos %u", node_mgr_index);
				/* node_mgr_job_info maps other jobs data sharing the node */
				node_mgr_job_info = calloc(MAX_CPUS_SUPPORTED,sizeof(node_mgr_sh_data_t));
				init_earl_node_mgr_info();
				if (nodemgr_get_num_jobs_attached(&current_jobs_in_node) == EAR_SUCCESS){
								verbose_master(2,"There are %u jobs sharing the node ",current_jobs_in_node);
				}
				exclusive = (current_jobs_in_node == 1);
				verbose_jobs_in_node(3,node_mgr_data,node_mgr_job_info);

        /* Add here the initialization of the shared region to coordinate with external libraries such as DLB */
        get_ear_external_path(tmp, ID, shexternal_region_path);
        external_mgt = create_ear_external_shared_area(shexternal_region_path);
        verbose_master(2, "Created shared region for external libraries!");

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

				/* This function is executed by processes different than masters */
				/* they first join the Node shared memory */
				xsnprintf(block_file,sizeof(block_file),"%s/%u/.my_local_lock.%s",tmp, ID, application.job.user_id);
				if ((bfd = file_lock_create(block_file))<0){
								error("Creating lock file for shared memory:%s (%s)", block_file, strerror(errno));
				}
        xsnprintf(sem_file, sizeof(sem_file), "ear_semaphore_%u_%s", ID, node_name);
        lib_shared_lock_sem = sem_open(sem_file, O_CREAT, S_IRUSR|S_IWUSR, 1);
        if (lib_shared_lock_sem == SEM_FAILED){
          error("Creating/Attaching sempahore %s (%s)", sem_file, strerror(errno));
        }
				/* This link will be used to access the lib_shared_region lock file */
				lib_shared_lock_fd = bfd;
				if (get_lib_shared_data_path(tmp,ID,lib_shared_region_path)!=EAR_SUCCESS){
								error("Getting the lib_shared_region_path");
				}else{
#if MPI
								if (PMPI_Barrier(MPI_COMM_WORLD)!=MPI_SUCCESS){
												error("MPI_Barrier");
												return;
								}
#endif
								do{
												lib_shared_region = attach_lib_shared_data_area(lib_shared_region_path, &fd_libsh);
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
				verbose(3,"My node ID is %d/%d", my_node_id, lib_shared_region->num_processes);
				if (get_shared_signatures_path(tmp,ID, shsignature_region_path) != EAR_SUCCESS){
								error("Getting the shsignature_region_path  ");
				}else{
								sig_shared_region = attach_shared_signatures_area(shsignature_region_path,lib_shared_region->num_processes, &fd_sigsh);
								if (sig_shared_region == NULL){
												error("NULL pointer returned in attach_shared_signatures_area");
								}
				}
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
				verbose(3,"%d: Waiting for master to be attached to node_mgr region", my_node_id);
				if (msync(lib_shared_region, sizeof(lib_shared_data_t),MS_SYNC) < 0){
					verbose(1, "Error synchronizing EARL shared memory (%s)", strerror(errno));
				}
				/* Synchro: This value is set by the master */
				while (lib_shared_region->node_mgr_index == NULL_NODE_MGR_INDEX){
								usleep(100);
								if (msync(lib_shared_region, sizeof(lib_shared_data_t),MS_SYNC) < 0){
									verbose(1, "Error synchronizing EARL shared memory (%s)", strerror(errno));
								}
				}
				verbose(3,"%d:We are attached at position %u", my_node_id, lib_shared_region->node_mgr_index);
				if (nodemgr_find_job(&me, &node_mgr_index) != EAR_SUCCESS){
								verbose_master(2,"Error attaching to node_mgr region");
								node_mgr_index = NULL_NODE_MGR_INDEX;
				}
				//verbose(3,"Attached no-master! at pos %u", node_mgr_index);
				/* node_mgr_job_info maps other jobs data sharing the node */
				if (node_mgr_index != NULL_NODE_MGR_INDEX) node_mgr_job_info = calloc(MAX_CPUS_SUPPORTED,sizeof(node_mgr_sh_data_t));
				init_earl_node_mgr_info();
				if (nodemgr_get_num_jobs_attached(&current_jobs_in_node) == EAR_SUCCESS){
								verbose_master(2,"There are %u jobs sharing the node ",current_jobs_in_node);
				}
				exclusive = (current_jobs_in_node == 1);


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
    cn = ear_getenv(SCHED_STEP_NUM_NODES);
    if (cn != NULL) return atoi(cn);
    else return 1;
}

void attach_to_master_set(int master)
{
				int color;
#if MPI
				char *is_erun = ear_getenv(SCHED_IS_ERUN);
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
								if ((n > 0) && (n != sched_num_nodes())){ 
									if ((is_erun == NULL) || ((is_erun != NULL) && (atoi(is_erun) == 0))) flag = 0;
								}
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
				uint ID = create_ID(my_job_id,my_step_id);

#if USE_LOCK_FILES
				sprintf(fd_lock_filename, "%s/%u/.ear_app_lock.%u", get_ear_tmp(), ID, ID);
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
				/* print_settings_conf(system_conf);*/
				set_ear_power_policy(system_conf->policy);
				set_ear_power_policy_th(system_conf->settings[0]);
				set_ear_p_state(system_conf->def_p_state);
				set_ear_coeff_db_pathname(system_conf->lib_info.coefficients_pathname);
				set_ear_dynais_levels(system_conf->lib_info.dynais_levels);
				/* Included for dynais tunning */
				cdynais_window=ear_getenv(HACK_DYNAIS_WINDOW_SIZE);
				if ((cdynais_window!=NULL) && (system_conf->user_type==AUTHORIZED)){
								dynais_window=atoi(cdynais_window);
				}
				set_ear_dynais_window_size(dynais_window);
				set_ear_learning(system_conf->learning);
				dynais_timeout=system_conf->lib_info.dynais_timeout;
				lib_period=system_conf->lib_info.lib_period;
				check_every=system_conf->lib_info.check_every;
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


        if (s == SIGSEGV){ 
          verbose(0, "%d: EAR finalized by signal SIGSEGV", getpid());
          print_stack(verb_channel);
        }

        if (cancelled || sig_shared_region[my_node_id].exited) return;
        if (syscall(SYS_gettid) != main_pid) return;

				cancelled = 1;
				ear_finalize(EAR_CANCELLED);
				exit(0);
}

static void read_hack_env()
{
	char *hack_tmp = ear_getenv(HACK_EAR_TMP);
	char *hack_etc = ear_getenv(HACK_EAR_ETC);
	if (hack_tmp != NULL){
		set_ear_tmp(hack_tmp);
		setenv("EAR_TMP", hack_tmp, 1);
		verbose(0,"[%s] Using HACK tmp %s ", node_name, get_ear_tmp());
	}
	if (hack_etc != NULL){
		setenv("EAR_ETC", hack_etc, 1);
		verbose(0,"[%s] Using HACK etc %s ", node_name, hack_etc);
	}
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
	topology_t earl_topo;

         #if EAR_OFF
         return;
         #endif


				uint fake_eard_error = 0;
				char *cfake_eard_error = ear_getenv(FAKE_EARD_ERROR);
        char *cfake_force_shared_node = ear_getenv(FLAG_FORCE_SHARED_NODE);
				if (cfake_eard_error != NULL) fake_eard_error = atoi(cfake_eard_error);
        if (cfake_force_shared_node != NULL) fake_force_shared_node = atoi(cfake_force_shared_node);

        #if START_END_OVH
        ullong elap_eard_init;
        timestamp start_eard_init, end_eard_init;
        timestamp_getfast(&start_eard_init);
        #endif

				topology_init(&earl_topo);


        main_pid = syscall(SYS_gettid);


				if (ear_lib_initialized){
								verbose_master(1,"EAR library already initialized");
								return;
				}
                ear_lib_initialized = 1;
				VERB_SET_LV(0);

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

				gethostname(node_name, sizeof(node_name));
				strtok(node_name, ".");

				read_hack_env();

				// Fundamental data

       pid_t my_pid = getpid();
				debug("Running in %s process = %d", node_name, my_pid);

				tmp = get_ear_tmp();
				folder_t tmp_test;
				if (folder_open(&tmp_test, tmp) != EAR_SUCCESS){
								/* If tmp does not exist ear will be off but we need to set some tmp until we contact with other nodes */
								verbose_master(1,"ear tmp %s does not exists in node %s. EAR OFF", tmp, node_name);  
								set_ear_tmp("/tmp");
								tmp = get_ear_tmp();
				}


       // TODO: We could update the default freq an avoid the macro DEF_FREQ on each policy.
       if (ext_def_freq_str) {
                    ext_def_freq = (ulong) atol(ext_def_freq_str);
                    verbose_master(VEAR_INIT, "%sWARNING%s Externally defined default freq (kHz): %lu", COL_YLW, COL_CLR, ext_def_freq);
       }

			 debug("[%s] Using tmp %s ", node_name, get_ear_tmp());

				verb_level = get_ear_verbose();
				verb_channel=2;
				if ((tmp = ear_getenv(HACK_EARL_VERBOSE)) != NULL) {
								verb_level = atoi(tmp);
								VERB_SET_LV(verb_level);
				}

        // We print here the topology in order to do it only at verbose 2
        if (VERB_ON(2))
        {
                    if (ear_my_rank == 0)
                    {
                        topology_print(&earl_topo, 1);
                    }
        }

				set_ear_total_processes(my_size);
        #if FAKE_LEARNING
        ear_whole_app = 1;
        #else
				ear_whole_app = get_ear_learning_phase();
        #endif
#if MPI
				num_nodes = get_ear_num_nodes();
				ppnode = my_size / num_nodes;
#else
				num_nodes=1;
				ppnode=1;
#endif

				get_job_identification();
				check_job_folder();
				uint ID = create_ID(my_job_id,my_step_id);
				if (get_lib_shared_data_path(tmp, ID, lib_shared_region_path)==EAR_SUCCESS){
								unlink(lib_shared_region_path);
				}
				if (is_already_connected(my_job_id, my_step_id, my_pid)) {
								verbose_master(VEAR_INIT,
                                        "%sWARNING%s (Job/step/process) (%d/%d/%d) already conncted.\n",
                                        COL_RED, COL_CLR, my_job_id, my_step_id, my_pid);
								return;
				}
				// Getting if the local process is the master or not
				/* my_id reflects whether we are the master or no my_id == 0 means we are the master *****/
				my_id = get_local_id(node_name);


                int i_am_master = my_id == 0;
                int verb_pid_lvl = i_am_master ? 2 : 3;
                verbose(verb_pid_lvl, "%sEAR INIT%s [PID: %d] [MASTER: %d] [NODE: %s]",
                        COL_BLU, COL_CLR, my_pid, i_am_master, node_name);

				debug("attach to master %d", my_id);

				/* All masters connect in a new MPI communicator */
				attach_to_master_set(my_id==0);
#if MPI_OPTIMIZED
        ear_mpi_opt_num_nodes = sched_num_nodes();
#endif

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

				/* At this point we haven't connected yet with EARD, but, it these regions doesn't exist, we can not connect */
				/* Three potential reasons: 1) EARD is not running, 2) there is some version mistmatch between the earl-earplug-eard,
 				 * 3) Paths are not correct and we are looking at incorrect ear_tmp path */

				get_settings_conf_path(get_ear_tmp(),ID,system_conf_path);
				get_resched_path(get_ear_tmp(),ID, resched_conf_path);
				debug("[%s.%d] system_conf_path  = %s",node_name, ear_my_rank, system_conf_path);
				debug("[%s.%d] resched_conf_path = %s",node_name, ear_my_rank, resched_conf_path);
        #if FAKE_EAR_NOT_INSTALLED
        uint first_dummy = 1;
        #endif
				do{
	
	
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
					if ((system_conf == NULL) && (resched_conf == NULL) && !my_id){
							verbose(2, "[%s] Shared memory not present, creating dummy areas", node_name);
							create_eard_dummy_shared_regions(get_ear_tmp(), create_ID(my_job_id,my_step_id));
							fd_dummy = file_lock_create(dummy_areas_path);
					}else if ((system_conf == NULL) && (resched_conf == NULL) && my_id){
							/* If I'm not the master, I will wait */
							while(!file_is_regular(dummy_areas_path));
					}
				}while((system_conf == NULL) || (resched_conf == NULL));

				debug("[%s.%d] system_conf  = %p", node_name, ear_my_rank, system_conf);
				debug("resched_conf = %p", resched_conf);
				if (system_conf != NULL) debug("system_conf->id = %u", system_conf->id);
				debug("create_ID() = %u", create_ID(my_job_id,my_step_id)); // id*100+sid
				if (fake_eard_error) verbose_master(0, "Warning, executing fake eard errors: %u", fake_eard_error);

				uint valid_ID;
				if ((system_conf != NULL) && (resched_conf != NULL)){
					valid_ID = (system_conf->id == create_ID(my_job_id,my_step_id));
					if (fake_eard_error == 1){
						/* This is synthetic error to simulated the case the shared memory is not present OR invalid ID */
						valid_ID = 0;	
					}
					if (fake_eard_error == 2){
						if (ear_my_rank %2 ) valid_ID = 0;
					}
				}else {
					valid_ID = 0;
				}
				/* Updating configuration */
				if (valid_ID){
								debug("Updating the configuration sent by the EARD");
								update_configuration();	
				}else{
								eard_ok = 0;
								verbose_master(2,"EARD not detected");
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
				init_application(&application);
				application.is_mpi   	= 1;
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
				debug("App name is %s",ear_app_name);
				if (application.is_learning){
								verbose_master(VEAR_INIT,"Learning phase app %s p_state %lu\n",ear_app_name,application.job.def_f);
				}
				if (ear_getenv("LOGNAME")!=NULL) strcpy(application.job.user_id, ear_getenv("LOGNAME"));
				else strcpy(application.job.user_id,"No userid");
				debug("User %s",application.job.user_id);
				strcpy(application.node_id, node_name);
				strcpy(application.job.user_acc,my_account);
				application.job.id = my_job_id;
				application.job.step_id = my_step_id;

				debug("Starting job");
				// sets the job start_time
				start_job(&application.job);
				debug("Job time initialized");

				verbose_master(2,"EARD connection section EARD=%d",eard_ok);
#if SINGLE_CONNECTION
        if (!my_id && eard_ok) { // only the master will connect with eard
                    verbose(3, "%sConnecting with EAR Daemon (EARD) %d node %s (tmp = %s)%s",
                            COL_BLU, ear_my_rank, node_name, get_ear_tmp(), COL_CLR);

                    // Connect process with EARD
                    state_t ret = eards_connect(&application, my_node_id);

                    // Only for testing purposes
                    if (fake_eard_error == 3) {
                        ret = EAR_ERROR;
                    }

                    // Checking GPU support compatibility with EARD
                    uint eard_supports_gpu = gpu_is_supported(); // eards_gpu_support();
                    if (lib_supports_gpu != eard_supports_gpu) {
                        verbose(1, "%sERROR%s EARL (%u) and EARD (%u) differ on GPU support. Check your EAR installation.",
                                COL_YLW, COL_CLR, lib_supports_gpu, eard_supports_gpu);
                        ret = EAR_ERROR;
                    }

                    if (ret == EAR_SUCCESS) {
                        debug("%sRank %d connected with EARD%s", COL_BLU, ear_my_rank, COL_CLR);
                        notify_eard_connection(1);
                        mark_as_eard_connected(my_job_id, my_step_id, my_pid);
                    } else {
                        eard_ok = 0;
                        notify_eard_connection(0);
                    }
       }
#else
       if (eard_ok) { // all processes will connect with eard

                    char *col;

                    if (!my_id) {
                        col = COL_BLU;
                    } else {
                        col = COL_GRE;
                    }
                    verbose(2, "%sProcess %d PID %d Connecting with EAR Daemon (EARD) %d node %s%s",
                            col, my_node_id, my_pid, ear_my_rank, node_name, COL_CLR);

                    // Connect process with EARD
                    state_t ret = eards_connect(&application, ear_my_rank);

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

                    if (ret == EAR_SUCCESS) {

                        verbose(3, "%sRank %d connected with EARD%s", COL_BLU, ear_my_rank, COL_CLR);

                        if (!my_id) {
                            notify_eard_connection(1);
                            mark_as_eard_connected(my_job_id, my_step_id, my_pid);
                        }

                    } else {
                        verbose(2, "%sRank %d NOT connected with EARD%s %s",
                                COL_BLU,ear_my_rank,COL_CLR, state_msg);
                        eard_ok=0;
                        if (!my_id) notify_eard_connection(0);
                    }
       }
#endif
       /* At this point, the master has detected EARL must be set to off , we must notify the others processes about that */

				debug("Shared region creation section");
				if (!my_id) {
								debug("%sI'm the master, creating shared regions%s",COL_BLU,COL_CLR);
								create_shared_regions();
				} else {
								attach_shared_regions();
				}

        if (!eard_ok) {

                    /* Dummy areas path is used in case the EARD was not present */
                    if (!my_id && (fd_dummy >= 0)) {
                        int err;
                        err = file_unlock_master(fd_dummy, dummy_areas_path);
                        if (err) verbose_master(2, "Error cleaning dummy area %s (%s)", dummy_areas_path, strerror(err));
                    }

                    if (!EARL_LIGHT) {
                        verbose_master(2,"%s: EARL off because of EARD",node_name);
                        strcpy(application.job.policy," "); /* Cleaning policy */
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

				/* create verbose file for each job */
				char *verb_path = ear_getenv(FLAG_EARL_VERBOSE_PATH);
                if (verb_path != NULL) {

                    /* Use a verbose file per process instead of only master */
                    char *per_proc_v_file = ear_getenv(HACK_PROCS_VERB_FILES);

                    if (per_proc_v_file != NULL) {
                        per_proc_verb_file = atoi(per_proc_v_file);
                        verbose_master(2, "Per-process log: %u", per_proc_verb_file);	
                    }

                    if (masters_info.my_master_rank >= 0 || per_proc_verb_file) {
                        if (( mkdir (verb_path, S_IRWXU|S_IRGRP|S_IWGRP) < 0) && (errno != EEXIST)) {
                            per_proc_verb_file = 0;
                            error("MR[%d] Verbose files folder cannot be created (%s)",
                                    masters_info.my_master_rank, strerror(errno));
                        } else {
                            using_verb_files = 1;

                            char file_name[128];
                            // Format: <verbose_path>/earl_log.<master_rank>.<local_rank>.<step>.<job>
                            sprintf(file_name, "%s/earl_log.%d.%d.%d.%d",
                                    verb_path, lib_shared_region->master_rank,
                                    my_node_id, my_step_id, my_job_id);
                            verbose_master(VEAR_INIT, "EARL log file: %s", file_name);

                            int fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP);
                            if (fd != -1) {
                                VERB_SET_FD(fd);
                                DEBUG_SET_FD(fd);
                                ERROR_SET_FD(fd);
                                WARN_SET_FD(fd);
                            } else {
                                verbose_master(VEAR_INIT, "Log file cannot be created (%s)", strerror(errno));
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
								debug("Process %d not completes the initialization", my_node_id);
								return;
				}

                /* Based on family and model we set limits for this node */
                classify_init(&arch_desc.top);
                /* Getting limits for this node */
                get_classify_limits(&phases_limits);


		// Initializing metrics
                if (metrics_init(&arch_desc.top) != EAR_SUCCESS) {
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
                        sig_shared_region[my_node_id].num_cpus = num_cpus_in_mask(&ear_process_mask);
                    } else { 
                        sig_shared_region[my_node_id].affinity = 0;
                        verbose_master(2,"Affinity mask not defined for rank %d",masters_info.my_master_rank);
                    } 
                }
#endif
#ifdef SHOW_DEBUGS
				print_arch_desc(&arch_desc);
#endif
                if (!sched_getaffinity(my_pid, sizeof(cpu_set_t), &ear_process_mask))
                {
                    int proc_mask_cpu_count = num_cpus_in_mask(&ear_process_mask);
                    if (proc_mask_cpu_count > 0)
                    {
                        ear_affinity_is_set = 1;
                        sig_shared_region[my_node_id].cpu_mask = ear_process_mask;
                        sig_shared_region[my_node_id].num_cpus = proc_mask_cpu_count;
                    } else {
                        verbose(1, "%sWARNING%s CPU mask for thread ID %d has no CPUs.",
                                COL_YLW, COL_CLR, my_pid);
                    }

                    sig_shared_region[my_node_id].affinity = ear_affinity_is_set;
                } else {
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

                    debug("Computing number of cpus per job and node mask");
                    compute_job_cpus(lib_shared_region, sig_shared_region, &lib_shared_region->num_cpus);

                    cpu_set_t jmask;
                    aggregate_all_the_cpumasks(lib_shared_region, sig_shared_region, &jmask);

                    /* lib_shared_region is shared by all the processes in the same node */
                    lib_shared_region->node_mask = jmask;

                    /* node_mgr_data is shared with the eard */
                    node_mgr_data[node_mgr_index].node_mask = jmask;	

                    /* app_mgt_info is shared with the eard through the powermon_app_t */
                    app_mgt_info->node_mask = jmask;

                    debug("Node mask computed in");
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

        verbose_master(2,"Init energy models");
				init_power_models(system_conf->user_type, &system_conf->installation, &arch_desc);

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
				strcpy(application.job.app_id, ear_app_name);

				// Passing the frequency in KHz to MHz
				application.signature.def_f   = application.job.def_f = EAR_default_frequency;
				application.job.procs         = my_size;
				application.job.th            = system_conf->settings[0];
        application.signature.sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
        #if DCGMI
        if (metrics_dcgmi_enabled()){
          sig_ext_t *sig_ext = (sig_ext_t *)application.signature.sig_ext;
          sig_ext->dcgmis.dcgmi_sets = 0;
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
        #if SINGLE_CONNECTION
                if (masters_info.my_master_rank>=0) {
                    traces_init(system_conf, application.job.app_id, masters_info.my_master_rank,
                            my_id, num_nodes, my_size, ppnode);

                    traces_start();
                    traces_frequency(ear_my_rank, my_id, EAR_default_frequency);
                    traces_stop();

                    traces_mpi_init();
                }
                #else
                    traces_init(system_conf, application.job.app_id, ear_my_rank,
                            my_node_id, num_nodes, my_size, ppnode);

                    traces_start();
                    traces_frequency(ear_my_rank, my_node_id, EAR_default_frequency);
                    traces_stop();


                #endif

				// All is OK :D
				if (masters_info.my_master_rank==0) {
								verbose_master(1, "EAR initialized successfully.");
				}

				/* SIGTERM action for job cancellation */
				struct sigaction term_action;
				sigemptyset(&term_action.sa_mask);
				term_action.sa_handler = ear_lib_sig_handler;
				term_action.sa_flags = 0;
				sigaction(SIGTERM, &term_action, NULL);
				sigaction(SIGINT, &term_action, NULL);
				sigaction(SIGSEGV, &term_action, NULL);

       #if !EAR_OFF
				/* Monitor is used for applications with no mpi of few mpi calss per second. Gets control of signature computation
         * if neded*/

        if (!is_mpi_enabled())
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

        /* For code organization we have a separate subscription for MPI optimization */
#if MPI_OPTIMIZED
        if (is_mpi_enabled() && ear_mpi_opt)
        {
          earl_mpi_opt = suscription();
          earl_mpi_opt->call_init = earl_periodic_actions_mpi_opt_init;
          earl_mpi_opt->call_main = earl_periodic_actions_mpi_opt;
          earl_mpi_opt->time_relax = 200;
          earl_mpi_opt->time_burst = 200;
          if (monitor_register(earl_mpi_opt) != EAR_SUCCESS) verbose_master(2,"Monitor for MPI opt register fails! %s",state_msg);
          monitor_relax(earl_mpi_opt);
          verbose_master(2,"Monitor for MPI opt created");
        }
#endif

#if DLB
        if (state_fail(earl_create_talp_monitor(earl_talp)))
        {
            char *err_msg = "Error creating TALP monitor:";
            verbose(TALP_INFO, "[%d] %s %s", my_node_id, err_msg, state_msg);
        }
#endif // DLB
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

void ear_finalize(int exit_status)
{
				uint fake_error = 0;
				char *cfake = ear_getenv(FAKE_PROCESS_ERRROR);
				if (cfake != NULL) fake_error = atoi(cfake);

         #if EAR_OFF
     //    print_resource_usage();
         return;
         #endif

        #if START_END_OVH
        ullong elap_eard_init;
        timestamp start_eard_init, end_eard_init;
        timestamp_getfast(&start_eard_init);
        #endif


				verbose_master(2, "ear_finalize on=%d status %d ", lib_shared_region->earl_on, exit_status);
				if (fake_error) verbose_master(0, "Warning, executing fake errors in ear_finalize : %u", fake_error);
		
				if (fake_error){	
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
				if (!eard_ok && !EARL_LIGHT){ 
								cleanup_job_folder();
								return;
				}
				if (!lib_shared_region->earl_on){
								return;
				}
				if (!eards_connected() && !EARL_LIGHT && (masters_info.my_master_rank>=0)){
								lib_shared_region->earl_on = 0;
								cleanup_job_folder();
								sig_shared_region[my_node_id].exited = 1;
								return;
				}
				debug("%d: Finalizing", my_node_id);
#if USE_LOCK_FILES
				if (!my_id){
								debug("Application master releasing the lock %d %s", ear_my_rank,fd_lock_filename);
								file_unlock_master(fd_master_lock,fd_lock_filename);
								mark_as_eard_disconnected(my_job_id,my_step_id,getpid());
				}
#endif

#if SINGLE_CONNECTION
        if (masters_info.my_master_rank>=0){
								verbose_master(2,"Stopping periodic earl thread");	
								verbose_master(2,"Stopping tracing");
								traces_stop();
								traces_end(ear_my_rank, my_id, 0);

								traces_mpi_end();
				}
#else
        traces_stop();
        traces_end(ear_my_rank, my_node_id, 0);
        traces_mpi_end();
#endif
#if DLB
        earl_periodic_actions_talp_finalize();
#endif
        if (ear_guided == TIME_GUIDED){
          monitor_unregister(earl_monitor);
				  monitor_dispose();
				  debug("%d: monitor_disposed", my_node_id);
        }


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
								if (report_applications(&rep_id,&application,1) != EAR_SUCCESS){
												verbose_master(1,"Error reporting application");
								}
								report_mpi_application_data(&application);
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
				#if SINGLE_CONNECTION
				if (!my_id){ 
								debug("Disconnecting");
								if (eards_connected()) eards_disconnect();
				}
				#else
				if (eards_connected()){ 
					verbose(3, "Process %d disconnecting from EARD", my_node_id);
					eards_disconnect();
				}
				#endif

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
        verbose_master(2, "%sWARNING%s EAR not initialized.", COL_RED, COL_CLR);
        ear_init();
    }

    if (!eard_ok && !EARL_LIGHT) return;
    if (synchronize_masters == 0 ) return;
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

                                    #if SINGLE_CONNECTION
                                    if (masters_info.my_master_rank >= 0)
                                    #endif
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
                                #if SINGLE_CONNECTION
																if (masters_info.my_master_rank>=0)
                                #endif
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
                                #if SINGLE_CONNECTION
																if (masters_info.my_master_rank>=0) 
                                #endif
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
                                #if SINGLE_CONNECTION
																if (masters_info.my_master_rank>=0)  
                                #endif
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
                                #if SINGLE_CONNECTION
																if (masters_info.my_master_rank>=0)
                                #endif
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
#if SINGLE_CONNECTION
                if (masters_info.my_master_rank>=0) 
#endif
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
#if SINGLE_CONNECTION
                    if (masters_info.my_master_rank>=0) 
#endif
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
#if SINGLE_CONNECTION
                if (masters_info.my_master_rank>=0) 
#endif
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

#if MPI_OPTIMIZED
/* 0 = init, 1 = end, 2= diff */
static mpi_information_t my_mpi_stats[3];
static mpi_calls_types_t my_mpi_types[3];
#endif

uint id_ovh_earl_monitor;

state_t earl_periodic_actions_init(void *no_arg)
{
    debug("RANK %d EARL periodic thread ON",ear_my_rank);
    timestamp_getfast(&periodic_time);
    seconds = 0;
    last_mpis = 0;
    if (!is_mpi_enabled() || (ear_guided == TIME_GUIDED)) {
        ear_periodic_mode = PERIODIC_MODE_ON;
#if SINGLE_CONNECTION
        if (masters_info.my_master_rank>=0) 
#endif
        traces_start();
        ear_iterations = 0;
        states_begin_period(my_id, 1, (ulong) lib_period,1);
        mpi_calls_in_period = lib_period;
        if (!ear_whole_app) states_new_iteration(my_id, 1, ear_iterations, 1, 1,mpi_calls_in_period,0);
    }
    pthread_setname_np(pthread_self(), "EARL_monitor");

    overhead_suscribe("earl_monitor", &id_ovh_earl_monitor);

    return EAR_SUCCESS;
}

extern timestamp_t time_last_signature;
#if MPI_OPTIMIZED
extern ulong local_max_sync_block;
#endif

state_t earl_periodic_actions(void *no_arg)
{
    uint mpi_period;
    float mpi_in_period;
    ulong period_elapsed, last_signature_elapsed;
    timestamp_t curr_periodic_time;
    uint new_ear_guided = ear_guided;

    overhead_start(id_ovh_earl_monitor);

    if (ear_guided == TIME_GUIDED){
      ear_iterations += ITERS_PER_PERIOD;
    }

    timestamp_getfast(&curr_periodic_time);
    period_elapsed = timestamp_diff(&curr_periodic_time, &periodic_time,TIME_SECS);
    last_signature_elapsed = timestamp_diff(&curr_periodic_time, &time_last_signature, TIME_SECS);


     debug("%s [%d] elapsed %lu: first_exec_monitor %d Iterations %d MPI %d time_guided %d periodic %u dynais(%d) calls %lu",
              node_name, my_node_id, period_elapsed, first_exec_monitor, ear_iterations, is_mpi_enabled(), ear_guided == TIME_GUIDED, ear_periodic_mode, dynais_enabled, dynais_calls);

    if (first_exec_monitor) {
            first_exec_monitor = 0;
            overhead_stop(id_ovh_earl_monitor);
            return EAR_SUCCESS;
    }

    if (!is_mpi_enabled() || (ear_guided == TIME_GUIDED)) {
            //if (ear_guided == TIME_GUIDED) verbose_master(1, "(Time guided) elapsed %lu iteration iters %d period %u", period_elapsed, ear_iterations, lib_period);
            states_new_iteration(my_id, ITERS_PER_PERIOD, ear_iterations, 1, 1,lib_period,0);
            if (avg_mpi_calls_per_second() >= MAX_MPI_CALLS_SECOND) limit_exceeded = 1;
    }

    seconds = period_elapsed - last_check;
    if ((seconds >= 5) && (ear_periodic_mode != PERIODIC_MODE_ON)){
            last_check = period_elapsed;
            /* total_mpi_calls is computed per process in this file, not in metrics. It is used to automatically switch to time_guided */
            /* IO metrics are now computed in metrics.c as part of the signature */
            if (must_switch_to_time_guide(last_signature_elapsed, &new_ear_guided) != EAR_SUCCESS){
                verbose_master(2,"Error estimating dynais/time guided");
            }
            verbose_master(EARL_GUIDED_LVL, "%d: Phase time_guided %d dynais_guided %d period %u",my_node_id,new_ear_guided==TIME_GUIDED, new_ear_guided==DYNAIS_GUIDED,lib_period);
            if (new_ear_guided != DYNAIS_GUIDED){
                ear_periodic_mode = PERIODIC_MODE_ON;
                if (ear_guided != new_ear_guided){ 
                    if (earl_monitor->time_burst < earl_monitor->time_relax){
                        ITERS_PER_PERIOD = earl_monitor->time_burst/1000;
                        monitor_burst(earl_monitor, 0);
                    }
                    //verbose(0,"[%d]: Dynais guided --> time guided at time %lu",
                    //        my_node_id, period_elapsed);
                    lib_period = PERIOD;
                    ear_iterations = 0;
                    mpi_calls_in_period = lib_period;
                    lib_period = PERIOD;
                    ear_iterations = 1;
                    states_begin_period(my_id, 1, (ulong) lib_period,1);
               }
          }
          ear_guided = new_ear_guided;
    }
    overhead_stop(id_ovh_earl_monitor);
    return EAR_SUCCESS;
}

/* PERIODIC Actions for MPI optimization */
state_t earl_periodic_actions_mpi_opt_init(void *no_arg)
{
#if MPI_OPTIMIZED
    if (is_mpi_enabled() && ear_mpi_opt){
        memset(my_mpi_stats, 0, sizeof(mpi_information_t)*3);
        memset(my_mpi_types, 0, sizeof(mpi_calls_types_t)*3);
        metrics_start_cpupower();
    }
#endif
    return EAR_SUCCESS;

}
state_t earl_periodic_actions_mpi_opt(void *no_arg)
{
#if MPI_OPTIMIZED
    if (is_mpi_enabled() && (ear_mpi_opt || traces_are_on())) {
        /* Read */
        mpi_call_get_stats(&my_mpi_stats[1], NULL, -1);
        mpi_call_get_types(&my_mpi_types[1], NULL, -1);
        /* Diff */
        mpi_call_diff(     &my_mpi_stats[2], &my_mpi_stats[1], &my_mpi_stats[0]);
        my_mpi_stats[2].perc_mpi = (double) my_mpi_stats[2].mpi_time * 100.0 / (double) my_mpi_stats[2].exec_time;
        mpi_call_types_diff(&my_mpi_types[2], &my_mpi_types[1], &my_mpi_types[0]);
        /* Copy */
        memcpy(&my_mpi_stats[0], &my_mpi_stats[1], sizeof(mpi_information_t));
        memcpy(&my_mpi_types[0], &my_mpi_types[1], sizeof(mpi_calls_types_t));

        metrics_read_cpupower();
        uint new_pstate;
        if (mpi_opt_symbols.periodic != NULL)  mpi_opt_symbols.periodic(&new_pstate);
        /* We change the CPU freq after processing , is that a problem, warning !*/
        if (new_pstate)  policy_restore_to_mpi_pstate(new_pstate);

#if SINGLE_CONNECTION
        if ((MASTER_ID >= 0) && traces_are_on()) {
#else
        if (traces_are_on()) {
#endif // SINGLE_CONNECTION
                traces_generic_event(ear_my_rank, my_node_id , PERC_MPI, (ullong)my_mpi_stats[2].perc_mpi * 100);
                traces_generic_event(ear_my_rank, my_node_id , MPI_CALLS, (ullong)my_mpi_types[2].mpi_call_cnt);
                traces_generic_event(ear_my_rank, my_node_id , MPI_SYNC_CALLS, (ullong)my_mpi_types[2].mpi_sync_call_cnt);
                traces_generic_event(ear_my_rank, my_node_id , MPI_BLOCK_CALLS, (ullong)my_mpi_types[2].mpi_block_call_cnt);
                traces_generic_event(ear_my_rank, my_node_id , TIME_SYNC_CALLS, (ullong)my_mpi_types[2].mpi_sync_call_time);
                traces_generic_event(ear_my_rank, my_node_id , TIME_BLOCK_CALLS, (ullong)my_mpi_types[2].mpi_block_call_time);
                traces_generic_event(ear_my_rank, my_node_id , TIME_MAX_BLOCK, (ullong) local_max_sync_block);
                local_max_sync_block = 0;
        }
     }
#endif // MPI_OPTIMIZED
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

#if DLB
static state_t earl_create_talp_monitor(suscription_t *sus)
{
    if (is_mpi_enabled())
    {
        sus = suscription();

        sus->call_init = earl_periodic_actions_talp_init;
        sus->call_main = earl_periodic_actions_talp;

        sus->time_relax = 10000;
        sus->time_burst = 10000;

        if (state_fail(monitor_register(sus)))
        {
            char ret_msg[128];

            int ret = snprintf(ret_msg, sizeof ret_msg, "Registering subscription (%s)", state_msg);
            if (ret < sizeof ret_msg)
            {
                return_msg(EAR_ERROR, ret_msg);
            } else
            {
                return_msg(EAR_ERROR, state_msg);
            }
        }

        monitor_relax(sus);
        verbose(TALP_INFO, "DLB-TALP subscription created.");
    }

    return EAR_SUCCESS;
}


static state_t earl_periodic_actions_talp_init(void *p)
{
    dlb_app_monitor = DLB_MonitoringRegionRegister("app");
    dlb_loop_monitor = DLB_MonitoringRegionRegister("loop");

    if (dlb_app_monitor && dlb_loop_monitor)
    {
        DLB_MonitoringRegionStart(dlb_app_monitor);
        DLB_MonitoringRegionStart(dlb_loop_monitor);

        verbose_master(TALP_INFO, "DLB-TALP Init done");
    } else
    {
        dlb_app_monitor = dlb_loop_monitor = NULL;
        talp_monitor = 0;

        sem_wait(lib_shared_lock_sem);
        lib_shared_region->must_call_libdlb = 0;
        sem_post(lib_shared_lock_sem);

        verbose(TALP_INFO, "%sERROR%s [%d] registering regions.",
                COL_RED, COL_CLR, my_node_id);
    }

    return (talp_monitor ? EAR_SUCCESS : EAR_ERROR);
}


static state_t earl_periodic_actions_talp(void *p)
{
    if (!talp_monitor)
    {
        return EAR_SUCCESS;
    }

    int must_call_libdlb = 0;
    verbose(TALP_INFO + 1, "DLB-TALP Periodic action");

    int sem_ret_val = sem_wait(lib_shared_lock_sem);
    if (sem_ret_val < 0)
    {
        verbose(TALP_INFO, "%sWARNING%s Locking semaphor for DLB API status checking failed: %s",
                COL_YLW, COL_CLR, strerror(errno));
    }

    if (lib_shared_region->must_call_libdlb)
    {
        sig_shared_region[my_node_id].libdlb_calls_cnt++;
        debug("proc %d libdlb_cnt %d libflb_max %d",
              my_node_id, sig_shared_region[my_node_id].libdlb_calls_cnt, lib_shared_region->max_libdlb_calls);

        must_call_libdlb = 1;

        if (sig_shared_region[my_node_id].libdlb_calls_cnt > lib_shared_region->max_libdlb_calls)
        {
            lib_shared_region->max_libdlb_calls = sig_shared_region[my_node_id].libdlb_calls_cnt;

            if (msync(lib_shared_region, sizeof(lib_shared_data_t), MS_SYNC) < 0)
            {
                verbose(TALP_INFO, "%sError%s Memory sync. for lib shared region failed: %s",
                        COL_RED, COL_CLR, strerror(errno));
            }
        }
    }

    if (sem_post(lib_shared_lock_sem) < 0)
    {
        verbose(TALP_INFO, "%sWARNING%s Unlocking semaphor for DLB API status checking failed: %s",
                COL_YLW, COL_CLR, strerror(errno));
    }

    if (must_call_libdlb)
    {
        // TODO: This code is replicated on earl_periodic_actions_talp_finalize
        sig_ext_t sig_ext_tmp;
        if (state_ok(metrics_compute_talp_node_metrics(&sig_ext_tmp, dlb_loop_monitor)))
        {
            if (MASTER_ID >= 0)
            {
                sig_ext_t *node_sig_ext = (sig_ext_t *) lib_shared_region->node_signature.sig_ext;
                node_sig_ext->dlb_talp_node_metrics = sig_ext_tmp.dlb_talp_node_metrics;
                // TODO: Move in the outer condition scope
                // verbose_talp_node_metrics(TALP_INFO, &node_sig_ext->dlb_talp_node_metrics);
            }
        }

        if (dlb_loop_monitor)
        {
            DLB_MonitoringRegionReset(dlb_loop_monitor);
            DLB_MonitoringRegionStart(dlb_loop_monitor);
        }
    }
    return EAR_SUCCESS;
}


static void verbose_talp_node_metrics(int verb_lvl, dlb_node_metrics_t *node_metrics)
{
    if (VERB_ON(verb_lvl))
    {
        if (is_mpi_enabled())
        {
            if (MASTER_ID >= 0 && !talp_report)
            {
                char buff[GENERIC_NAME];
                int ret = snprintf(buff, sizeof buff, "%d-%d-%d-talp.csv",
                                   my_job_id, my_step_id, MASTER_ID);

                if (ret < sizeof buff)
                {
                    talp_report = fopen(buff, "a");
                    if (talp_report)
                    {
                        char talp_report_header[] = "time,node_id,total_useful_time,"
                                                    "total_mpi_time,max_useful_time,"
                                                    "max_mpi_time,load_balance,parallel_efficiency";

                        size_t header_len = strlen(talp_report_header);

                        size_t fwrite_ret = fwrite((const void *) talp_report_header,
                                                   sizeof(char), header_len,
                                                   talp_report);

                        if (fwrite_ret != header_len)
                        {
                            verbose(TALP_INFO, "%sWARNING%s Writing TALP report header: %d", COL_YLW, COL_CLR, errno)
                        }
                    } else
                    {
                        verbose(TALP_INFO, "%sERROR%s Creating and openning TALP report file: %d", COL_RED, COL_CLR, errno);
                    }
                } else
                {
                    verbose(TALP_INFO, "%sERROR%s TALP report file name did not created: increase the buffer size.", COL_RED, COL_CLR);
                }
            }
           
            if (MASTER_ID >= 0 && talp_report)
            {
                // Get the current time
                time_t curr_time = time(NULL);

                struct tm *loctime = localtime (&curr_time);

                char time_buff[32];
                strftime(time_buff, sizeof time_buff, "%c", loctime);

                char talp_node_metrics_sum[512];

                int ret = snprintf(talp_node_metrics_sum, sizeof talp_node_metrics_sum,
                                   "\n%s,%d,%"PRId64",%"PRId64",%"PRId64",%"PRId64",%f,%f",
                                   time_buff, node_metrics->node_id, node_metrics->total_useful_time,
                                   node_metrics->total_mpi_time, node_metrics->max_useful_time,
                                   node_metrics->max_mpi_time, node_metrics->load_balance,
                                   node_metrics->parallel_efficiency);

                if (ret < sizeof talp_node_metrics_sum)
                {
                    size_t char_count = strlen(talp_node_metrics_sum);
                    size_t fwrite_ret = fwrite((const void *) talp_node_metrics_sum, sizeof(char), char_count, talp_report);

                    if (fwrite_ret != char_count)
                    {
                        verbose(TALP_INFO, "%sERROR%s Writing TALP info: %d", errno);
                    }
                } else
                {
                    verbose(TALP_INFO, "%sERROR%s Writing the node metrics line. Increase the buffer size", COL_RED, COL_CLR);
                }
            } else if (MASTER_ID >= 0)
            {
                verbose(TALP_INFO, "%sWARNING%s TALP report file not created.", COL_YLW, COL_CLR);
            }
        }
    }
}


static state_t earl_periodic_actions_talp_finalize()
{
    if (!talp_monitor)
    {
        return EAR_SUCCESS;
    }
    int must_call_libdlb = 0;
    
    int sem_ret_val = sem_wait(lib_shared_lock_sem);
    if (sem_ret_val < 0)
    {
        verbose(TALP_INFO, "%sWARNING%s Locking semaphor for DLB API status checking failed: %s",
                COL_YLW, COL_CLR, strerror(errno));
    }
    debug("TALP monitor finalize");

    lib_shared_region->must_call_libdlb = 0;

    if (msync(lib_shared_region, sizeof(lib_shared_data_t), MS_SYNC) < 0)
    {
        verbose(TALP_INFO, "%sError%s Memory sync. for lib shared region failed: %s",
                COL_RED, COL_CLR, strerror(errno));
    }

    debug("libdlb_cnt %d max %d", sig_shared_region[my_node_id].libdlb_calls_cnt, lib_shared_region->max_libdlb_calls);
    if (sig_shared_region[my_node_id].libdlb_calls_cnt < lib_shared_region->max_libdlb_calls)
    {
        must_call_libdlb = 1;
        sig_shared_region[my_node_id].libdlb_calls_cnt++;
    }

    debug("unlocking semaphore");
    if (sem_post(lib_shared_lock_sem) < 0)
    {
        verbose(TALP_INFO, "%sWARNING%s Unlocking semaphor for DLB API status checking failed: %s",
                COL_YLW, COL_CLR, strerror(errno));
    }
    debug("semaphore unlocked");

    if (must_call_libdlb)
    {
        // TODO: This code is replicated on earl_periodic_actions_talp
        sig_ext_t sig_ext_tmp;
        if (state_ok(metrics_compute_talp_node_metrics(&sig_ext_tmp, dlb_loop_monitor)))
        {
            if (MASTER_ID >= 0)
            {
                sig_ext_t *node_sig_ext = (sig_ext_t *) lib_shared_region->node_signature.sig_ext;
                node_sig_ext->dlb_talp_node_metrics = sig_ext_tmp.dlb_talp_node_metrics;
                // TODO: Move in the outer condition scope
                // verbose_talp_node_metrics(TALP_INFO, &node_sig_ext->dlb_talp_node_metrics);
            }
        }
    }

    if (MASTER_ID >= 0 && talp_report)
    {
        if (fclose(talp_report) == EOF)
        {
            verbose(TALP_INFO, "%sERROR%s Closing TALP report file: %d", COL_RED, COL_CLR, errno);
        }
    }

    if (dlb_app_monitor)
    {
        DLB_MonitoringRegionStop(dlb_app_monitor);
        // DLB_MonitoringRegionReport(dlb_app_monitor);
    }

    debug("TALP monitor unregistering");
    monitor_unregister(earl_talp);

    return EAR_SUCCESS;
}
#endif // DLB
