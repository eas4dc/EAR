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

// #define SHOW_DEBUGS 1

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <float.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/colors.h>

#include <common/config.h>
#include <common/system/sockets.h>
#include <common/system/folder.h>
#include <common/messaging/msg_conf.h>

#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/application.h>
#include <common/types/periodic_metric.h>
#include <common/types/configuration/cluster_conf.h>
#include <metrics/cpufreq/cpufreq.h>
#include <management/imcfreq/imcfreq.h>
#include <common/hardware/hardware_info.h>
#include <management/cpufreq/frequency.h>
#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/node_metrics.h>
#include <daemon/eard_checkpoint.h>
#include <daemon/shared_configuration.h>
#include <daemon/local_api/node_mgr.h>
#include <daemon/eard_node_services.h>
#include <daemon/log_eard.h>
#if USE_GPUS
#include <daemon/gpu/gpu_mgt.h>
#endif

#if SYSLOG_MSG
#include <syslog.h>
#endif

#include <report/report.h>

/* Defined in eard.c */
extern uint report_eard ;
extern report_id_t rid;
extern state_t report_connection_status;

static pthread_mutex_t app_lock = PTHREAD_MUTEX_INITIALIZER;
static ehandler_t my_eh_pm;
static char *TH_NAME = "PowerMon";
state_t s;
/* Used to compute avg freq for jobs. Must be moved to powermon_app */
static cpufreq_t *freq_job2;
static uint       freq_count;

nm_t my_nm_id;
nm_data_t nm_init, nm_end, nm_diff, last_nm;

/** These variables comes from eard.c */
extern topology_t node_desc;
extern int eard_must_exit;
extern char ear_tmp[MAX_PATH_SIZE];
extern my_node_conf_t *my_node_conf;
extern cluster_conf_t my_cluster_conf;
extern eard_dyn_conf_t eard_dyn_conf;
extern policy_conf_t default_policy_context, energy_tag_context, authorized_context;
extern int application_id;
extern uint f_monitoring;
extern ulong current_node_freq;
extern char nodename[MAX_PATH_SIZE];

/* This vector has 1 entry per job and numc is the number of contexts created for this job */
static job_context_t jobs_in_node_list[MAX_CPUS_SUPPORTED];


extern state_t report_connection_status;
static char shmem_path[MAX_PATH_SIZE];
static int fd_powermon = -1;
static int fd_periodic = -1;
static energy_data_t c_energy;

/* Powercap */
static energy_data_t e_pmon_begin, e_pmon_end, e_pmon;
static power_data_t p_pmon;


static ulong * powermon_freq_list;
static int powermon_num_pstates;
static periodic_metric_t current_sample;
static double last_power_reported = 0;
static ulong  last_node_power     = 0;
dom_power_t pdomain;
cpu_set_t in_jobs_mask;

/***************** JOBS in NODE ***************************/
void init_jobs_in_node()
{
	memset(jobs_in_node_list, 0, sizeof(job_context_t)*MAX_CPUS_SUPPORTED);
}

uint is_job_in_node(job_id id, job_context_t **jc)
{
	uint i = 0; 
	while ((i < MAX_CPUS_SUPPORTED) && (jobs_in_node_list[i].id != id)) i++;
	if (i < MAX_CPUS_SUPPORTED) *jc = &jobs_in_node_list[i];
	return (i < MAX_CPUS_SUPPORTED);
}

void add_job_in_node(job_id id)
{
	/* Find the job */
	uint i = 0;
	while ((i < MAX_CPUS_SUPPORTED) && (jobs_in_node_list[i].id != id)) i++;
	if (i < MAX_CPUS_SUPPORTED){ 
		jobs_in_node_list[i].numc++;
		return;
	}
	/* New job */
  i = 0;
	while ((i < MAX_CPUS_SUPPORTED) && (jobs_in_node_list[i].id)) i++;
	if (i == MAX_CPUS_SUPPORTED){
		debug("Error, not job free detected, this should not happen");
		return;
	}
	jobs_in_node_list[i].id = id;
	jobs_in_node_list[i].numc = 1;	
}

void del_job_in_node(job_id id)
{
	/* Find the job */
  uint i = 0;
  while ((i < MAX_CPUS_SUPPORTED) && (jobs_in_node_list[i].id != id)) i++;
  if (i < MAX_CPUS_SUPPORTED){  
		jobs_in_node_list[i].numc--;
		if (jobs_in_node_list[i].numc == 0) jobs_in_node_list[i].id = 0;
	}else{
		debug("Error, job not found %lu", id);
	}
	return;
	
}

void clean_cpus_in_jobs()
{
	for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++){
		jobs_in_node_list[i].num_cpus = 0;
	}
}


uint num_jobs_in_node()
{
	uint total = 0;
	for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++){
		if (jobs_in_node_list[i].id) total++;
	}
	return total;
}

void verbose_jobs_in_node(uint vl)
{
	for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++){
		if (jobs_in_node_list[i].numc){
			verbose(vl, "JOB[%u] id=%lu contexts %u cpus %u", i, jobs_in_node_list[i].id, jobs_in_node_list[i].numc, jobs_in_node_list[i].num_cpus);
		}
	}
}

/****************** CONTEXT MANAGEMENT ********************/
#define NULL_SID 4294967294
powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];
int max_context_created = 0;
int num_contexts = 0;

void init_contexts() {
  int i;
  for (i = 0; i < MAX_NESTED_LEVELS; i++) current_ear_app[i] = NULL;
	init_jobs_in_node();
}

int find_empty_context() {
  int i = 0, pos = -1;
  debug("find_empty_context ");
  /* No more contexts supported */
  if (num_contexts == MAX_NESTED_LEVELS) return -1;
  while ((i < MAX_NESTED_LEVELS) && (pos < 0)) {
    if (current_ear_app[i] == NULL) {
      pos = i;
    } else i++;
  }
  return pos;
}

int find_context_for_job(job_id id, job_id sid) {
  int i = 0, pos = -1;
  debug("find_context_for_job %lu %lu", id, sid);
  while ((i <= max_context_created) && (pos < 0)) {
    if ((current_ear_app[i] != NULL) && (current_ear_app[i]->app.job.id == id) &&
      (current_ear_app[i]->app.job.step_id == sid)) {
      pos = i;
    } else i++;
  }
  if (pos >= 0) {
    debug("find_context_for_job %lu,%lu found at pos %d", id, sid, pos);
  } else {
    debug("find_context_for_job %lu,%lu not found", id, sid);
  }
  return pos;
}
/* This function is called at job or step end */
void end_context(int cc)
{
  int ID,i;
  int new_max;
  debug("end_context %d", cc);
  if (current_ear_app[cc] != NULL) {
    free_energy_data(&current_ear_app[cc]->energy_init);
    ID = create_ID(current_ear_app[cc]->app.job.id,current_ear_app[cc]->app.job.step_id);

		del_job_in_node(current_ear_app[cc]->app.job.id);		

    get_settings_conf_path(ear_tmp,ID,  shmem_path);
    settings_conf_shared_area_dispose(shmem_path);
    get_resched_path(ear_tmp, ID, shmem_path);
    resched_shared_area_dispose(shmem_path);
    get_app_mgt_path(ear_tmp,ID, shmem_path);
    app_mgt_shared_area_dispose(shmem_path);
    get_pc_app_info_path(ear_tmp, ID, shmem_path);
    pc_app_info_shared_area_dispose(shmem_path);
    cpufreq_data_free(&current_ear_app[cc]->freq_job1, &current_ear_app[cc]->freq_diff);
    free(current_ear_app[cc]);
    current_ear_app[cc] = NULL;
    num_contexts--;

    if (cc == max_context_created){
      new_max = 0;
      for (i=0; i<= max_context_created;i++){
        if ((current_ear_app[i] != NULL) && (i>new_max)) new_max = i;
      }
      debug("New max_context_created %d",new_max);
      max_context_created = new_max;
    }
  }
}
/* This function will be called when job ends to be sure there is not steps pending to clean*/
void clean_job_contexts(job_id id) {
  int i, cleaned = 0;
  debug("clean_job_contexts for job %lu", id);
  for (i = 0; i <= max_context_created; i++) {
    if (current_ear_app[i] != NULL) {
      if (current_ear_app[i]->app.job.id == id) {
        cleaned++;
        end_context(i);
      }
    }
  }
  debug("%d contexts cleaned", cleaned);
}

/* This function is called to remove contexts belonging to jobs different than new one */
void clean_contexts_diff_than(job_id id) {
  int i;
  debug("clean_contexts_diff_than %lu", id);
  for (i = 0; i <= max_context_created; i++) {
    if (current_ear_app[i] != NULL) {
      if (current_ear_app[i]->app.job.id != id) {
        debug("Clearning context %d from job %lu", i, current_ear_app[i]->app.job.id);
        end_context(i);
      }
    }
  }
}

int select_last_context() {
  int i = 0, pos = -1;
  while ((i < MAX_NESTED_LEVELS) && (pos < 0)) {
    if (current_ear_app[i] != NULL) pos = i;
    else i++;
  }
  if (pos >= 0) {
    debug("select_last_context selects context %d (%lu,%lu)", pos, current_ear_app[pos]->app.job.id,
        current_ear_app[pos]->app.job.step_id);
  } else {
    debug("select_last_context no contexts actives");
  }
  return pos;
}


int new_context(job_id id, job_id sid,int *cc)
{
  char shmem_path[GENERIC_NAME];
  uint ID;
  int ccontext;

  *cc = -1;
  debug("new_context for %lu %lu", id, sid);

  if (num_contexts == MAX_NESTED_LEVELS) {
    error("Panic: Maximum number of levels reached in new_job %d", num_contexts);
    return EAR_ERROR;
  }
  int pos = find_empty_context();
  if (pos < 0) {
    error("Panic: Maximum number of levels reached in new_job %d", num_contexts);
    return EAR_ERROR;
  }
  ccontext = pos;
  *cc = ccontext;
  current_ear_app[ccontext] = (powermon_app_t *) calloc(1,sizeof(powermon_app_t));
  if (current_ear_app[ccontext] == NULL) {
    *cc = -1;
    error("Panic: malloc returns NULL for current context");
    return EAR_ERROR;
  }
  if (ccontext > max_context_created ) max_context_created = ccontext;
  alloc_energy_data(&current_ear_app[ccontext]->energy_init);
  /* This info will be overwritten later */
  current_ear_app[ccontext]->app.job.id = id;
  current_ear_app[ccontext]->app.job.step_id = sid;
  debug("New context created at pos %d", ccontext);

	add_job_in_node(id);

  /* We must create per jobid,stepid shared memory regions */
  ID = create_ID(id,sid);
  get_settings_conf_path(ear_tmp, ID, shmem_path);
  debug("Settings for new context placed at %s",shmem_path);

  current_ear_app[ccontext]->settings = create_settings_conf_shared_area(shmem_path);
  if (current_ear_app[ccontext]->settings == NULL) {
    error("Error creating shared memory between EARD & EARL for (%lu,%lu)",id,sid);
    return EAR_ERROR;
  }

  /* Context 0 is the default one */
  memcpy(current_ear_app[ccontext]->settings,current_ear_app[0]->settings,sizeof(settings_conf_t));
  get_resched_path(ear_tmp, ID, shmem_path);
  debug("Resched path for new context placed at %s",shmem_path);
  current_ear_app[ccontext]->resched = create_resched_shared_area(shmem_path);
  if (current_ear_app[ccontext]->resched == NULL) {
    error("Error creating resched shared memory between EARD & EARL for (%lu,%lu)",id,sid);
    return EAR_ERROR;
  }
  current_ear_app[ccontext]->resched->force_rescheduling = 0;
  cpufreq_data_alloc(&current_ear_app[ccontext]->freq_job1, &current_ear_app[ccontext]->freq_diff);
  /* This area is for application data */
  get_app_mgt_path(ear_tmp, ID, shmem_path);
  debug("App_MGR for new context placed at %s",shmem_path);
  current_ear_app[ccontext]->app_info = create_app_mgt_shared_area(shmem_path);
  if (current_ear_app[ccontext]->app_info == NULL){
    error("Error creating shared memory between EARD & EARL for app_mgt (%lu,%lu)",id,sid);
    return EAR_ERROR;
  }
  /* Default value for app_info */
  get_pc_app_info_path(ear_tmp,ID, shmem_path);
  debug("PC_app_info for new context placed at %s",shmem_path);
  current_ear_app[ccontext]->pc_app_info = create_pc_app_info_shared_area(shmem_path);
  if (current_ear_app[ccontext]->pc_app_info == NULL){
    error("Error creating shared memory between EARD & EARL for pc_app_infoi (%lu,%lu)",id,sid);
    return EAR_ERROR;
  }

  current_ear_app[ccontext]->pc_app_info->cpu_mode = powercap_get_cpu_strategy();
#if USE_GPUS
  current_ear_app[ccontext]->pc_app_info->gpu_mode = powercap_get_gpu_strategy();
#endif
	/* accum_ps is the accumulated power signature when sharing nodes */
	memset(&(current_ear_app[ccontext]->accum_ps), 0, sizeof(accum_power_sig_t));
	current_ear_app[ccontext]->exclusive = (num_jobs_in_node() == 1);

  /* Default value for pc_app_info */
  debug("Context created for (%lu,%lu), num_contexts %d max_context_created %d",id,sid,num_contexts+1,max_context_created);
  num_contexts++;
  return EAR_SUCCESS;
}


/****************** END CONTEXT MANAGEMENT ********************/


void create_job_area(uint ID)
{
	int ret;
	char job_path[MAX_PATH_SIZE];
	xsnprintf(job_path,sizeof(job_path),"%s/%u",ear_tmp,ID);
	verbose(VJOBPMON+1,"Creating job_path: %s", job_path);
	mode_t old_mask = umask((mode_t) 0);
	ret = mkdir(job_path,0777);
	if (ret < 0 ){
		verbose(VJOBPMON+1,"Cannot create jobpath (%s)",strerror(errno));
	}
	umask(old_mask);	
	return;
}

void clean_job_area(uint ID)
{
	char job_path[MAX_PATH_SIZE];
    xsnprintf(job_path,sizeof(job_path),"%s/%u",ear_tmp,ID);
	folder_remove(job_path);
}

powermon_app_t *get_powermon_app() {
  return current_ear_app[max_context_created];

}

static void PM_my_sigusr1(int signun) {
	verbose(VNODEPMON, " thread %u receives sigusr1", (uint) pthread_self());
  powercap_end();

	pthread_exit(0);
}

static void PM_set_sigusr1() {
	sigset_t set;
	struct sigaction sa;
	sigfillset(&set);
	sigdelset(&set, SIGUSR1);
	sigdelset(&set, SIGALRM);
	pthread_sigmask(SIG_SETMASK, &set, NULL); // unblocking SIGUSR1 and SIGALRM for this thread
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = PM_my_sigusr1;
	sa.sa_flags = 0;
	if (sigaction(SIGUSR1, &sa, NULL) < 0)
		error(" doing sigaction of signal s=%d, %s", SIGUSR1, strerror(errno));

}

void reset_shared_memory(powermon_app_t *pmapp)
{
  verbose(VJOBPMON+2,"reset_shared_memory");
  policy_conf_t *my_policy;
  my_policy = get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
	if (my_policy == NULL){
		verbose(VJOBPMON+1,"Error, my_policy is null in reset_shared_memory when looking for default policy %d",my_cluster_conf.default_policy);
	}
  pmapp->resched->force_rescheduling= 0;
  pmapp->settings->user_type        = NORMAL;
  pmapp->settings->learning         = 0;
  pmapp->settings->lib_enabled      = 1;
  pmapp->settings->policy           = my_cluster_conf.default_policy;
  pmapp->settings->def_freq         = frequency_pstate_to_freq(my_policy->p_state);
  pmapp->settings->def_p_state      = my_policy->p_state;
  pmapp->settings->max_freq         = frequency_pstate_to_freq(my_node_conf->max_pstate);
  strncpy(pmapp->settings->policy_name, my_policy->name, 64); 
  memcpy(pmapp->settings->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
  pmapp->settings->min_sig_power    = my_node_conf->min_sig_power;
  pmapp->settings->max_sig_power    = my_node_conf->max_sig_power;
  pmapp->settings->max_power_cap    = my_node_conf->powercap;
  pmapp->settings->report_loops     = my_cluster_conf.database.report_loops;
  pmapp->settings->max_avx512_freq  = my_node_conf->max_avx512_freq;
  pmapp->settings->max_avx2_freq    = my_node_conf->max_avx2_freq;
  memcpy(&pmapp->settings->installation,&my_cluster_conf.install,sizeof(conf_install_t));
  copy_ear_lib_conf(&pmapp->settings->lib_info,&my_cluster_conf.earlib);
  verbose(VJOBPMON,"Shared memory regions for job set with default values");

}

void powermon_update_node_mask(cpu_set_t *nodem)
{
  powermon_app_t *pmapp;
  int i;
  debug("Updating the mask");
	CPU_ZERO(nodem);
  /* reconfiguring running apps */
  for (i=0; i <=  max_context_created;i++){
    pmapp = current_ear_app[i];
    if (pmapp !=NULL){
			aggregate_cpus_to_mask(nodem, &pmapp->app_info->node_mask);
    }
  }

}

void powermon_new_configuration()
{
  powermon_app_t *pmapp;
  int i;
  debug("New configuration for jobs");
  /* reconfiguring running apps */
  for (i=0; i <=  max_context_created;i++){
    pmapp = current_ear_app[i];
    if (pmapp !=NULL){
      reset_shared_memory(pmapp);
      if (pmapp->app.is_mpi) pmapp->resched->force_rescheduling = 1;
    }
  }
}

state_t powermon_create_idle_context()
{
  job_id jid = 0,sid = 0;
  uint ID;
  int cc = 0;
  powermon_app_t *pmapp;
  char shmem_path[GENERIC_NAME];

  verbose(VJOBPMON,"Creating idle context");
  current_ear_app[cc] = calloc(1,sizeof(powermon_app_t));
  if (current_ear_app[cc] == NULL) return EAR_ERROR;

	create_job_area(0);

  pmapp = current_ear_app[cc];
  max_context_created = 0;
  num_contexts = 1;
  alloc_energy_data(&pmapp->energy_init);
  pmapp->app.job.id = jid;
  pmapp->app.job.step_id = sid;

  cpufreq_data_alloc(&pmapp->freq_job1, &pmapp->freq_diff);
  cpufreq_data_alloc(&freq_job2, NULL);
  cpufreq_count_devices(no_ctx, &freq_count);

  ID = create_ID(jid,sid);
  /* Shared memory regions for default context */
  get_settings_conf_path(ear_tmp, ID, shmem_path);
  verbose(VJOBPMON,"Settings for new context placed at %s",shmem_path);
  pmapp->settings = create_settings_conf_shared_area(shmem_path);
	if (pmapp->settings == NULL){
		verbose(VJOBPMON,"Error creating shared memory region for earl shared data");
	}
  get_resched_path(ear_tmp, ID,shmem_path);
  verbose(VJOBPMON,"Resched area for new context placed at %s",shmem_path);
  pmapp->resched = create_resched_shared_area(shmem_path);
	if (pmapp->resched == NULL){
		verbose(VJOBPMON,"Error creating shared memory region for re-scheduling data");
	}
  reset_shared_memory(pmapp);
  get_app_mgt_path(ear_tmp, ID, shmem_path);
  verbose(VJOBPMON,"App- Mgr area for new context placed at %s",shmem_path);
  pmapp->app_info = create_app_mgt_shared_area(shmem_path);
	if (pmapp->app_info == NULL){
		verbose(VJOBPMON,"Error creating app mgr shared region ");
	}
  app_mgt_new_job(pmapp->app_info);

  get_pc_app_info_path(ear_tmp, ID, shmem_path);
  verbose(VJOBPMON,"App PC area for new context placed at %s",shmem_path);
  pmapp->pc_app_info = create_pc_app_info_shared_area(shmem_path);
	if (pmapp->pc_app_info == NULL){
		verbose(VJOBPMON,"Error creating app pc shared region");
	}
  pmapp->pc_app_info->cpu_mode = powercap_get_cpu_strategy();
#if USE_GPUS
  pmapp->pc_app_info->gpu_mode = powercap_get_gpu_strategy();
#endif
  set_null_loop(&pmapp->last_loop);
  verbose(VJOBPMON,"Idle context created");
  return EAR_SUCCESS;
}


void copy_powermon_app(powermon_app_t *dest, powermon_app_t *src) {
  if (dest == NULL || src == NULL) {
    error("copy_powermon_app: NULL prointers provided");
    return;
  }
  debug("copy_powermon_app");
  dest->job_created = src->job_created;
  dest->local_ids = src-> local_ids;
  copy_energy_data(&dest->energy_init,&src->energy_init);
  copy_application(&(dest->app), &(src->app));
  dest->governor.min_f = src->governor.min_f;
  dest->governor.max_f = src->governor.max_f;
  strcpy(dest->governor.name, src->governor.name);
  dest->current_freq = src->current_freq;
  dest->sig_reported = src->sig_reported;
  /* Should we copy the mem content ? */
  memcpy(dest->settings , src->settings, sizeof(settings_conf_t)); // new
  dest->resched->force_rescheduling = src->resched->force_rescheduling;
  memcpy(dest->app_info , src->app_info,sizeof(app_mgt_t));
  memcpy(dest->pc_app_info , src->pc_app_info,sizeof(pc_app_info_t));
  cpufreq_data_copy(dest->freq_job1, src->freq_job1);
}


#if DB_FILES
char database_csv_path[PATH_MAX];

void report_application_in_file(application_t *app)
{
	int ret1,ret2;
	ret2 = append_application_text_file(database_csv_path, app,app->is_mpi, 1);
	if (ret1 == EAR_SUCCESS && ret2 == EAR_SUCCESS)
	{
		debug( "application signature correctly written");
	} else {
		error("ERROR while application signature writing");
	}
}

void form_database_paths()
{
	char island[PATH_MAX];
	mode_t mode,old_mode;

	old_mode=umask(0);	
	mode=S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
	sprintf(island,"%s/island%d",my_cluster_conf.DB_pathname,my_node_conf->island);
	if ((mkdir(island,mode)<0 ) && (errno!=EEXIST)){
		error("DB island cannot be created for %s node %s",island,nodename);
		sprintf(database_csv_path, "/dev/null");
	}else{
		sprintf(database_csv_path, "%s/%s.csv", island, nodename);
	}
	umask(old_mode);

	verbose(VCONF, "Using DB plain-text file: %s", database_csv_path);
}

#else
#define form_database_paths()
#define report_application_in_file(x)
#endif


/*
*
*
*	BASIC FUNCTIONS 
*
*/
void job_init_powermon_app(powermon_app_t *pmapp, ehandler_t *ceh, application_t *new_app, uint from_mpi) {
	state_t s;
	verbose(VJOBPMON,"job_init_powermon_app init %lu/%lu ", new_app->job.id, new_app->job.step_id);
  if (pmapp == NULL){
    verbose(VJOBPMON,"Error, job_init_powermon_app with null pmapp");
    return;
  }
  debug("New context for N jobs scenario selected %lu/%lu",pmapp->app.job.id,pmapp->app.job.step_id);

	pmapp->job_created = !from_mpi;
	copy_application(&pmapp->app, new_app);
	strcpy(pmapp->app.node_id, nodename);
	time(&pmapp->app.job.start_time);
	pmapp->app.job.start_mpi_time = 0;
	pmapp->app.job.end_mpi_time = 0;
	pmapp->app.job.end_time = 0;
	pmapp->app.job.procs = new_app->job.procs;
	// reset signature
	signature_init(&pmapp->app.signature);
	init_power_signature(&pmapp->app.power_sig);
	pmapp->app.power_sig.max_DC_power = 0;
	pmapp->app.power_sig.min_DC_power = 10000;
	pmapp->app.power_sig.def_f = pmapp->settings->def_freq;
	// Initialize energy
	read_enegy_data(ceh, &c_energy);
	copy_energy_data(&pmapp->energy_init, &c_energy);
	// CPU Frequency
	state_assert(s, cpufreq_read(no_ctx, pmapp->freq_job1), );
	verbose(VJOBPMON,"job_init_powermon_app end %lu/%lu is_mpi %d",pmapp->app.job.id,pmapp->app.job.step_id, pmapp->app.is_mpi);
}

void job_end_powermon_app(powermon_app_t *pmapp,ehandler_t *ceh)
{
    power_data_t app_power;
    state_t s;
    double elapsed;

    if (pmapp == NULL){
        verbose(VJOBPMON,"Error, job_init_powermon_app with null pmapp");
        return;
    }
    debug("job_end_powermon_app for %lu/%lu",pmapp->app.job.id,pmapp->app.job.step_id);

    alloc_power_data(&app_power);
    time(&pmapp->app.job.end_time);
    if (pmapp->app.job.end_time == pmapp->app.job.start_time) {
        pmapp->app.job.end_time = pmapp->app.job.start_time + 1;
    }
    elapsed = difftime(pmapp->app.job.end_time, pmapp->app.job.start_time);
    read_enegy_data(ceh, &c_energy);
    compute_power(&pmapp->energy_init, &c_energy, &app_power);
    if (pmapp->exclusive){
        verbose(VJOBPMON, "Computing power signature. Job executed in exclusive mode");
        pmapp->app.power_sig.DC_power = accum_node_power(&app_power);

        /* Filter */
        if (pmapp->app.power_sig.DC_power > my_node_conf->max_sig_power) pmapp->app.power_sig.DC_power = my_node_conf->max_sig_power;
        if (pmapp->app.power_sig.DC_power < my_node_conf->min_sig_power) pmapp->app.power_sig.DC_power = my_node_conf->min_sig_power;

        /* Compute max and min */
        pmapp->app.power_sig.max_DC_power = ear_max(pmapp->app.power_sig.max_DC_power, pmapp->app.power_sig.max_DC_power);
        pmapp->app.power_sig.min_DC_power = ear_min(pmapp->app.power_sig.min_DC_power, pmapp->app.power_sig.max_DC_power);

        /* DRAM and PCK */
        pmapp->app.power_sig.DRAM_power = accum_dram_power(&app_power);
        pmapp->app.power_sig.PCK_power = accum_cpu_power(&app_power);
        // CPU Frequency
        state_assert(s, cpufreq_read_diff(no_ctx, freq_job2, pmapp->freq_job1, pmapp->freq_diff, &pmapp->app.power_sig.avg_f), );
        ulong lcpus;
        if (pmapp->app.is_mpi) {
            pstate_freqtoavg(pmapp->app_info->node_mask, pmapp->freq_diff, freq_count, &pmapp->app.power_sig.avg_f, &lcpus);
        }
    }else{
        if (pmapp->app.is_mpi){
            verbose(VJOBPMON, "Computing power signature. Job executed in NOT exclusive mode. Using EARL signature");
            pmapp->app.power_sig.DC_power 	= pmapp->app.signature.DC_power;
            pmapp->app.power_sig.DRAM_power = pmapp->app.signature.DRAM_power;
            pmapp->app.power_sig.PCK_power	= pmapp->app.signature.PCK_power;
            pmapp->app.power_sig.avg_f        = pmapp->app.signature.avg_f;
        }else{
            verbose(VJOBPMON, "Computing power signature. Job executed in NOT exclusive mode");
            pmapp->app.power_sig.DC_power 	= pmapp->accum_ps.DC_energy / elapsed;
            pmapp->app.power_sig.DRAM_power = pmapp->accum_ps.DRAM_energy/ elapsed;
            pmapp->app.power_sig.PCK_power	= pmapp->accum_ps.PCK_energy / elapsed;
            pmapp->app.power_sig.avg_f        = pmapp->accum_ps.avg_f / elapsed;
        }
        if (pmapp->app.power_sig.DC_power > my_node_conf->max_sig_power) pmapp->app.power_sig.DC_power = my_node_conf->max_sig_power;
        if (pmapp->app.power_sig.DC_power < my_node_conf->min_sig_power) pmapp->app.power_sig.DC_power = my_node_conf->min_sig_power;
        pmapp->app.power_sig.max_DC_power = ear_max(pmapp->accum_ps.max, pmapp->app.power_sig.DC_power); 
        pmapp->app.power_sig.min_DC_power = ear_min(pmapp->accum_ps.min, pmapp->app.power_sig.DC_power);
    }
    pmapp->app.power_sig.time = difftime(app_power.end, app_power.begin);

    // debug("Computed avg cpu %lu for %lu cpus job %lu/%lu", &pmapp->app.power_sig.avg_f, &lcpus, pmapp->app.job.id,pmapp->app.job.step_id);
    // nominal is still pending

    // Metrics are not reported in this function
    free_power_data(&app_power);
    debug("job_end_powermon_app done for %lu/%lu",pmapp->app.job.id,pmapp->app.job.step_id);

}

void report_powermon_app(powermon_app_t *app) {
    // We can write here power information for this job
    if (app == NULL) {
        error("report_powermon_app: NULL app");
        return;
    }
    if (app->sig_reported == 0) {
        verbose(VJOBPMON + 1, "Reporting not mpi application");
        app->app.is_mpi = 0;
    }

    verbose_application_data(0, &app->app);
    clean_db_power_signature(&app->app.power_sig, my_node_conf->max_sig_power);
    report_application_in_file(&app->app);
    report_connection_status = report_applications(&rid,&app->app,1);
}

policy_conf_t per_job_conf;

policy_conf_t *configure_context(uint user_type, energy_tag_t *my_tag, application_t *appID) {
    policy_conf_t *my_policy;
    int p_id;
    if (appID == NULL) {
        error("configure_context: NULL application provided, setting default values");
        my_policy = &default_policy_context;
        return my_policy;
    }
    verbose(VJOBPMON,"configuring policy for user %u policy %s freq %lu th %lf is_learning %u is_mpi %d force_freq %d",user_type,appID->job.policy,appID->job.def_f,appID->job.th,appID->is_learning,appID->is_mpi,my_cluster_conf.eard.force_frequencies);
    switch (user_type){
        case NORMAL:
            appID->is_learning=0;
            debug("Policy requested %s",appID->job.policy);
            p_id=policy_name_to_nodeid(appID->job.policy, my_node_conf);
            debug("Policy %s is ID %d",appID->job.policy,p_id);
            /* Use cluster conf function */
            if (p_id!=EAR_ERROR){
                my_policy=get_my_policy_conf(my_node_conf,p_id);
                if (!my_policy->is_available){
                    debug("User type %d is not alloweb to use policy %s",user_type,appID->job.policy);
                    my_policy=get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
                }
                copy_policy_conf(&per_job_conf,my_policy);
                my_policy=&per_job_conf;
            }else{
                debug("Invalid policy %s ",appID->job.policy);
                my_policy=get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
                if (my_policy==NULL){
                    error("Error Default policy configuration returns NULL,invalid policy, check ear.conf (setting MONITORING)");
                    authorized_context.p_state=1;
                    int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);
                    if (mo_pid != EAR_ERROR)
                        authorized_context.policy = mo_pid;
                    else
                        authorized_context.policy=MONITORING_ONLY;
                    authorized_context.settings[0]=0;
                }else{
                    copy_policy_conf(&authorized_context,my_policy);
                }
                my_policy=&authorized_context;
            }
            if (my_policy==NULL){
                error("Default policy configuration returns NULL,invalid policy, check ear.conf");
                my_policy=&default_policy_context;
            }
            break;

        case AUTHORIZED:
            if (appID->is_learning){
                int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);
                if (mo_pid != EAR_ERROR)
                    authorized_context.policy = mo_pid;
                else
                    authorized_context.policy=MONITORING_ONLY;
                if (appID->job.def_f){ 
                    if (frequency_is_valid_frequency(appID->job.def_f)) authorized_context.p_state=frequency_closest_pstate(appID->job.def_f);
                    else authorized_context.p_state=1;
                } else authorized_context.p_state=1;

                authorized_context.settings[0]=0;
                my_policy=&authorized_context;
            }else{
                p_id=policy_name_to_nodeid(appID->job.policy, my_node_conf);
                if (p_id!=EAR_ERROR){
                    my_policy=get_my_policy_conf(my_node_conf,p_id);
                    authorized_context.policy=p_id;
                    if (appID->job.def_f){ 
                        verbose(VJOBPMON+1,"Setting freq to NOT default policy p_state ");
                        if (frequency_is_valid_frequency(appID->job.def_f)) authorized_context.p_state=frequency_closest_pstate(appID->job.def_f);
                        else authorized_context.p_state=my_policy->p_state;
                    }else{ 
                        verbose(VJOBPMON+1,"Setting freq to default policy p_state %u",my_policy->p_state);
                        authorized_context.p_state=my_policy->p_state;	
                    }
                    if (appID->job.th>0) authorized_context.settings[0]=appID->job.th;
                    else authorized_context.settings[0]=my_policy->settings[0];
                    strcpy(authorized_context.name, my_policy->name);
                    my_policy=&authorized_context;
                }else{
                    verbose(VJOBPMON,"Authorized user is executing not defined/invalid policy using default %d",my_cluster_conf.default_policy);
                    my_policy=get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
                    if (my_policy==NULL){
                        error("Error Default policy configuration returns NULL,invalid policy, check ear.conf (setting MONITORING)");
                        authorized_context.p_state=1;
                        int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);
                        if (mo_pid != EAR_ERROR)
                            authorized_context.policy = mo_pid;
                        else
                            authorized_context.policy=MONITORING_ONLY;
                        authorized_context.settings[0]=0;
                    }else{
                        print_policy_conf(my_policy);		
                        copy_policy_conf(&authorized_context,my_policy);
                    }
                    my_policy = &authorized_context;
                }
            }
            break;
        case ENERGY_TAG:
            appID->is_learning=0;
            int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);
            if (mo_pid != EAR_ERROR)
                authorized_context.policy = mo_pid;
            else
                energy_tag_context.policy=MONITORING_ONLY;
            energy_tag_context.p_state=my_tag->p_state;
            energy_tag_context.settings[0]=0;
            my_policy=&energy_tag_context;
            break;
    }
    if ((!appID->is_mpi) && (!my_cluster_conf.eard.force_frequencies)){
        my_policy->p_state=frequency_closest_pstate(frequency_get_cpu_freq(0));
        verbose(VJOBPMON,"Application is not using ear and force_frequencies=off, frequencies are not changed pstate=%u",my_policy->p_state);

    }else{
        /* We have to force the frequency */
        ulong f;
        frequency_set_userspace_governor_all_cpus();
        f = frequency_pstate_to_freq(my_policy->p_state);
        cpu_set_t not_in_jobs_mask;
        powermon_update_node_mask(&in_jobs_mask);
        cpus_not_in_mask(&not_in_jobs_mask, &in_jobs_mask);
        uint cpus_used = num_cpus_in_mask(&in_jobs_mask);
        uint cpus_free = num_cpus_in_mask(&not_in_jobs_mask);

        /* Setting the default CPU freq for the new job */
        frequency_set_with_mask(&not_in_jobs_mask, f);
        verbose(VJOBPMON, "Setting userspace governor pstate=%u to %u cpus (used %u)", my_policy->p_state, cpus_free, cpus_used);

        /* Setting the default IMC freq by default in all the sockets */
        if (state_fail(mgt_imcfreq_set_auto(no_ctx))){
            verbose(VJOBPMON, "Error when setting IMCFREQ by default");
        }
        verbose(VJOBPMON,"Restoring IMC frequency" );
#if USE_GPUS
        if (my_node_conf->gpu_def_freq > 0){
            /** Can we know which GPUS are used by this job?? */
            gpu_mgr_set_freq_all_gpus(my_node_conf->gpu_def_freq);
            verbose(VJOBPMON,"Restoring GPU frequency to %lu", my_node_conf->gpu_def_freq);
        }
#endif
    }
    appID->is_mpi = 0;
    debug("context configured");
    return my_policy;
}


/*
 *
 *
 *	MPI PART
 *
 */

void powermon_loop_signature(job_id jid,job_id sid,loop_t *loops)
{
    int cc;
    powermon_app_t *pmapp;
    cc = find_context_for_job(jid, sid);
    if (cc < 0){
        verbose(VJOBPMON,"powermon_loop_signature and no current context for %lu/%lu",jid,sid);
        return;
    }
    pmapp = current_ear_app[cc];
    debug("New loop signature for %lu/%lu, iterations %lu",pmapp->app.job.id,pmapp->app.job.step_id,loops->total_iterations);
    copy_loop(&pmapp->last_loop,loops);
    return;
}

// That functions controls the mpi init/end of jobs . These functions are called by eard when application executes mpi_init/mpi_finalized and contacts eard

void powermon_mpi_init(ehandler_t *eh, application_t *appID) {
    int cc;
    ulong jid,sid;
    jid = appID->job.id;
    sid = appID->job.step_id;
    powermon_app_t *pmapp;

    if (appID == NULL) {
        error("powermon_mpi_init: NULL appID");
        return;
    }
    verbose(VJOBPMON + 1, "powermon_mpi_init job_id %lu step_id %lu (is_mpi %u)", appID->job.id, appID->job.step_id,
            appID->is_mpi);
    cc = find_context_for_job(jid, sid);
    if (cc < 0){
        verbose(VJOBPMON,"powermon_mpi_init and context not found job_id %lu/%lu, creating automatically",appID->job.id, appID->job.step_id);
        powermon_new_job(NULL,eh, appID, 1,  0);
        cc = find_context_for_job(jid, sid);
        if (cc < 0){
            verbose(VJOBPMON,"ERROR: Context not created!");
            return;
        }
    }
    pmapp = current_ear_app[cc];
    pmapp->local_ids++;
    debug("powermon_mpi_init for %lu/%lu Total local connections %d",pmapp->app.job.id,pmapp->app.job.step_id, pmapp->local_ids);
    /* The job was already connected, this is only a new local connection */
    if (pmapp->local_ids > 1){
        return;
    }
    // MPI_init : It only changes mpi_init time, we don't need to acquire the lock
    start_mpi(&pmapp->app.job);
    pmapp->app.is_mpi = 1;
    // save_eard_conf(&eard_dyn_conf);
}

void powermon_mpi_finalize(ehandler_t *eh, ulong jid,ulong sid) 
{
    int cc;
    powermon_app_t *pmapp;
    /* jid,sid can finish in a different order than expected */
    debug("powermon_mpi_finalize %lu %lu", jid, sid);
    cc = find_context_for_job(jid, sid);
    if (cc < 0) {
        error("powermon_mpi_finalize %lu,%lu and no context created for it", jid, sid);
        return;
    }
    pmapp = current_ear_app[cc];
    pmapp->local_ids--;
    if (pmapp->local_ids > 0) return;
    /* We set ccontex to the specific one */

    verbose(VJOBPMON + 1, "powermon_mpi_finalize (%lu,%lu)", pmapp->app.job.id, pmapp->app.job.step_id);
    end_mpi(&pmapp->app.job);
    set_powercapstatus_mode(AUTO_CONFIG);
    if (!pmapp->job_created) {  // If the job is not submitted through slurm, end_job would not be submitted
        powermon_end_job(eh, pmapp->app.job.id, pmapp->app.job.step_id, 0);
    }
    debug("powermon_mpi_finalize done for %lu,%lu",jid,sid);

}

/*
 *
 * TASK MGT
 *
 */

void powermon_new_task(new_task_req_t *newtask)
{
    powermon_app_t *pmapp;
    int cc;
    verbose(VJOBPMON,"powermon_new_task %lu/%lu/%d/%u", newtask->jid, newtask->sid, newtask->pid, newtask->num_cpus);
    cc = find_context_for_job(newtask->jid, newtask->sid);
    if (cc < 0){
        verbose(0,"New task and Context for %lu/%lu not created", newtask->jid, newtask->sid);
        return;
    }
    pmapp = current_ear_app[cc];
    verbose(VJOBPMON+1, "New task for %lu/%lu PID = %d num_cpus %u", newtask->jid, newtask->sid, newtask->pid, newtask->num_cpus);
    pmapp->plug_num_cpus += newtask->num_cpus;
    aggregate_cpus_to_mask(&pmapp->plug_mask, &newtask->mask);		
    verbose(VJOBPMON, "Total cpus in %lu/%lu = %u", newtask->jid, newtask->sid, pmapp->plug_num_cpus);
}

/*
 *
 *
 *   JOB PART
 *
 */

/* This functiono is called by dynamic_configuration thread when a new_job command arrives */

void powermon_new_job(powermon_app_t *pmapp,ehandler_t *eh, application_t *appID, uint from_mpi, uint is_job) {
    int i;
    int ccontext;
    job_context_t* alloc;
    // New application connected
    if (appID == NULL) {
        error("powermon_new_job: NULL appID");
        return;
    }
    uint new_app_id = create_ID(appID->job.id, appID->job.step_id);
    /* Creating ID folder */
    create_job_area(new_app_id);
    energy_tag_t *my_tag;
    policy_conf_t *my_policy;
    ulong f;
    uint user_type;
    verbose(VJOBPMON , "*******************");
    verbose(VJOBPMON, "%spowermon_new_job (%lu,%lu)%s", COL_BLU,appID->job.id, appID->job.step_id,COL_CLR);
    if (powermon_is_idle()) powercap_idle_to_run();
    set_powercapstatus_mode(AUTO_CONFIG);
    powercap_new_job();
    if (pmapp == NULL){
        if (new_context(appID->job.id, appID->job.step_id,&ccontext) != EAR_SUCCESS) {
            error("Maximum number of contexts reached, no more concurrent jobs supported");
            return;
        }
        pmapp = current_ear_app[ccontext];
    }
    /* Saving the context */
    /* We cannot assume all the node will have the same freq: PENDING */
    pmapp->current_freq = frequency_get_cpu_freq(0);
    pmapp->is_job = is_job;
    /* Returns the number of contexts for this jobid, if is 1 is an srun without sbacth/or salloc (or equivalent on other schedulers) */
    if (is_job_in_node(appID->job.id, &alloc)){
        /* It's an allocation by itself */
        if (alloc->numc == 1) pmapp->is_job = 1;
    }
    verbose(VJOBPMON, "Is new job %d", pmapp->is_job);
    get_governor(&pmapp->governor);
    debug("Saving governor %s", pmapp->governor.name);
    /* Setting userspace */
    user_type = get_user_type(&my_cluster_conf, appID->job.energy_tag, appID->job.user_id, appID->job.group_id,
            appID->job.user_acc, &my_tag);
    verbose(VJOBPMON + 1, "New job USER type is %u", user_type);
    if (my_tag != NULL) print_energy_tag(my_tag);
    /* Given a user type, application, and energy_tag, my_policy is the cofiguration for this user and application */
    /* This function sets CPU frequency and GPu frequency */
    my_policy = configure_context(user_type, my_tag, appID);
    verbose(VJOBPMON,"Node configuration for policy %u p_state %d th %lf is_mpi %d",my_policy->policy,my_policy->p_state,my_policy->settings[0], appID->is_mpi);
    /* Updating info in shared memory region */
    f = frequency_pstate_to_freq(my_policy->p_state);
    /* Info shared with the job */
    pmapp->settings->id             = new_app_id;
    pmapp->settings->user_type      = user_type;
    if (user_type == AUTHORIZED) pmapp->settings->learning = appID->is_learning;
    else pmapp->settings->learning  = 0;
    pmapp->settings->lib_enabled    = (user_type!=ENERGY_TAG);
    verbose(0, "User type %u, etag %u lib_enabled %u", user_type, ENERGY_TAG, pmapp->settings->lib_enabled);
    pmapp->settings->policy         = my_policy->policy;
    strcpy(pmapp->settings->policy_name,  my_policy->name);
    pmapp->settings->def_freq       = f;
    pmapp->settings->def_p_state    = my_policy->p_state;
    pmapp->settings->island         = my_node_conf->island;
    /*  Max CPU and IMC pstates */
    pmapp->settings->cpu_max_pstate = my_node_conf->cpu_max_pstate;
    pmapp->settings->imc_max_pstate = my_node_conf->imc_max_pstate;
    strcpy(pmapp->settings->tag, my_node_conf->tag);
    pmapp->resched->force_rescheduling = 0;
    memcpy(pmapp->settings->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
    set_null_loop(&pmapp->last_loop);
    /* End app configuration */
    appID->job.def_f                  = pmapp->settings->def_freq;


    app_mgt_new_job(pmapp->app_info);
    /* Requested frequencies must be done per JOB: PENDING to apply the affinity mask */
    pcapp_info_new_job(pmapp->pc_app_info);
    for (i = 0; i < MAX_CPUS_SUPPORTED; i++) pmapp->pc_app_info->req_f[i] = f;
    // verbose_frequencies(node_desc.cpu_count, pmapp->pc_app_info->req_f);
#if USE_GPUS
    pmapp->pc_app_info->num_gpus_used = gpu_mgr_num_gpus();
    for (uint gpuid = 0; gpuid < pmapp->pc_app_info->num_gpus_used ;gpuid++) pmapp->pc_app_info->req_gpu_f[gpuid] = my_node_conf->gpu_def_freq;
#endif
    powercap_set_app_req_freq();
    while (pthread_mutex_trylock(&app_lock));
    job_init_powermon_app(pmapp,eh, appID, from_mpi);

    /* We must report the energy beforesetting the job_id: PENDING */
    new_job_for_period(&current_sample, appID->job.id, appID->job.step_id);
    pthread_mutex_unlock(&app_lock);
    //save_eard_conf(&eard_dyn_conf);
    verbose(VJOBPMON_BASIC , "%sJob created ctx %d (%lu,%lu) is_mpi %d procs %lu def_cpuf %lu%s", COL_BLU, ccontext, current_ear_app[ccontext]->app.job.id,
            current_ear_app[ccontext]->app.job.step_id, current_ear_app[ccontext]->app.is_mpi, current_ear_app[ccontext]->app.job.procs, pmapp->settings->def_freq, COL_CLR);
    verbose(VJOBPMON , "*******************");
    pmapp->sig_reported = 0;

}


/* This function is called by dynamic_configuration thread when a end_job command arrives */

void powermon_end_job(ehandler_t *eh, job_id jid, job_id sid, uint is_job) {
    // Application disconnected
    powermon_app_t summary, *pmapp;
    int cc;
    /* jid,sid can finish in a different order than expected */
    verbose(VJOBPMON , "********End job %lu/%lu***********", jid, sid);
    debug("powermon_end_job %lu %lu", jid, sid);
    cc = find_context_for_job(jid, sid);
    if (cc < 0) {
        error("powermon_end_job %lu,%lu and no context created for it", jid, sid);
        return;
    }
    pmapp = current_ear_app[cc];

    /* We set ccontex to the specific one */
    verbose(VJOBPMON_BASIC, "%spowermon end job (%lu,%lu)%s",COL_BLU, pmapp->app.job.id, pmapp->app.job.step_id,COL_CLR);
    while (pthread_mutex_trylock(&app_lock));
    uint ID = create_ID(jid, sid);
    job_end_powermon_app(pmapp, eh);

    // After that function, the avg power is computed
    memcpy(&summary, pmapp, sizeof(powermon_app_t));

    // we must report the cur.rent period for that job before the reset:PENDING
    end_job_for_period(&current_sample);
    pthread_mutex_unlock(&app_lock);
    report_powermon_app(&summary);
    set_null_loop(&pmapp->last_loop);
    //save_eard_conf(&eard_dyn_conf);

    /* Restore Frequency and Governor: PENDING. We must restore GPU? We must do using the affinity mask*/
    verbose(VJOBPMON, "restoring governor %s", pmapp->governor.name);
    set_governor(&pmapp->governor);
#if 0
    if (strcmp(pmapp->governor.name, "userspace") == 0) {
        frequency_set_all_cpus(pmapp->current_freq);
    }
    /* How to do that for GPUS */
#if USE_GPUS
    if (my_node_conf->gpu_def_freq > 0){
        gpu_mgr_set_freq_all_gpus(my_node_conf->gpu_def_freq);
    }
#endif
#endif

    //reset_shared_memory();
    app_mgt_end_job(pmapp->app_info);
    pcapp_info_end_job(pmapp->pc_app_info);
    powercap_end_job();
    end_context(cc);

    if (powermon_is_idle()) { 
        powercap_run_to_idle();
    }
    verbose(VJOBPMON+1,"Cleaning job from node_mgr");
    nodemgr_clean_job(jid, sid);
    clean_job_area(ID);
    verbose(VJOBPMON+1 , "*******************");
    service_close_by_id(jid,sid);

}

/*
 * This function is called by dynamic_configuration thread: Used to notify when a new task has been created
 */


/*
 * These functions are called by dynamic_configuration thread: Used to notify when a configuracion setting is changed
 *
 */
void powermon_inc_th(uint p_id,double th)
{
    policy_conf_t *p;
    p=get_my_policy_conf(my_node_conf,p_id);
    if (p==NULL){
        warning("policy %u not supported, th setting has no effect",p_id);
        return;
    }else{
        if (((p->settings[0]+th)>0) && ((p->settings[0]+th)<=1.0)){
            p->settings[0]=p->settings[0]+th;
        }else{
            warning("Current th + new is out of range, not changed");
        }
    }
    //save_eard_conf(&eard_dyn_conf);
}

void powermon_set_th(uint p_id, double th) {
    policy_conf_t *p;
    p = get_my_policy_conf(my_node_conf, p_id);
    if (p == NULL) {
        warning("policy %u not supported, th setting has no effect", p_id);
        return;
    }else{
        p->settings[0]=th;
    }
    //save_eard_conf(&eard_dyn_conf);
}

/* Sets the maximum frequency in the node */
void powermon_new_max_freq(ulong maxf) {
    uint ps;
    int cc;
    powermon_app_t *pmapp;

    /* Job running and not EAR-aware */
    for (cc = 0;cc <= max_context_created; cc++){
        if (current_ear_app[cc] != NULL){
            pmapp = current_ear_app[cc];
            if ((pmapp->app.job.id > 0) && (pmapp->app.is_mpi == 0)) {
                verbose(VJOBPMON, "MaxFreq: Application is not mpi, automatically changing freq from %lu to %lu",
                        current_node_freq, maxf);
                /* PENDING: Use the affinity mask */
#if 0
                frequency_set_all_cpus(maxf);
#endif
            }
        }
    }
    ps = frequency_closest_pstate(maxf);
    verbose(VJOBPMON, "Max pstate set to %u freq=%lu", ps, maxf);
    my_node_conf->max_pstate = ps;
    save_eard_conf(&eard_dyn_conf);
}

void powermon_new_def_freq(uint p_id, ulong def) {
    uint ps;
    uint cpolicy;
    int cc;
    powermon_app_t *pmapp;
    verbose(VJOBPMON,"New default freq for policy %u set to %lu",p_id,def);
    ps = frequency_closest_pstate(def);
    /* Job running and not EAR-aware */
    for (cc = 0;cc <= max_context_created; cc++){
        if (current_ear_app[cc] != NULL){
            pmapp = current_ear_app[cc];
            if (pmapp->app.is_mpi == 0 && my_cluster_conf.eard.force_frequencies) {
                cpolicy = policy_name_to_nodeid(pmapp->app.job.policy, my_node_conf);
                if (cpolicy == p_id) { /* If the process runs at selected policy */
                    if (def < current_node_freq) {
                        frequency_set_with_mask(&pmapp->app_info->node_mask,def);
                    }
                }
            }
        }
    }/* for */
    if (is_valid_policy(p_id, &my_cluster_conf)) {
        verbose(VJOBPMON, "New default pstate %u for policy %u freq=%lu", ps, my_node_conf->policies[p_id].policy, def);
        my_node_conf->policies[p_id].p_state = ps;
    } else {
        error("Invalid policy %u", p_id);
    }
    save_eard_conf(&eard_dyn_conf);
}


/** Sets temporally the default and max frequency to the same value. When application is not mpi, automatically chages the node freq if needed */

void powermon_set_freq(ulong freq) {
    int nump;
    uint ps,cc;
    powermon_app_t *pmapp;

    ps = frequency_closest_pstate(freq);
    for (cc = 0;cc <= max_context_created; cc++){
        if (current_ear_app[cc] != NULL){
            pmapp = current_ear_app[cc];
            if ((my_cluster_conf.eard.force_frequencies) && (pmapp->app.is_mpi == 0)) {
                frequency_set_with_mask(&pmapp->app_info->node_mask,freq);
            }
        }
    }

    my_node_conf->max_pstate = ps;
    for (nump = 0; nump < my_node_conf->num_policies; nump++) {
        verbose(VJOBPMON + 1, "New default pstate %u for policy %u freq=%lu", ps, my_node_conf->policies[nump].policy,
                freq);
        my_node_conf->policies[nump].p_state = ps;
    }
    save_eard_conf(&eard_dyn_conf);
}

/** it reloads ear.conf */
void powermon_reload_conf() {
}
void update_pmapps(power_data_t *last_pmon, nm_data_t *nm)
{
    powermon_app_t *pmapp;
    ulong lcpus, tcpus = 0;
    float cpu_ratio;
    ulong num_jobs;
    ulong time_consumed;
    double my_dc_power;
    job_context_t *alloc;
    int pos_job_in_node;

    time_consumed = (ulong) difftime(last_pmon->end, last_pmon->begin);

    /* we must detect which ones are using the node, filtering ssh etc */

    num_jobs = num_jobs_in_node();
    if (num_jobs == 0) return;

    /* Set to 0 the num_cpus per job, we will compute again */
    clean_cpus_in_jobs();


    for (uint cc = 1;cc <= max_context_created; cc++){
        if (current_ear_app[cc] != NULL){
            pmapp = current_ear_app[cc];
            /* We must update the current signature */
            pmapp->exclusive = (num_jobs == 1) && pmapp->exclusive;
            pos_job_in_node = is_job_in_node(pmapp->app.job.id, &alloc);
            /* is_mpi means is an step + EARL */
            if (pmapp->app.is_mpi) pmapp->earl_num_cpus  = num_cpus_in_mask(&pmapp->app_info->node_mask);
            /* Else, if !is_job means step without EARL */
            else  if (!pmapp->is_job)             pmapp->earl_num_cpus  = ear_min(num_cpus_in_mask(&pmapp->plug_mask), pmapp->plug_num_cpus);
            /* is job and 1 context means srun without sbatch */
            else  if (pmapp->is_job && pos_job_in_node && alloc->numc == 1) pmapp->earl_num_cpus  = ear_min(num_cpus_in_mask(&pmapp->plug_mask), pmapp->plug_num_cpus);


            /* earl_num_cpus is > 0 when the app uses EARL */	
            if (pmapp->earl_num_cpus){ 
                if (is_job_in_node(pmapp->app.job.id, &alloc)){
                    verbose(VEARD_NMGR, "Job %lu found, adding %u cpus ", pmapp->app.job.id, pmapp->earl_num_cpus);
                    tcpus += pmapp->earl_num_cpus;
                    alloc->num_cpus += pmapp->earl_num_cpus;
                }
            }
        }
    }


    verbose_jobs_in_node(VCONF);

    for (uint cc = 1;cc <= max_context_created; cc++){
        verbose(VEARD_NMGR, "Testing context %d", cc);
        if (current_ear_app[cc] != NULL){
            pmapp = current_ear_app[cc];
            /* We must update the current signature */
            lcpus = pmapp->earl_num_cpus;
            if (lcpus){
                cpu_ratio = (num_jobs > 1) ? (float)lcpus/(float)tcpus : 1;
                verbose(VEARD_NMGR,"Job %lu/%lu has %lu from %lu. cpu_ratio %f", pmapp->app.job.id ,pmapp->app.job.step_id , lcpus, tcpus, cpu_ratio);
            }else{
                if (is_job_in_node(pmapp->app.job.id, &alloc)){
                    cpu_ratio = (float)alloc->num_cpus/(float)tcpus;
                    verbose(VEARD_NMGR,"Job %lu/%lu without node mask, using ratio %f = %u/%lu", pmapp->app.job.id ,pmapp->app.job.step_id, cpu_ratio, alloc->num_cpus, tcpus); 
                }else{
                    verbose(VEARD_NMGR,"Warning, no cpus detected using num_jobs %lu", num_jobs);
                    cpu_ratio = 1.0/(float)num_jobs;

                }
            }
            my_dc_power = accum_node_power(last_pmon) * cpu_ratio;
            pmapp->accum_ps.DC_energy 	+= accum_node_power(last_pmon) * cpu_ratio * time_consumed;
            pmapp->accum_ps.DRAM_energy += accum_dram_power(last_pmon) * cpu_ratio * time_consumed;
            pmapp->accum_ps.PCK_energy 	+= accum_cpu_power(last_pmon)  * cpu_ratio * time_consumed;
            if (pmapp->app.is_mpi) pmapp->accum_ps.avg_f += get_nm_cpufreq_with_mask( &my_nm_id, nm, pmapp->app_info->node_mask) * time_consumed;
            else                   pmapp->accum_ps.avg_f += get_nm_cpufreq_with_mask( &my_nm_id, nm, pmapp->plug_mask) * time_consumed;
            pmapp->accum_ps.max = ear_max(pmapp->accum_ps.max, my_dc_power);
            if (pmapp->accum_ps.min > 0 ) pmapp->accum_ps.min = ear_min(pmapp->accum_ps.min, my_dc_power);
            else 													pmapp->accum_ps.min = my_dc_power;			
#if USE_GPUS
            float gpu_ratio = 1;
            pmapp->accum_ps.GPU_energy  = accum_gpu_power(last_pmon)  * gpu_ratio * time_consumed;
#endif

        }
    }
    verbose(VEARD_NMGR,"Power update pmapp done");
}


// Each sample is processed by this function
//
// MAIN PERIODIC_METRIC FUNCTION
//
//
//
//
//

#define VERBOSE_POWERMON 1
void update_historic_info(power_data_t *last_pmon, nm_data_t *nm, power_data_t *pmon_info, uint report) {
    ulong jid, mpi,sid;
    ulong time_consumed;
    double maxpower, minpower, rapl_dram, rapl_pck, corrected_power,pckp,dramp, gpup, gpu_power, node_dom_power;
    powermon_app_t *pmapp;
    uint current_jobs_running_in_node = 0;

    /* last_pmon is the last short power monitor period info. pmon_info can be larger. Use last_pmon  for fine grain power metrics*/

    pmapp 		= current_ear_app[max_context_created];
    jid       = pmapp->app.job.id;
    sid       = pmapp->app.job.step_id;
    mpi       = pmapp->app.is_mpi;
    maxpower  = pmapp->app.power_sig.max_DC_power;
    minpower  = pmapp->app.power_sig.min_DC_power;

    update_pmapps(last_pmon, nm);

    /* If there is no powercap, last_pmon and pmon_info is the same info. Otherwise, last_pmon is a short period and pmon_info corresponds with the period set in the ear.conf */
    /* Powercap */
    current_sample.avg_f = (ulong) get_nm_cpufreq(&my_nm_id, nm);
    pdomain.platform = last_pmon->avg_dc;
    pdomain.cpu      = accum_cpu_power(last_pmon);
    pdomain.dram     = accum_dram_power(last_pmon);
    pdomain.gpu      = accum_gpu_power(last_pmon);

    dramp = pdomain.dram;
    pckp  = pdomain.cpu;
    gpup  = pdomain.gpu;
    if (powercap_elapsed_last_powercap() >= f_monitoring) powercap_set_power_per_domain(&pdomain,mpi,current_sample.avg_f);



    if (nodemgr_get_num_jobs_attached(&current_jobs_running_in_node) == EAR_ERROR){
        verbose(VEARD_NMGR,"Error asking for the number of jobs in node");
    }


#if VERBOSE_POWERMON
    print_app_mgt_data(pmapp->app_info);
    verbose(VNODEPMON,"%s",COL_BLU);
#if USE_GPUS
    verbose(VNODEPMON_BASIC, "JOBS %u ID %lu EARL=%lu  Power [Node=%.1lf PCK=%.1lf DRAM=%.1lf GPU=%.1lf] max %.1lf min %.1lf ",
            current_jobs_running_in_node, jid, mpi, last_pmon->avg_dc,pckp,dramp,gpup, maxpower, minpower);
#else
    verbose(VNODEPMON_BASIC, "JOBS %u ID %lu EARL=%lu  Power [Node=%.1lf PCK=%.1lf DRAM=%.1lf] max %.1lf min %.1lf ",
            current_jobs_running_in_node, jid, mpi, last_pmon->avg_dc, pckp,dramp,maxpower, minpower);
#endif
    verbose_node_metrics(&my_nm_id, nm);
    verbose(VNODEPMON,"%s",COL_CLR);

    /* We use last context as reference but we print all of them */
    if (verb_level >= VEARD_NMGR) {
        for (uint cc = 0;cc < MAX_CPUS_SUPPORTED; cc ++){ 
            pmapp = current_ear_app[cc];
            if ((pmapp != NULL) && !(is_null(&pmapp->last_loop)==1)){
                signature_print_simple_fd(verb_channel,&pmapp->last_loop.signature);
                verbose(VEARD_NMGR," ");
            }
        }
    }
    report_periodic_power(fd_periodic, last_pmon);
#endif

    pmapp     = current_ear_app[max_context_created];

    while (pthread_mutex_trylock(&app_lock));

    /* We compute max and mins */
    if (pmapp->app.job.id > 0) {
        if ((last_pmon->avg_dc > maxpower) && (last_pmon->avg_dc < my_node_conf->max_error_power)){
            pmapp->app.power_sig.max_DC_power = last_pmon->avg_dc;
        }
        if ((last_pmon->avg_dc < minpower) && (minpower>= my_node_conf->min_sig_power)){
            pmapp->app.power_sig.min_DC_power = last_pmon->avg_dc;
        }
    }

    pthread_mutex_unlock(&app_lock);

    /* Pending fields for periodic_metric. we compute anyway because we use for error checking */
    current_sample.start_time = pmon_info->begin;
    current_sample.end_time   = pmon_info->end;
    corrected_power           = pmon_info->avg_dc;

    /* we use per domain power as a minimum */
    rapl_dram                 = accum_dram_power(pmon_info);
    rapl_pck                  = accum_cpu_power(pmon_info); 
    gpu_power                 = accum_gpu_power(pmon_info);
    node_dom_power = rapl_dram + rapl_pck + gpu_power;
    if (pmon_info->avg_dc < node_dom_power ) {
        corrected_power = node_dom_power;
    }
    if ((corrected_power > my_node_conf->max_error_power) || (corrected_power < 0)){
        corrected_power = my_node_conf->max_sig_power;
    }
    time_consumed = (ulong) difftime(pmon_info->end, pmon_info->begin);
    current_sample.DRAM_energy = rapl_dram * time_consumed;
    current_sample.PCK_energy  = rapl_pck  * time_consumed;
#if USE_GPUS
    current_sample.GPU_energy = gpu_power * time_consumed;
#endif
    current_sample.DC_energy = corrected_power * time_consumed;
    current_sample.temp = (ulong) get_nm_temp(&my_nm_id, nm);

    /* To be usd by status */
    copy_node_metrics(&my_nm_id, &last_nm, nm);



    /* Error control */
#if SYSLOG_MSG
    if ((last_pmon->avg_dc == 0) 
            || (last_pmon->avg_dc < my_node_conf->min_sig_power) 
            || (last_pmon->avg_dc > my_node_conf->max_sig_power) 
            || (last_pmon->avg_dc > my_node_conf->max_error_power))
    {
        syslog(LOG_DAEMON|LOG_ERR,"Node %s reports %.2lf as avg power in last period\n",nodename,last_pmon->avg_dc);
        log_report_eard_rt_error(&rid,jid,sid,DC_POWER_ERROR,last_pmon->avg_dc);
    }
    if (current_sample.temp > my_node_conf->max_temp){
        syslog(LOG_DAEMON|LOG_ERR,"Node %s reports %lu degress (max %lu)\n",nodename,current_sample.temp,my_node_conf->max_temp);
        log_report_eard_rt_error(&rid,jid,sid,TEMP_ERROR,current_sample.temp);
    }
    if ((rapl_dram + rapl_pck) == 0) {
        log_report_eard_rt_error(&rid,jid,sid,RAPL_ERROR,0);
    }
    if ((current_sample.avg_f == 0) || ((current_sample.avg_f > frequency_get_nominal_freq()) && (mpi))){
        log_report_eard_rt_error(&rid,jid,sid,FREQ_ERROR,current_sample.avg_f);
    }
#endif

    /* We check if we have to reset the IPMI interface */
    if ((last_pmon->avg_dc == 0) || (last_pmon->avg_dc > my_node_conf->max_error_power)) {
        warning("Resetting IPMI interface since power is %.2lf  (limit %lf)", last_pmon->avg_dc, my_node_conf->max_error_power);

        if (energy_dispose(&my_eh_pm) != EAR_SUCCESS) {
            error("when resetting IPMI interface 'energy_dispose'");
        }

        if (energy_init(NULL, &my_eh_pm) != EAR_SUCCESS) {
            error("when resetting IPMI interface in 'energy_init'");
        }
    }

    /* report periodic metric */
    if (report){	
        report_connection_status = report_periodic_metrics(&rid, &current_sample, 1);
        last_power_reported = corrected_power;
    }
    last_node_power     = last_pmon->avg_dc;

    return;
}

void create_powermon_out() {
    char output_name[MAX_PATH_SIZE * 4];
    char *header = "job_id;begin_time;end_time;mpi_init_time;mpi_finalize_time;avg_dc_power;max_dc_power;min_dc_power\n";
    mode_t my_mask;
    // We are using EAR_TMP but this info will go to the DB
    my_mask = umask(0);
    sprintf(output_name, "%s/%s.pm_data.csv", ear_tmp, nodename);
    fd_powermon = open(output_name, O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd_powermon < 0) {
        fd_powermon = open(output_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd_powermon >= 0) write(fd_powermon, header, strlen(header));
    }
    if (fd_powermon < 0) {
        error("Error creating output file %s", strerror(errno));
    } else {
        verbose(VNODEPMON + 1, " Created node power monitoring  file %s", output_name);
    }
    sprintf(output_name, "%s/%s.pm_periodic_data.txt", ear_tmp, nodename);
    fd_periodic = open(output_name, O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd_periodic < 0) {
        fd_periodic = open(output_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    }
    if (fd_periodic < 0) {
        error("Error creating output file for periodic monitoring %s", strerror(errno));
    } else {
        verbose(VNODEPMON + 1, " Created power monitoring file for periodic information %s", output_name);
    }
    umask(my_mask);
}

void check_rt_error_for_signature(application_t *app)
{
    job_id jid,sid;
    jid       = app->job.id;
    sid       = app->job.step_id;
    if (app->signature.GBS == 0)
    {
        log_report_eard_rt_error(&rid,jid,sid,GBS_ERROR,0);
    }
    if (app->signature.CPI == 0){
        log_report_eard_rt_error(&rid,jid,sid,CPI_ERROR,0);
    }
    if (app->signature.DC_power == 0){
        log_report_eard_rt_error(&rid,jid,sid,DC_POWER_ERROR,0);
    }
    if (app->signature.avg_f == 0){
        log_report_eard_rt_error(&rid,jid,sid,FREQ_ERROR,0);
    }
}

void powermon_mpi_signature(application_t *app) {
    powermon_app_t *pmapp;
    int cc;

    if (app == NULL) {
        error("powermon_mpi_signature: and NULL app provided");
        return;
    }
    cc = find_context_for_job(app->job.id, app->job.step_id);
    if (cc < 0){
        error("powermon_mpi_signature and no context created for (%lu,%lu)",app->job.id, app->job.step_id);
        return;
    }
    pmapp = current_ear_app[cc];

    check_rt_error_for_signature(app);

    signature_copy(&pmapp->app.signature, &app->signature);
    pmapp->app.job.def_f = app->job.def_f;
    pmapp->app.job.th = app->job.th;
    pmapp->app.job.procs = app->job.procs;
    strcpy(pmapp->app.job.policy, app->job.policy);
    strcpy(pmapp->app.job.app_id, app->job.app_id);
    pmapp->sig_reported = 1;
    verbose(VJOBPMON,"MPI signature reported for context %d,freq = %lu",cc,pmapp->app.signature.avg_f);

    save_eard_conf(&eard_dyn_conf);
}

void powermon_init_nm()
{
    if (init_node_metrics(&my_nm_id, &node_desc, frequency_get_nominal_freq()) != EAR_SUCCESS) {
        error("Initializing node metrics");
    }
    if (init_node_metrics_data(&my_nm_id, &nm_init) != EAR_SUCCESS) {
        error("init_node_metrics_data init");
    }
    if (init_node_metrics_data(&my_nm_id, &nm_end) != EAR_SUCCESS) {
        error("init_node_metrics_data end");
    }
    if (init_node_metrics_data(&my_nm_id, &nm_diff) != EAR_SUCCESS) {
        error("init_node_metrics_data diff");
    }
    if (init_node_metrics_data(&my_nm_id, &last_nm) != EAR_SUCCESS) {
        error("init_node_metrics_data diff");
    }
}

void powermon_new_jobmask(cpu_set_t *newjobmask)
{
    aggregate_cpus_to_mask(&in_jobs_mask, newjobmask);
}

void powermon_end_jobmask(cpu_set_t *endjobmask)
{
    remove_cpus_from_mask(&in_jobs_mask, endjobmask);
}


/*
 *	MAIN POWER MONITORING THREAD
 */


// frequency_monitoring will be expressed in usecs
void *eard_power_monitoring(void *noinfo)
{
    energy_data_t e_begin;
    energy_data_t e_end;
    power_data_t my_current_power;

    /* Powercap */
    uint report_data = 1;
    uint report_periods = 1;
    uint current_periods = 0;

    CPU_ZERO(&in_jobs_mask);

    PM_set_sigusr1();
    form_database_paths();

    verbose(VJOBPMON, "Power monitoring thread UP");
    init_contexts();
    powermon_create_idle_context();

    if (pthread_setname_np(pthread_self(), TH_NAME)) error("Setting name for %s thread %s", TH_NAME, strerror(errno));
    if (init_power_ponitoring(&my_eh_pm) != EAR_SUCCESS) {
        error("Error in init_power_ponitoring");
    }
    debug("Allocating memory for energy data");	
    alloc_energy_data(&c_energy);
    alloc_energy_data(&e_begin);
    alloc_energy_data(&e_end);
    alloc_power_data(&my_current_power);

    /* Powercap */
    alloc_energy_data(&e_pmon_begin);
    alloc_energy_data(&e_pmon_end);
    alloc_energy_data(&e_pmon);
    alloc_power_data(&p_pmon);

    // current_sample is the current powermonitoring period
    debug("init_periodic_metric");
    init_periodic_metric(&current_sample);
    powercap_init();
    set_powercapstatus_mode(AUTO_CONFIG);
    create_powermon_out();

    /* Powercap */
    if (powermon_get_powercap_def() > 0){	
        report_periods = ear_max(1, f_monitoring/10);
        f_monitoring = 10; 
    }

    // We will collect and report avg power until eard finishes
    // Get time and Energy

    debug("read_enegy_data");
    read_enegy_data(&my_eh_pm, &e_begin);

    /* Powercap */
    copy_energy_data(&e_pmon_begin, &e_begin);

    /* Update with the curent node conf */
    debug("Initializing node metrics");
    powermon_init_nm();
    debug("Starting node metrics");
    if (start_compute_node_metrics(&my_nm_id, &nm_init) != EAR_SUCCESS) {
        error("start_compute_node_metrics");
    }
#if SYSLOG_MSG
    openlog("eard",LOG_PID|LOG_PERROR,LOG_DAEMON);
#endif

    powermon_freq_list   = frequency_get_freq_rank_list();
    powermon_num_pstates = frequency_get_num_pstates();

#if USE_GPUS
    if (gpu_mgr_init() != EAR_SUCCESS){
        error("Error initializing GPU management");
    }
#endif

    /*
     *	MAIN LOOP
     */
    debug("Starting power monitoring loop, reporting metrics every %d seconds",f_monitoring);

    while (!eard_must_exit) {
        verbose(VNODEPMON,"\n%s------------------- NEW PERIOD -------------------%s",COL_BLU,COL_CLR);
        // Wait for N secs
        sleep(f_monitoring);

        // Get time and Energy
        //debug("Reading energy");
        read_enegy_data(&my_eh_pm, &e_end);

        if (e_end.DC_node_energy == 0) {
            warning("Power monitor period invalid energy reading, continue");
        } else {

            // Get node metrics
            //debug("Reading node metrics");
            end_compute_node_metrics(&my_nm_id, &nm_end);
            diff_node_metrics(&my_nm_id, &nm_init, &nm_end, &nm_diff);
            start_compute_node_metrics(&my_nm_id, &nm_init);


            // Compute the power
            compute_power(&e_begin, &e_end, &my_current_power);

            // print_power(&my_current_power,1,-1);

            /* Powercap */
            current_periods++;
            report_data = (current_periods == report_periods);
            if (report_data){
                current_periods = 0;
                copy_energy_data(&e_pmon_end, &e_end);
                compute_power(&e_pmon_begin, &e_pmon_end, &p_pmon);	
                copy_energy_data(&e_pmon_begin, &e_pmon_end);
            }

            // Save current power
            if (!report_data) copy_power_data(&p_pmon, &my_current_power);
            update_historic_info(&my_current_power, &nm_diff, &p_pmon, report_data);

            // Set values for next iteration
            copy_energy_data(&e_begin, &e_end);
        }

    }
    debug("Power monitor thread EXITs");
    if (dispose_node_metrics(&my_nm_id) != EAR_SUCCESS) {
        error("dispose_node_metrics ");
    }
    powercap_end();
    pthread_exit(0);
}

void powermon_get_status(status_t *my_status) {
    int i;
    int max_p; // More than TOTAL_POLICIES is not yet supported
    /* Policies */
    if (my_node_conf->num_policies > TOTAL_POLICIES) max_p = TOTAL_POLICIES;
    else max_p = my_node_conf->num_policies;
    my_status->num_policies = max_p;
    for (i=0;i<max_p;i++){
        my_status->policy_conf[i].freq = frequency_pstate_to_freq(my_node_conf->policies[i].p_state);
        my_status->policy_conf[i].th   = (uint)(my_node_conf->policies[i].settings[0]*100.0);
        my_status->policy_conf[i].id   = (uint)my_node_conf->policies[i].policy;
    }
    /* Current app info */
    my_status->app.job_id = current_ear_app[max_context_created]->app.job.id;
    my_status->app.step_id = current_ear_app[max_context_created]->app.job.step_id;
    /* Node info */
    my_status->node.avg_freq = (ulong)(last_nm.avg_cpu_freq);
    my_status->node.temp = (ulong) get_nm_temp(&my_nm_id, &last_nm);
    my_status->node.power     = (ulong) last_node_power;
    my_status->node.rep_power = (ulong) last_power_reported;
    my_status->node.max_freq = (ulong) frequency_pstate_to_freq(my_node_conf->max_pstate);
    my_status->eardbd_connected = (report_connection_status == EAR_SUCCESS);
}

void powermon_get_power(uint64_t *power) 
{
    *power = (uint64_t) last_node_power;
}

/* Returns the number of applications all or only master */
int powermon_get_num_applications(int only_master)
{
    int i;
    int total = 0,total_master = 0;
    for (i=0;i <= max_context_created;i ++){
        if (current_ear_app[i] != NULL){
            total ++;
            if (is_app_master(current_ear_app[i]->app_info)) total_master++;
            verbose(VJOBPMON,"App %d is master=%d",i,is_app_master(current_ear_app[i]->app_info));
        }
    }
    total         = ear_max(1, total - 1);
    total_master  = ear_max(1, total_master -1);
    verbose(VJOBPMON,"We have %d apps created and %d masters",total, total_master);
    if (only_master) return total_master;
    return total;
}

#define DEBUG_APP_STATUS 1
void powermon_get_app_status(app_status_t *my_status,  int num_apps, int only_master)
{
    time_t consumed_time;
    int i,cs = 0;
    verbose(DEBUG_APP_STATUS, "powermon_get_app_status num_apps %d only_master %d max_context_created %d", num_apps, only_master, max_context_created);
    /*if (max_context_created == 0 || (only_master && (num_apps == 1))){*/
    if (max_context_created == 0 || (only_master && (num_apps == 0))){
        verbose(DEBUG_APP_STATUS, "No applications found");
        my_status[cs].job_id = 0;
        signature_init(&my_status[cs].signature);
        my_status[cs].signature.DC_power = (ulong) last_power_reported;
        my_status[cs].signature.avg_f = (ulong)(last_nm.avg_cpu_freq);
        return;
    }
    for (i=1;(i<= max_context_created) && (cs<num_apps);i ++){
        verbose(DEBUG_APP_STATUS, "Looking for app %d", i);
        if (current_ear_app[i] != NULL){
            if (!only_master || (only_master && is_app_master(current_ear_app[i]->app_info))){
                verbose(DEBUG_APP_STATUS, "Adding app %d to the app status %lu/%lu MR %d", i,current_ear_app[i]->app.job.id,current_ear_app[i]->app.job.step_id,current_ear_app[i]->app_info->master_rank);
                my_status[cs].job_id = current_ear_app[i]->app.job.id;
                my_status[cs].step_id = current_ear_app[i]->app.job.step_id;
                if (!(is_null(&current_ear_app[i]->last_loop) == 1)){
                    signature_copy(&my_status[cs].signature,&current_ear_app[i]->last_loop.signature);
                    verbose(DEBUG_APP_STATUS, "App with signature");
                }else{
                    verbose(DEBUG_APP_STATUS, "App without signature");
                    signature_init(&my_status[cs].signature);
                    my_status[cs].signature.DC_power = (ulong) last_power_reported;
                    my_status[cs].signature.avg_f = (ulong)(last_nm.avg_cpu_freq);
                }
                time(&consumed_time);
                my_status[cs].signature.time = difftime(consumed_time,current_ear_app[i]->app.job.start_time);
                my_status[cs].nodes = current_ear_app[i]->app_info->nodes;
                my_status[cs].master_rank = current_ear_app[i]->app_info->master_rank;
                cs++;
            }
        }
    }
    debug("app_status end ");

}

void print_powermon_app(powermon_app_t *app) 
{
    print_application(&app->app);
    print_energy_data(&app->energy_init);
}

void powermon_report_event(uint event_type, ulong value) 
{
    int jid, sid;
    int cc = max_context_created;
    jid = current_ear_app[cc]->app.job.id;
    sid = current_ear_app[cc]->app.job.step_id; 
    debug("powermon_report_event: sending event %u with value %lu", event_type, value);
    log_report_eard_powercap_event(&rid,jid,sid,event_type, value);
}

uint powermon_is_idle()
{
    return (max_context_created == 0); //if only the idle context exists

}
uint powermon_current_power()
{
    return (uint)last_power_reported;
}
uint powermon_get_powercap_def()
{
    return (uint) my_node_conf->powercap;
}

uint powermon_get_max_powercap_def()
{
    return (uint) my_node_conf->max_powercap;
}





