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
#include <sys/wait.h>
#include <sys/syscall.h>
#include <math.h>

#include <common/config.h>
#include <common/config/config_sched.h>
#include <common/messaging/msg_conf.h>
#include <common/system/execute.h>
#include <common/system/sockets.h>
#include <common/system/folder.h>
#include <common/system/lock.h>

#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/application.h>
#include <common/types/periodic_metric.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/utils/sched_support.h>
#include <metrics/cpufreq/cpufreq.h>
#include <management/imcfreq/imcfreq.h>
#include <common/hardware/hardware_info.h>
#include <management/management.h>
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
#include <daemon/shared_pmon.h>

#if SYSLOG_MSG
#include <syslog.h>
#endif

#include <report/report.h>


#define POWERMON_RESTORE 1 // Enables the restore settings mechanism.

extern pthread_barrier_t setup_barrier;

/* Defined in eard.c */
extern uint report_eard ;
extern report_id_t rid;
extern state_t report_connection_status;

/** These variables come from eard.c */
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

extern state_t report_connection_status;

/* from eard_node_services */
extern manages_info_t man;
extern metrics_info_t met;

state_t s;
nm_t my_nm_id;
nm_data_t nm_init, nm_end, nm_diff, last_nm;

dom_power_t pdomain;
cpu_set_t in_jobs_mask;

static pthread_mutex_t app_lock = PTHREAD_MUTEX_INITIALIZER;
static ehandler_t my_eh_pm;
static char *TH_NAME = "PowerMon";

/* Used to compute avg freq for jobs. Must be moved to powermon_app */
static cpufreq_t *freq_job2;
static uint       freq_count;

// This array has 1 entry per job supported
static job_context_t jobs_in_node_list[MAX_CPUS_SUPPORTED];

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

static uint *powermon_restore_cpufreq_list;


/************** RECOVERY HW SETTINGS - Only work under POWERMON_RESTORE *************/

/** This function is called to restore any setting when the job \p app begins.
 * By now, this function restores the IMC frequency and the GPU frequency of the node.
 * \todo GPU restoring can be improved, as we can know the GPUs used by the job. */
static void set_default_powermon_init(powermon_app_t *app);


#if !NAIVE_SETTINGS_SAVE
/** This function is called to restore any setting when the job \p app ends.
 * \param idle Indicates whether the node will be idle after the job ends. */
static void set_default_powermon_end(powermon_app_t *app, uint idle);

/************************************************************************************/

/** This function sets the current job's governor information
 * at the memory region pointed by \p powermon_app. Concretaly,
 * it stores current governor of the CPUs in the \p mask. */
static state_t store_current_task_governor(powermon_app_t *powermon_app, cpu_set_t *mask);
#else
/** This function fills attributes governor and current_freq of \p powermon_app with the
 * current values of CPU 0. The goal is to have an errorless function to be used on cases
 * we need a simple power monitor use case. */
static state_t save_global_cpu_settings(powermon_app_t *powermon_app);


/** This function restores governor and CPU settings to all CPUs in the node. */
static state_t restore_global_cpu_settings(powermon_app_t *powermon_app);
#endif // NAIVE_SETTINGS_SAVE




/* This function is called to validate the jobs in the list that are still alive.
 * Only usable in SLURM systems. Deactivated for now. */
static void check_status_of_jobs(job_id current);


/***************** JOBS in NODE ***************************/

void init_jobs_in_node()
{
  memset(jobs_in_node_list, 0, sizeof(job_context_t) * MAX_CPUS_SUPPORTED);
}


uint is_job_in_node(job_id id, job_context_t **jc)
{
  uint i = 0;
  while ((i < MAX_CPUS_SUPPORTED) && (jobs_in_node_list[i].id != id)) i++;

  if (i < MAX_CPUS_SUPPORTED) *jc = &jobs_in_node_list[i];

  return (i < MAX_CPUS_SUPPORTED);
}


/** Adds the Job ID \p id into the jobs_in_node_list array.
 * If the Job ID already exists, increments the number of contexts.
 * Otherwise, it finds a free space in the array and add there the 
 * passed Job ID. */
static void add_job_in_node(job_id id)
{
    // Find the job
    uint i = 0;
    while (i < MAX_CPUS_SUPPORTED && jobs_in_node_list[i].id != id) i++;

    if (i < MAX_CPUS_SUPPORTED) {
        jobs_in_node_list[i].num_ctx++;
        return;
    }

    // `id` is a new job.

    // Find a free space, i.e., n-th jobs_in_node_list is 0
    i = 0;
    while (i < MAX_CPUS_SUPPORTED && jobs_in_node_list[i].id) i++;
    if (i == MAX_CPUS_SUPPORTED) {
        error("Not job position free found! This should not happen.");
        return;
    }

    jobs_in_node_list[i].id = id;
    jobs_in_node_list[i].num_ctx = 1;
}


void del_job_in_node(job_id id)
{
    /* Find the job */
    uint i = 0;
    while ((i < MAX_CPUS_SUPPORTED) && (jobs_in_node_list[i].id != id)) i++;

    if (i < MAX_CPUS_SUPPORTED) {
        jobs_in_node_list[i].num_ctx--;

        if (jobs_in_node_list[i].num_ctx == 0) {
            jobs_in_node_list[i].id = 0;
        }
    } else {
		debug("Deleting job %lu in node list: not found", id);
	}
}


void clean_cpus_in_jobs()
{
	for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++) {
		jobs_in_node_list[i].num_cpus = 0;
	}
}


uint num_jobs_in_node()
{
	uint total = 0;
	for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++) {
		if (jobs_in_node_list[i].id) total++;
	}
	return total;
}


void verbose_jobs_in_node(uint vl)
{
	for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++) {
		if (jobs_in_node_list[i].num_ctx) {
			verbose(vl, "JOB[%u] id %lu #contexts %u #CPUs %u",
                    i, jobs_in_node_list[i].id, jobs_in_node_list[i].num_ctx, jobs_in_node_list[i].num_cpus);
		}
	}
}

/****************** CONTEXT MANAGEMENT ********************/


powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];
static pthread_mutex_t powermon_app_mutex[MAX_NESTED_LEVELS];

/**** Shared data ****/

/* This vector is a list with the IDs . It's 1 single vector */
static uint eard_joblist[MAX_NESTED_LEVELS];
static int fd_joblist;
static uint *shared_eard_joblist;
static char joblist_path[MAX_PATH_SIZE];

/* This is 1 region per pmon , this string is re-used, only to create the paths */
static char jobpmon_path[MAX_PATH_SIZE];
/***** End shared data ****/

int max_context_created = 0;
int num_contexts = 0;


void init_contexts() {
  int i;
  for (i = 0; i < MAX_NESTED_LEVELS; i++) current_ear_app[i] = NULL;
  for (i = 0; i < MAX_NESTED_LEVELS; i++) pthread_mutex_init(&powermon_app_mutex[i], NULL);
    init_jobs_in_node();

   /* Creating shared region with job list */
   if (get_joblist_path(ear_tmp, joblist_path) != EAR_SUCCESS){
                verbose(0, "Error creating shared region for job list");
   }
   verbose(VJOBPMON, "Shared region for jobs created '%s'", joblist_path);
   memset(eard_joblist,0, sizeof(int)*MAX_NESTED_LEVELS);
   shared_eard_joblist = create_joblist_shared_area(joblist_path, &fd_joblist, eard_joblist, MAX_NESTED_LEVELS);
   if (shared_eard_joblist == NULL){
                verbose(0, "Error creating shared region for job list in path '%s', NULL address returned", joblist_path);
   }

   verbose(VJOBPMON, "Shared region with joblist initialized");

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


#if SHOW_DEBUGS
void debug_powermon_app(powermon_app_t *app)
{
	debug("jid %lu sid %lu created %u is_job %u state %d sig_reported %d exclusive %d",
			app->app.job.id, app->app.job.step_id, app->job_created, app->is_job, app->state,
			app->sig_reported, app->exclusive);
}


void print_contexts_status() {
	int i;
	for (i = 0; i <= max_context_created; i++) {
		if ((current_ear_app[i] != NULL)) {
			debug("app in position %d", i);
			debug_powermon_app(current_ear_app[i]);
		}
	}
}
#endif


int mark_contexts_to_finish_by_pid()
{
    int i = 0, found_contexts = 0;
    char pid_file[SHORT_NAME];
    struct stat tmp;

    debug("marking_contexts_to_finish by pid");

    for (i = 0; i <= max_context_created; i++) {

        if (current_ear_app[i] != NULL) {

            if (current_ear_app[i]->master_pid == 0) continue;

            sprintf(pid_file, "/proc/%d", current_ear_app[i]->master_pid);

            if (stat(pid_file, &tmp)) {

                state_t lock_st;
                if ((lock_st = ear_trylock(&powermon_app_mutex[i])) != EAR_SUCCESS) {
                    error("Locking context %d to mark it as FINISHED: %s", i, state_msg);
                    return found_contexts;
                }

                current_ear_app[i]->state = APP_FINISHED;

                ear_unlock(&powermon_app_mutex[i]);

                found_contexts++;
            }
        }
    }

    return found_contexts;
}


int mark_contexts_to_finish_by_jobid(job_id id, job_id step_id) {

    // only batch steps and interactive need to check this
    if (step_id != BATCH_STEP && step_id != INTERACT_STEP) return 0;

    int i = 0, found_contexts = 0;

    debug("marking_contexts_to_finish %lu %lu", id, step_id);

    for (i = 0; i <= max_context_created; i++) {

        if (current_ear_app[i] != NULL)
        {
            state_t lock_st;
            if ((lock_st = ear_trylock(&powermon_app_mutex[i])) != EAR_SUCCESS) {
                error("Locking context %d to mark it as FINISHED: %s", i, state_msg);
                return found_contexts;
            }

            if (current_ear_app[i]->app.job.id == id)
            {
                current_ear_app[i]->state = APP_FINISHED;

                found_contexts++;
            }

            ear_unlock(&powermon_app_mutex[i]);
        }
    }

    return found_contexts;
}


void finish_pending_contexts(ehandler_t *eh)
{
    for (int i = 0; i <= max_context_created; i++) {
        if ((current_ear_app[i] != NULL) && (current_ear_app[i]->state == APP_FINISHED))
        {
            verbose(VJOBPMON, "Finishing the %lu/%lu context...",
                    current_ear_app[i]->app.job.id, current_ear_app[i]->app.job.step_id);

            powermon_end_job(eh, current_ear_app[i]->app.job.id, current_ear_app[i]->app.job.step_id, 0);
        } 
    }
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


/* This function is called at job or step end. TODO: Thread-save here. */
void end_context(int cc, uint get_lock)
{
    int ID, i;
    int new_max;

    debug("end_context %d", cc);

    if (current_ear_app[cc] != NULL) {

        state_t lock_st;
        if (get_lock && ((lock_st = ear_trylock(&powermon_app_mutex[cc])) != EAR_SUCCESS)) {
            error("Locking context %d at ending: %s", cc, state_msg);
            return;
        }

        // We check again if the current context is NULL because we free memory
        // in a non-thread-save zone.
        if (current_ear_app[cc] != NULL) {

            free_energy_data(&current_ear_app[cc]->energy_init);

            ID = create_ID(current_ear_app[cc]->app.job.id,
                           current_ear_app[cc]->app.job.step_id);

						/* 0 means no context or Idle context */
						shared_eard_joblist[cc] = 0;
						if (get_jobmon_path(ear_tmp, ID, jobpmon_path) != EAR_SUCCESS){
							error("Creating job pmon path in end_context");
						}
						/* Releasing self shared area */
						jobmon_shared_area_dispose(jobpmon_path, NULL, current_ear_app[cc]->fd_shared_areas[SELF]);

            del_job_in_node(current_ear_app[cc]->app.job.id);

            get_settings_conf_path(ear_tmp, ID, shmem_path);

            settings_conf_shared_area_dispose(shmem_path, current_ear_app[cc]->settings, current_ear_app[cc]->fd_shared_areas[SETTINGS_AREA]);
            get_resched_path(ear_tmp, ID, shmem_path);
            resched_shared_area_dispose(shmem_path, current_ear_app[cc]->resched, current_ear_app[cc]->fd_shared_areas[RESCHED_AREA]);

            get_app_mgt_path(ear_tmp, ID, shmem_path);
            app_mgt_shared_area_dispose(shmem_path, current_ear_app[cc]->app_info, current_ear_app[cc]->fd_shared_areas[APP_MGT_AREA]);

            get_pc_app_info_path(ear_tmp, ID, shmem_path);
            pc_app_info_shared_area_dispose(shmem_path, current_ear_app[cc]->pc_app_info, current_ear_app[cc]->fd_shared_areas[PC_APP_AREA]);

            cpufreq_data_free(&current_ear_app[cc]->freq_job1,
                              &current_ear_app[cc]->freq_diff);

            free(current_ear_app[cc]->prio_idx_list);
            current_ear_app[cc]->prio_idx_list = NULL; // This is redundant since we free
                                                       // immediatelly the current context.

            num_contexts--;

            if (get_lock) ear_unlock(&powermon_app_mutex[cc]);

#if USE_PMON_SHARED_AREA
		/* Releasing self shared area */
		jobmon_shared_area_dispose(jobpmon_path, current_ear_app[cc], current_ear_app[cc]->fd_shared_areas[SELF]);
#else
	     jobmon_shared_area_dispose(jobpmon_path, (powermon_app_t *)current_ear_app[cc]->sh_self, current_ear_app[cc]->fd_shared_areas[SELF]);
             free(current_ear_app[cc]);
#endif


            current_ear_app[cc] = NULL;

            if (cc == max_context_created) {
                new_max = 0;

                for (i = 0; i <= max_context_created; i++) {
                    if ((current_ear_app[i] != NULL) && (i > new_max)) new_max = i;
                }

                debug("New max_context_created %d", new_max);
                max_context_created = new_max;
            }
        }
    }
}


/* This function will be called when job ends to be sure there is not steps pending to clean. */
void clean_job_contexts(job_id id) {
  int i, cleaned = 0;
  debug("clean_job_contexts for job %lu", id);
  for (i = 0; i <= max_context_created; i++) {
    if (current_ear_app[i] != NULL) {
      if (current_ear_app[i]->app.job.id == id) {
        cleaned++;
        end_context(i, 1);
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
        end_context(i, 1);
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
    debug("select_last_context selects context %d (%lu,%lu)",
          pos, current_ear_app[pos]->app.job.id, current_ear_app[pos]->app.job.step_id);
  } else {
    debug("select_last_context no contexts actives");
  }

  return pos;
}


/** This function iterates over the eard data and cleans not valid job metrics */
void powermon_purge_old_jobs()
{
	powermon_app_t *pmapp;
  for (uint cc = 1; cc <= max_context_created; cc++) {

  	if (current_ear_app[cc] != NULL) {
    	pmapp = current_ear_app[cc];

			state_t lock_st;
	if ((lock_st = ear_trylock(&powermon_app_mutex[cc])) != EAR_SUCCESS) {
     		error("Locking context %lu/%lu for updating its mask: %s",
                      pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
       	return;
     	}
			/* test application */
			
			ear_unlock(&powermon_app_mutex[cc]);
		}
	}
}


/* TODO: Thread-save here?: It's not possible because mutex is inside the context that will be created. */
int new_context(job_id id, job_id sid, int *cc)
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

    current_ear_app[ccontext] = (powermon_app_t *) calloc(1, sizeof(powermon_app_t));

    if (current_ear_app[ccontext] == NULL) {
        *cc = -1;
        error("Panic: malloc returns NULL for current context");
        return EAR_ERROR;
    }

    int err_num;
    if ((err_num = pthread_mutex_init(&powermon_app_mutex[ccontext],
                                      NULL)))
    {
        error("Initializing mutex for context %d (%lu/%lu): %s",
              ccontext, id, sid, strerror(err_num));
    }

    if (state_fail(ear_trylock(&powermon_app_mutex[ccontext])))
    {
        error("Locking new context");
        return EAR_ERROR;
    }

    if (ccontext > max_context_created) max_context_created = ccontext;

    alloc_energy_data(&current_ear_app[ccontext]->energy_init);

    /* This info will be overwritten later */
    current_ear_app[ccontext]->app.job.id = id;
    current_ear_app[ccontext]->app.job.step_id = sid;
    debug("New context created at pos %d", ccontext);

    add_job_in_node(id);

    /* We must create per jobid, stepid shared memory regions. */
    ID = create_ID(id, sid);
    /* Shared vector with joblist */
    shared_eard_joblist[ccontext] = ID;

		/* Start: Mapping my own data in a shared file */
		if (get_jobmon_path(ear_tmp, ID, jobpmon_path) != EAR_SUCCESS){
			error("Error creating jobpmon path");
			ear_unlock(&powermon_app_mutex[ccontext]);
			return EAR_ERROR;
		}
		// We use the new shared area rather than the allocated area
		powermon_app_t *aux1 , *aux2;
		aux1 = current_ear_app[ccontext];
		if ((aux2 = create_jobmon_shared_area(jobpmon_path, current_ear_app[ccontext], &current_ear_app[ccontext]->fd_shared_areas[SELF])) == NULL){
			error("Error creating shared memory for pmon for ((%lu,%lu)", id,sid);
			ear_unlock(&powermon_app_mutex[ccontext]);
			return EAR_ERROR;
		}
		current_ear_app[ccontext]->sh_self = (void *)aux2;
#if USE_PMON_SHARED_AREA
		// This option forces eard to use the shared pmapp region , otherwise it is not updated dynamically
		// it could be used to see the list of jobs but not to update job state etc
		current_ear_app[ccontext] = aux2;
		free(aux1);
#endif
		verbose(VJOBPMON,"Self context in path '%s' for (%lu/%lu)", jobpmon_path, id, sid);
		/* End of Pmon mapping */

    get_settings_conf_path(ear_tmp, ID, shmem_path);
    debug("Settings for new context placed at %s", shmem_path);

    current_ear_app[ccontext]->settings = create_settings_conf_shared_area(shmem_path, &current_ear_app[ccontext]->fd_shared_areas[SETTINGS_AREA]);
    if (current_ear_app[ccontext]->settings == NULL) {
        error("Error creating shared memory between EARD & EARL for (%lu,%lu)",
              id,sid);
        ear_unlock(&powermon_app_mutex[ccontext]);
        return EAR_ERROR;
    }

    /* Context 0 is the default one */
    memcpy(current_ear_app[ccontext]->settings, current_ear_app[0]->settings, sizeof(settings_conf_t));

    get_resched_path(ear_tmp, ID, shmem_path);
    debug("Resched path for new context placed at %s",shmem_path);

    current_ear_app[ccontext]->resched = create_resched_shared_area(shmem_path, &current_ear_app[ccontext]->fd_shared_areas[RESCHED_AREA]);

    if (current_ear_app[ccontext]->resched == NULL) {
        error("Error creating resched shared memory between EARD & EARL for (%lu,%lu)",id,sid);
        ear_unlock(&powermon_app_mutex[ccontext]);
        return EAR_ERROR;
    }

    current_ear_app[ccontext]->resched->force_rescheduling = 0;

    cpufreq_data_alloc(&current_ear_app[ccontext]->freq_job1, &current_ear_app[ccontext]->freq_diff);

    /* This area is for application data */

    get_app_mgt_path(ear_tmp, ID, shmem_path);

    debug("App_MGR for new context placed at %s",shmem_path);

    current_ear_app[ccontext]->app_info = create_app_mgt_shared_area(shmem_path, &current_ear_app[ccontext]->fd_shared_areas[APP_MGT_AREA]);

    if (current_ear_app[ccontext]->app_info == NULL) {
        error("Error creating shared memory between EARD & EARL for app_mgt (%lu,%lu)",id,sid);
        ear_unlock(&powermon_app_mutex[ccontext]);
        return EAR_ERROR;
    }

    /* Default value for app_info */

    get_pc_app_info_path(ear_tmp,ID, shmem_path);

    debug("PC_app_info for new context placed at %s", shmem_path);

    current_ear_app[ccontext]->pc_app_info = create_pc_app_info_shared_area(shmem_path, &current_ear_app[ccontext]->fd_shared_areas[PC_APP_AREA]);
    if (current_ear_app[ccontext]->pc_app_info == NULL){
        error("Error creating shared memory between EARD & EARL for pc_app_infoi (%lu,%lu)",id,sid);
        ear_unlock(&powermon_app_mutex[ccontext]);
        return EAR_ERROR;
    }

    current_ear_app[ccontext]->pc_app_info->cpu_mode = powercap_get_cpu_strategy();

#if USE_GPUS
    current_ear_app[ccontext]->pc_app_info->gpu_mode = powercap_get_gpu_strategy();
#endif

    /* accum_ps is the accumulated power signature when sharing nodes */
    memset(&(current_ear_app[ccontext]->accum_ps), 0, sizeof(accum_power_sig_t));

    current_ear_app[ccontext]->exclusive = (num_jobs_in_node() == 1);

    cpuprio_t *prio_list_dummy; // The priority API requires a valid pointer to cpuprio_t
                                // to call the alloc method, although we won't need it here.
    mgt_cpuprio_data_alloc(&prio_list_dummy, &current_ear_app[ccontext]->prio_idx_list);
    free(prio_list_dummy);

    /* Default value for pc_app_info
     * ???????????????????????????? */


    debug("Context created for (%lu,%lu), num_contexts %d max_context_created %d",
          id,sid,num_contexts+1,max_context_created);

    num_contexts++;

    ear_unlock(&powermon_app_mutex[ccontext]);

    return EAR_SUCCESS;
}

/****************** END CONTEXT MANAGEMENT ********************/


void create_job_area(uint ID)
{
	int ret;

	char job_path[MAX_PATH_SIZE];
	xsnprintf(job_path, sizeof(job_path), "%s/%u", ear_tmp, ID);

	verbose(VJOBPMON + 1, "Creating job path %s...", job_path);

	mode_t old_mask = umask((mode_t) 0);

	ret = mkdir(job_path, 0777);

	if (ret < 0 ) {
		verbose(VJOBPMON + 1, "Couldn't create jobpath (%s)", strerror(errno));
	}

	umask(old_mask);
}


void clean_job_area(uint ID)
{
	char job_path[MAX_PATH_SIZE];
    xsnprintf(job_path, sizeof(job_path), "%s/%u", ear_tmp, ID);
	folder_remove(job_path);
}

static state_t test_job_folder(uint ID)
{
	char job_path[MAX_PATH_SIZE];
	xsnprintf(job_path, sizeof(job_path), "%s/%u", ear_tmp, ID);
	return folder_exists(job_path);
}


powermon_app_t *get_powermon_app() {
  return current_ear_app[max_context_created];
}


static void PM_my_sigusr1(int signun) {
    verbose(VNODEPMON, " thread %u receives sigusr1", (uint) pthread_self());
    if (signun == SIGSEGV){ 
        verbose(VNODEPMON, "Powermon receives SIGSEGV. Stack......");
        print_stack(verb_channel);
    }
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
	if (sigaction(SIGSEGV, &sa, NULL) < 0)
		error(" doing sigaction of signal s=%d, %s", SIGSEGV, strerror(errno));
}


/* TODO: Thread-save here. */
void reset_shared_memory(powermon_app_t *pmapp)
{
    verbose(VJOBPMON + 2, "reset_shared_memory");

    policy_conf_t *my_policy;
    my_policy = get_my_policy_conf(my_node_conf, my_cluster_conf.default_policy);

    if (my_policy == NULL) {
        verbose(VJOBPMON + 1,
                "Error, my_policy is null in reset_shared_memory when looking for default policy %d",
                my_cluster_conf.default_policy);
    }

    /*
    state_t lock_st;
    if ((lock_st = ear_trylock(&pmapp->powermon_app_mutex)) != EAR_SUCCESS) {
        error("Locking context %lu/%lu to reset shared memory: %s",
              pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
        return;
    }
    */

    pmapp->resched->force_rescheduling = 0;
    pmapp->settings->user_type         = NORMAL;
    pmapp->settings->learning          = 0;
    pmapp->settings->lib_enabled       = 1;
    pmapp->settings->policy            = my_cluster_conf.default_policy;
    pmapp->settings->def_freq          = frequency_pstate_to_freq(my_policy->p_state);
    pmapp->settings->def_p_state       = my_policy->p_state;
    pmapp->settings->max_freq          = frequency_pstate_to_freq(my_node_conf->max_pstate);

    strncpy(pmapp->settings->policy_name, my_policy->name, 64); 
    memcpy(pmapp->settings->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);

    pmapp->settings->min_sig_power   = my_node_conf->min_sig_power;
    pmapp->settings->max_sig_power   = my_node_conf->max_sig_power;
    pmapp->settings->max_power_cap   = my_node_conf->powercap;
    pmapp->settings->report_loops    = my_cluster_conf.database.report_loops;
    pmapp->settings->max_avx512_freq = my_node_conf->max_avx512_freq;
    pmapp->settings->max_avx2_freq   = my_node_conf->max_avx2_freq;

    memcpy(&pmapp->settings->installation,&my_cluster_conf.install,sizeof(conf_install_t));

    copy_ear_lib_conf(&pmapp->settings->lib_info, &my_cluster_conf.earlib);

    // ear_unlock(&pmapp->powermon_app_mutex);

    verbose(VJOBPMON, "Shared memory regions for job set with default values.");
}


void powermon_update_node_mask(cpu_set_t *nodem)
{
    powermon_app_t *pmapp;
    int i;
    debug("Updating the mask");
    CPU_ZERO(nodem);

    /* reconfiguring running apps */
    for (i = 0; i <= max_context_created; i++){

        pmapp = current_ear_app[i];

        if (pmapp != NULL) {
            cpumask_aggregate(nodem, &pmapp->app_info->node_mask);
        }
    }
}


void powermon_new_configuration()
{
    powermon_app_t *pmapp;
    int i;
    debug("New configuration for jobs");
    /* reconfiguring running apps */
    for (i = 0; i <=  max_context_created; i++) {

        pmapp = current_ear_app[i];

        if (pmapp != NULL) {
            

            reset_shared_memory(pmapp);
            if (pmapp->app.is_mpi) {
                pmapp->resched->force_rescheduling = 1; // TODO: Thread-save here.
            }

        }
    }
}


state_t powermon_create_idle_context()
{
  job_id jid = 0, sid = 0;
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

	/* Shared vector with joblist */
	shared_eard_joblist[0] = ID;
	/* Start: Mapping my own data in a shared file */
	if (get_jobmon_path(ear_tmp, ID, jobpmon_path) != EAR_SUCCESS){
		error("Error creating jobpmon path");
		return EAR_ERROR;
	}
	verbose(VJOBPMON,"Self context in path '%s'", jobpmon_path);
	if (create_jobmon_shared_area(jobpmon_path, current_ear_app[0], &current_ear_app[0]->fd_shared_areas[SELF]) == NULL){
		return EAR_ERROR;
	}
	/* End of Pmon mapping */


  /* Shared memory regions for default context */
  get_settings_conf_path(ear_tmp, ID, shmem_path);
  verbose(VJOBPMON,"Settings for new context placed at %s",shmem_path);
  pmapp->settings = create_settings_conf_shared_area(shmem_path, &pmapp->fd_shared_areas[SETTINGS_AREA]);
	if (pmapp->settings == NULL){
		verbose(VJOBPMON,"Error creating shared memory region for earl shared data");
	}
  get_resched_path(ear_tmp, ID,shmem_path);
  verbose(VJOBPMON,"Resched area for new context placed at %s",shmem_path);
  pmapp->resched = create_resched_shared_area(shmem_path, &pmapp->fd_shared_areas[RESCHED_AREA]);
	if (pmapp->resched == NULL){
		verbose(VJOBPMON,"Error creating shared memory region for re-scheduling data");
	}
  reset_shared_memory(pmapp);
  get_app_mgt_path(ear_tmp, ID, shmem_path);
  verbose(VJOBPMON,"App- Mgr area for new context placed at %s",shmem_path);
  pmapp->app_info = create_app_mgt_shared_area(shmem_path, &pmapp->fd_shared_areas[APP_MGT_AREA]);
	if (pmapp->app_info == NULL){
		verbose(VJOBPMON,"Error creating app mgr shared region ");
	}
  app_mgt_new_job(pmapp->app_info);

  get_pc_app_info_path(ear_tmp, ID, shmem_path);
  verbose(VJOBPMON,"App PC area for new context placed at %s",shmem_path);
  pmapp->pc_app_info = create_pc_app_info_shared_area(shmem_path, &pmapp->fd_shared_areas[PC_APP_AREA]);
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
// TODO: Thread-save here.
void job_init_powermon_app(powermon_app_t *pmapp, ehandler_t *ceh,
                           application_t *new_app, uint from_mpi) {
    state_t s;

    verbose(VJOBPMON, "job_init_powermon_app init %lu/%lu ",
            new_app->job.id, new_app->job.step_id);

    if (pmapp == NULL) {
        verbose(VJOBPMON,"Error, job_init_powermon_app with null pmapp");
        return;
    }


    debug("New context for N jobs scenario selected %lu/%lu",
          pmapp->app.job.id,pmapp->app.job.step_id);

    pmapp->job_created = !from_mpi;

    copy_application(&pmapp->app, new_app);

    strcpy(pmapp->app.node_id, nodename);

    time(&pmapp->app.job.start_time);

    pmapp->app.job.start_mpi_time = 0;
    pmapp->app.job.end_mpi_time   = 0;
    pmapp->app.job.end_time       = 0;
    pmapp->app.job.procs          = new_app->job.procs;

    // reset signature
    signature_init(&pmapp->app.signature);

    init_power_signature(&pmapp->app.power_sig);

    pmapp->app.power_sig.max_DC_power = 0;
    pmapp->app.power_sig.min_DC_power = 10000;
    pmapp->app.power_sig.def_f        = pmapp->settings->def_freq;

    // Initialize energy
    read_enegy_data(ceh, &c_energy);
    copy_energy_data(&pmapp->energy_init, &c_energy);

    // CPU Frequency
    state_assert(s, cpufreq_read(no_ctx, pmapp->freq_job1), );

    verbose(VJOBPMON, "job_init_powermon_app end %lu/%lu is_mpi %d",
            pmapp->app.job.id,pmapp->app.job.step_id, pmapp->app.is_mpi);

}


// TODO: Thread-save here.
void job_end_powermon_app(powermon_app_t *pmapp, ehandler_t *ceh)
{
    power_data_t app_power;
    state_t s;
    double elapsed;

    if (pmapp == NULL) {
        verbose(VJOBPMON,"Error, job_end_powermon_app with null pmapp");
        return;
    }

    debug("job_end_powermon_app for %lu/%lu", pmapp->app.job.id, pmapp->app.job.step_id);

    alloc_power_data(&app_power);


    time(&pmapp->app.job.end_time);

    if (pmapp->app.job.end_time == pmapp->app.job.start_time) {
        pmapp->app.job.end_time = pmapp->app.job.start_time + 1;
    }

    elapsed = difftime(pmapp->app.job.end_time, pmapp->app.job.start_time);

    read_enegy_data(ceh, &c_energy);
    compute_power(&pmapp->energy_init, &c_energy, &app_power);

    if (pmapp->exclusive) {

        verbose(VJOBPMON, "[%lu/%lu] Computing power signature. Job executed in exclusive mode",
                pmapp->app.job.id, pmapp->app.job.step_id);

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
    } else {
        if (pmapp->app.is_mpi) {
            verbose(VJOBPMON, "Computing power signature. Job executed in NOT exclusive mode. Using EARL signature");
            pmapp->app.power_sig.DC_power 	= pmapp->app.signature.DC_power;
            pmapp->app.power_sig.DRAM_power = pmapp->app.signature.DRAM_power;
            pmapp->app.power_sig.PCK_power	= pmapp->app.signature.PCK_power;
            pmapp->app.power_sig.avg_f        = pmapp->app.signature.avg_f;
        } else {
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
    verbose(VJOBPMON,"job_end_powermon_app done for %lu/%lu", pmapp->app.job.id, pmapp->app.job.step_id);
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

    report_connection_status = report_applications(&rid, &app->app,1);

}


policy_conf_t per_job_conf;

// TODO: Thread-save here.
policy_conf_t *configure_context(powermon_app_t *pmapp, uint user_type,
                                 energy_tag_t *my_tag, application_t *appID)
{
  policy_conf_t *my_policy = NULL;

  if (appID == NULL) {
    error("configure_context: NULL application provided, setting default values");
    my_policy = &default_policy_context;
    return my_policy;
  }

  verbose(VJOBPMON, "Configuring policy for user %u, policy %s, freq %lu, th %lf, is_learning %u, is_mpi %d, force_freq %d",
      user_type, appID->job.policy, appID->job.def_f, appID->job.th, appID->is_learning, appID->is_mpi,
      my_cluster_conf.eard.force_frequencies);

  int p_id;

  switch (user_type) {
    case NORMAL:
      appID->is_learning = 0;

      p_id = policy_name_to_nodeid(appID->job.policy, my_node_conf);

      debug("Policy requested %s, ID %d", appID->job.policy, p_id);

      /* Use cluster conf function */
      if (p_id != EAR_ERROR) {
        my_policy=get_my_policy_conf(my_node_conf,p_id);
        if (!my_policy->is_available) {
          debug("User type %d is not alloweb to use policy %s",user_type,appID->job.policy);
          my_policy=get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
        }
        copy_policy_conf(&per_job_conf, my_policy);
        my_policy=&per_job_conf;
      } else {

        debug("Invalid policy"); // You will see the invalid policy name from the last debug message.

        my_policy = get_my_policy_conf(my_node_conf, my_cluster_conf.default_policy);

        if (my_policy == NULL) {

          error("Invalid policy (%s) and no default policy configuration (NULL). Check ear.conf. Setting monitoring...",
              appID->job.policy);

          authorized_context.p_state = 1;

          int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);

          if (mo_pid != EAR_ERROR)
            authorized_context.policy = mo_pid;
          else
            authorized_context.policy = MONITORING_ONLY;

          authorized_context.settings[0] = 0;
        } else {
          copy_policy_conf(&authorized_context,my_policy);
        }

        my_policy = &authorized_context;
      }

      if (my_policy==NULL) {
        error("Default policy configuration returns NULL,invalid policy, check ear.conf");
        my_policy=&default_policy_context;
      }
      break;
    case AUTHORIZED:
      if (appID->is_learning) {
        int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);
        if (mo_pid != EAR_ERROR)
          authorized_context.policy = mo_pid;
        else
          authorized_context.policy=MONITORING_ONLY;
        if (appID->job.def_f) {
          if (frequency_is_valid_frequency(appID->job.def_f)) authorized_context.p_state=frequency_closest_pstate(appID->job.def_f);
          else authorized_context.p_state=1;
        } else authorized_context.p_state=1;

        authorized_context.settings[0]=0;
        my_policy=&authorized_context;
      } else {
        p_id=policy_name_to_nodeid(appID->job.policy, my_node_conf);
        if (p_id!=EAR_ERROR) {
          my_policy=get_my_policy_conf(my_node_conf,p_id);
          authorized_context.policy=p_id;
          if (appID->job.def_f) {
            verbose(VJOBPMON+1,"Setting freq to NOT default policy p_state ");
            if (frequency_is_valid_frequency(appID->job.def_f)) authorized_context.p_state=frequency_closest_pstate(appID->job.def_f);
            else authorized_context.p_state=my_policy->p_state;
          } else {
            verbose(VJOBPMON+1,"Setting freq to default policy p_state %u",my_policy->p_state);
            authorized_context.p_state=my_policy->p_state;	
          }
          if (appID->job.th>0) authorized_context.settings[0]=appID->job.th;
          else authorized_context.settings[0]=my_policy->settings[0];

          strcpy(authorized_context.name, my_policy->name);
          my_policy=&authorized_context;

        } else {
          verbose(VJOBPMON,"Authorized user is executing not defined/invalid policy using default %d",my_cluster_conf.default_policy);
          my_policy=get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
          if (my_policy==NULL) {
            error("Error Default policy configuration returns NULL,invalid policy, check ear.conf (setting MONITORING)");
            authorized_context.p_state=1;
            int mo_pid = policy_name_to_nodeid("monitoring", my_node_conf);
            if (mo_pid != EAR_ERROR)
              authorized_context.policy = mo_pid;
            else
              authorized_context.policy=MONITORING_ONLY;
            authorized_context.settings[0]=0;
          } else {
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

  if ((!appID->is_mpi) && (!my_cluster_conf.eard.force_frequencies)) {


    pmapp->policy_freq = 0;

    my_policy->p_state = frequency_closest_pstate(frequency_get_cpu_freq(0));

    verbose(VJOBPMON, "Application is not using ear and force_frequencies=off, frequencies are not changed pstate=%u", my_policy->p_state);

  } else {
    /* We have to force the frequency */
    ulong f = frequency_pstate_to_freq(my_policy->p_state);

#if !POWERMON_RESTORE
    cpu_set_t not_in_jobs_mask;
    powermon_update_node_mask(&in_jobs_mask);

    cpumask_not(&not_in_jobs_mask, &in_jobs_mask);

    uint cpus_used = cpumask_count(&in_jobs_mask);
    uint cpus_free = cpumask_count(&not_in_jobs_mask);
    verbose(VCONF, "CPUs used %u. CPUs free %u", cpus_used, cpus_free);
#endif

    /* New, to be used in new function */
    pmapp->policy_freq = f;

#if !POWERMON_RESTORE
    verbose(VJOBPMON, "Setting userspace governor pstate=%u to %u cpus (used %u)",
            my_policy->p_state, cpus_free, cpus_used);
		state_t ret_st = frequency_set_userspace_governor_all_cpus();
    check_usrspace_gov_set(ret_st, VJOBPMON);

    /* Setting the default CPU freq for the new job */
    frequency_set_with_mask(&not_in_jobs_mask, f);

    /* Moved to powermon_init mgw configuration */
    /* Setting the default IMC freq by default in all the sockets */
    verbose(VJOBPMON, "Restoring IMC frequency");
    if (state_fail(mgt_imcfreq_set_auto(no_ctx))){
        verbose(VJOBPMON, "Error when setting IMCFREQ by default");
    }
#if USE_GPUS
    if (my_node_conf->gpu_def_freq > 0)
    {
      /** Can we know which GPUS are used by this job?? */
      verbose(VJOBPMON, "Restoring GPU frequency to %lu", my_node_conf->gpu_def_freq);
      gpu_mgr_set_freq_all_gpus(my_node_conf->gpu_def_freq);
    }
#endif

#endif // !POWERMON_RESTORE
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
void powermon_loop_signature(job_id jid, job_id sid, loop_t *loops)
{
    int cc;
    powermon_app_t *pmapp;
    cc = find_context_for_job(jid, sid);
    if (cc < 0){
        verbose(VJOBPMON,"powermon_loop_signature and no current context for %lu/%lu",jid,sid);
        return;
    }
    pmapp = current_ear_app[cc];

    debug("New loop signature for %lu/%lu, iterations %lu",
          pmapp->app.job.id, pmapp->app.job.step_id, loops->total_iterations);

    copy_loop(&pmapp->last_loop, loops);
    return;
}


// That functions controls the mpi init/end of jobs. These functions are called by eard when application executes mpi_init/mpi_finalized and contacts eard
// TODO: Thread-save here.
void powermon_mpi_init(ehandler_t *eh, application_t *appID) {

    ulong jid,sid;
    jid = appID->job.id;
    sid = appID->job.step_id;
    powermon_app_t *pmapp;

    if (appID == NULL) {
        error("powermon_mpi_init: NULL appID");
        return;
    }

    verbose(VTASKMON, "powermon_mpi_init job_id %lu step_id %lu (is_mpi %u)",
            appID->job.id, appID->job.step_id, appID->is_mpi);

    int cc = find_context_for_job(jid, sid);

    if (cc < 0) {
        verbose(VJOBPMON, "powermon_mpi_init and context not found job_id %lu/%lu, creating automatically",
                appID->job.id, appID->job.step_id);

        powermon_new_job(NULL, eh, appID, 1, 0);

        cc = find_context_for_job(jid, sid);

        if (cc < 0) {
            verbose(VJOBPMON,"ERROR: Context not created!");
            return;
        }
    }

    pmapp = current_ear_app[cc];
    if (pmapp) {
        state_t lock_st;
        if ((lock_st = ear_trylock(&powermon_app_mutex[cc])) != EAR_SUCCESS) {
            error("Locking context %lu/%lu updating the number of local ids: %s",
                  pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
            return;
        }

        pmapp->local_ids++;

        debug("powermon_mpi_init for %lu/%lu Total local connections %d",
              pmapp->app.job.id, pmapp->app.job.step_id, pmapp->local_ids);

        /* The job was already connected, this is only a new local connection */
        if (pmapp->local_ids > 1) {
            ear_unlock(&powermon_app_mutex[cc]);
            return;
        }

        // MPI_init: It only changes mpi_init time, we don't need to acquire the lock
        start_mpi(&pmapp->app.job);
        pmapp->app.is_mpi = 1;

        verbose(VTASKMON + 1, "powermon_mpi_init FINALIZE job_id %lu step_id %lu (is_mpi %u)",
                appID->job.id, appID->job.step_id, appID->is_mpi);

        ear_unlock(&powermon_app_mutex[cc]);
        // save_eard_conf(&eard_dyn_conf);
    } else {
        error("powermon_mpi_init: Context is NULL");
    }
}


// TODO: Thread-save here.
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

    state_t lock_st;
    if ((lock_st = ear_trylock(&powermon_app_mutex[cc])) != EAR_SUCCESS) {
        error("Locking context %lu/%lu updating the number of local ids: %s",
              pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
        return;
    }

    pmapp->local_ids--;

    ear_unlock(&powermon_app_mutex[cc]);

    if (pmapp->local_ids > 0) return;

    /* We set ccontex to the specific one */

    verbose(VTASKMON, "powermon_mpi_finalize (%lu,%lu)", pmapp->app.job.id, pmapp->app.job.step_id);

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

  verbose(VTASKMON, "New task for %lu-%lu PID %d #CPUs %u",
          newtask->jid, newtask->sid, newtask->pid, newtask->num_cpus);

  /* Getting the context of the task. */

  int curr_job_ctx_idx = find_context_for_job(newtask->jid, newtask->sid);

  if (curr_job_ctx_idx < 0) {
    error("New task %d and context %lu/%lu not created.", newtask->pid, newtask->jid, newtask->sid);
    return;
  }

  pmapp = current_ear_app[curr_job_ctx_idx];

  /* ******************************** */
  
  state_t lock_st;
  if ((lock_st = ear_trylock(&powermon_app_mutex[curr_job_ctx_idx])) != EAR_SUCCESS) {
      error("Locking context %lu/%lu at receiving new task request: %s",
            pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
      return;
  }

  if (pmapp->master_pid == 0) pmapp->master_pid = newtask->pid;

  CPU_OR(&pmapp->plug_mask, &pmapp->plug_mask, &newtask->mask);

  // Instead of aggregating the number of CPUs
  // of the new task, we count the number of CPUs
  // of the aggregated mask. By this way we avoid
  // to get a bad CPU count in the case where
  // multiple masks share CPUs.
  pmapp->plug_num_cpus = CPU_COUNT(&pmapp->plug_mask); 
  verbose(VTASKMON, "#CPUs of %lu-%lu updated: %u", newtask->jid, newtask->sid, pmapp->plug_num_cpus);

  // old condition: pmapp->app.job.step_id != (job_id) BATCH_STEP && pmapp->policy_freq
  if (pmapp->policy_freq) // "As EARD can force the CPU frequency"
  {
    // CPU freq and governor
#if !NAIVE_SETTINGS_SAVE // If this macro is enabled, we already saved the governor

        /* Storing current governor (The previous governor of the powermon
         * app will be the current governor of the first CPU of the last task arrived)
         */
        if (state_fail(store_current_task_governor(pmapp, &newtask->mask)))
        {
            error("Storing task (PID %d) governor: %s", newtask->pid, state_msg);
        }
#endif // !NAIVE_SETTINGS_SAVE

    // We set the userspace governor just when the driver supports it. If not, we are assuming
    // the driver lets us to change the CPU frequency without that governor.
    if (mgt_cpufreq_governor_is_available(no_ctx, Governor.userspace))
    {

        verbose(VTASKMON, "Setting userspace governor...");

        // Governor 
        // governor_toint(cgov, &gov);
        if (state_fail(mgt_cpufreq_governor_set_mask(no_ctx, Governor.userspace, newtask->mask)))
        {
          error("Error setting cpufreq userspace governor");
        }
    }

    uint app_pstate;

    mgt_cpufreq_get_index(no_ctx, pmapp->policy_freq, &app_pstate, 1);

    verbose(VTASKMON, "Setting CPU freq %.2f GHz and pstate %u",
            (float) pmapp->policy_freq / 1000000, app_pstate);

    verbose_affinity_mask(VTASKMON, &newtask->mask, man.cpu.devs_count);

    /* CPU freq. */
    fill_cpufreq_list(&newtask->mask, man.cpu.devs_count, app_pstate, ps_nothing, powermon_restore_cpufreq_list);
    mgt_cpufreq_set_current_list(no_ctx, powermon_restore_cpufreq_list);

    /* *********************/
  }

  ear_unlock(&powermon_app_mutex[curr_job_ctx_idx]);
}

/*
 *
 *
 *   JOB PART
 *
 */


/* This function is called by dynamic_configuration thread when a new_job command arrives.
 * More info in the header file.
 * TODO: Thread-save here. */
void powermon_new_job(powermon_app_t *pmapp, ehandler_t *eh, application_t *appID, uint from_mpi, uint is_job) {

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

    if (!powermon_is_idle() && is_job) check_status_of_jobs(appID->job.id);

    verbose(VJOBPMON, "%s--- powermon_new_job (%lu,%lu) ---%s",
            COL_BLU,appID->job.id, appID->job.step_id,COL_CLR);

    if (powermon_is_idle()) powercap_idle_to_run();

    set_powercapstatus_mode(AUTO_CONFIG);

    powercap_new_job();

    if (pmapp == NULL) {
        if (new_context(appID->job.id, appID->job.step_id, &ccontext) == EAR_SUCCESS) {
            pmapp = current_ear_app[ccontext];
        } else {
            error("Maximum number of contexts reached, no more concurrent jobs supported.");
            return;
        }
    }

    state_t lock_st;
    if ((lock_st = ear_trylock(&powermon_app_mutex[ccontext])) != EAR_SUCCESS) {
        error("Locking context %lu/%lu for new job: %s",
              pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
        return;
    }

    /* Saving the context */
    /* TODO: We cannot assume all the node will have the same freq: PENDING. */
    pmapp->current_freq = frequency_get_cpu_freq(0);

    // We save the current list of priorities. If there is an error, we initialize the
    // prio_idx_list attribute to PRIO_SAME for robustness.
    if (state_fail(mgt_cpufreq_prio_get_current_list(pmapp->prio_idx_list))) {

        for (int i = 0; i < man.pri.devs_count; i++) {
            pmapp->prio_idx_list[i] = PRIO_SAME;
        }
    }

    if (VERB_ON(VPMON_DEBUG))
    {
        verbose(VPMON_DEBUG, "Priority system enabled: %d | Stored current priority list:",
                mgt_cpuprio_is_enabled());
        mgt_cpuprio_data_print((cpuprio_t *) man.pri.list1, pmapp->prio_idx_list, verb_channel);
    }

    pmapp->is_job = is_job;

    /* Returns the number of contexts for this job id.
     * If the value is 1, it means that the job is not from a batch script (e.g., srun). */
    if (is_job_in_node(appID->job.id, &alloc)) {
        /* It's an allocation by itself */
        if (alloc->num_ctx == 1) pmapp->is_job = 1;
    }
    verbose(VJOBPMON, "Is new job %d", pmapp->is_job);
    
    /* Setting userspace */
    uint user_type = get_user_type(&my_cluster_conf, appID->job.energy_tag, appID->job.user_id,
                                   appID->job.group_id, appID->job.user_acc, &my_tag);
    verbose(VJOBPMON + 1, "New job USER type: %u", user_type);

    if (my_tag != NULL) print_energy_tag(my_tag);

    /* Given a user type, application, and energy_tag,
     * my_policy is the cofiguration for this user and application. */
#if CPUFREQ_SET_ALL_USERS
    if (user_type != AUTHORIZED) appID->is_learning = 0;
    uint user_type_policy = AUTHORIZED;
#else
    uint user_type_policy = user_type;
#endif

    policy_conf_t *my_policy = configure_context(pmapp, user_type_policy, my_tag, appID); // Sets CPU and GPU frequency.

    set_default_powermon_init(pmapp);
#if NAIVE_SETTINGS_SAVE
    // We save the governor of CPU 0 to be restored later for all CPUs.
    save_global_cpu_settings(pmapp);
#endif

    verbose(VJOBPMON, "[Node configuration] policy: %u p_state: %d policy thresh: %lf is_mpi: %d",
            my_policy->policy, my_policy->p_state, my_policy->settings[0], appID->is_mpi);

    /* Updating info in shared memory region */

    /* Info shared with the job */
    pmapp->settings->id             = new_app_id;
    pmapp->settings->user_type      = user_type;

    if (user_type == AUTHORIZED) pmapp->settings->learning = appID->is_learning;
    else pmapp->settings->learning  = 0;

    pmapp->settings->lib_enabled    = (user_type!=ENERGY_TAG);

    verbose(VJOBPMON, "User type %u, etag %u lib_enabled %u",
            user_type, ENERGY_TAG, pmapp->settings->lib_enabled);
    pmapp->settings->policy         = my_policy->policy;

    strcpy(pmapp->settings->policy_name, my_policy->name);

    pmapp->settings->def_p_state    = my_policy->p_state;
    pmapp->settings->def_freq       = frequency_pstate_to_freq(my_policy->p_state);
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

    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++) pmapp->pc_app_info->req_f[i] = pmapp->settings->def_freq;

#if USE_GPUS
    pmapp->pc_app_info->num_gpus_used = gpu_mgr_num_gpus();
    for (uint gpuid = 0; gpuid < pmapp->pc_app_info->num_gpus_used ;gpuid++)
        pmapp->pc_app_info->req_gpu_f[gpuid] = my_node_conf->gpu_def_freq;


		verbose(VCONF, "GPU set to active monitor mode");
		gpu_set_monitoring_mode(MONITORING_MODE_RUN);
#endif

    powercap_set_app_req_freq();


    job_init_powermon_app(pmapp, eh, appID, from_mpi);

    /* TODO: We must report the energy beforesetting the job_id: PENDING */
    new_job_for_period(&current_sample, appID->job.id, appID->job.step_id);

    // save_eard_conf(&eard_dyn_conf);

    verbose(VJOBPMON_BASIC , "%sJob created ctx %d (%lu,%lu) is_mpi %d procs %lu def_cpuf %lu%s",
            COL_BLU, ccontext, current_ear_app[ccontext]->app.job.id,
            current_ear_app[ccontext]->app.job.step_id, current_ear_app[ccontext]->app.is_mpi,
            current_ear_app[ccontext]->app.job.procs, pmapp->settings->def_freq, COL_CLR);
    verbose(VJOBPMON , "*******************");

    pmapp->sig_reported = 0;
    
    ear_unlock(&powermon_app_mutex[ccontext]);
}


/* This function is called by dynamic_configuration thread when a end_job command arrives. */
void powermon_end_job(ehandler_t *eh, job_id jid, job_id sid, uint is_job) {

    // Application disconnected
    powermon_app_t summary, *pmapp;

    int curr_ctx;

    /* jid--sid can finish in a different order than expected */
    verbose(VJOBPMON , "*********** End job %lu/%lu ***********", jid, sid);

    curr_ctx = find_context_for_job(jid, sid);

    if (curr_ctx < 0) {
        error("At powermon_end_job: no context found (%lu, %lu).", jid, sid);
        return;
    }

    pmapp = current_ear_app[curr_ctx];

    state_t lock_st;
    if ((lock_st = ear_trylock(&powermon_app_mutex[curr_ctx])) != EAR_SUCCESS) {
        error("Locking context %lu/%lu ending the job: %s",
              pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
        return;
    }

    /* We set ccontex to the specific one */
    verbose(VJOBPMON_BASIC, "%spowermon end job (%lu, %lu)%s",COL_BLU, pmapp->app.job.id, pmapp->app.job.step_id,COL_CLR);

    uint ID = create_ID(jid, sid);

    job_end_powermon_app(pmapp, eh);

    // After that function, the avg power is computed
    memcpy(&summary, pmapp, sizeof(powermon_app_t));

    // we must report the current period for that job before the reset: PENDING
    end_job_for_period(&current_sample);

    report_powermon_app(&summary);

    set_null_loop(&pmapp->last_loop);
    //save_eard_conf(&eard_dyn_conf);

#if !POWERMON_RESTORE
    /* Restore Frequency and Governor: PENDING. We must restore GPU? We must do using the affinity mask*/
    verbose(VJOBPMON, "restoring governor %s", pmapp->governor.name);
    set_governor(&pmapp->governor);
    if (strcmp(pmapp->governor.name, "userspace") == 0) {
        frequency_set_all_cpus(pmapp->current_freq);
    }
#endif

    // reset_shared_memory();
    app_mgt_end_job(pmapp->app_info);

    pcapp_info_end_job(pmapp->pc_app_info);

    //ear_unlock(&powermon_app_mutex[curr_ctx]);

    powercap_end_job();

    end_context(curr_ctx, 0);

    if (summary.state != APP_FINISHED)
    {
        if (powermon_is_idle())
        {
#if USE_GPUS
					verbose(VCONF, "GPU set to idle monitor mode");
						gpu_set_monitoring_mode(MONITORING_MODE_IDLE);
#endif
            powercap_run_to_idle();

#if !NAIVE_SETTINGS_SAVE
            /* summary is a copy of the last context */
            set_default_powermon_end(&summary, 1);
#endif
        } else {
#if !NAIVE_SETTINGS_SAVE
            set_default_powermon_end(&summary, 0);
#endif
        }
#if NAIVE_SETTINGS_SAVE
        restore_global_cpu_settings(&summary);
#endif
    }

    verbose(VJOBPMON + 1, "Cleaning job from node_mgr");

    nodemgr_clean_job(jid, sid);

    clean_job_area(ID);
		ear_unlock(&powermon_app_mutex[curr_ctx]);

    verbose(VJOBPMON , "***************************************");

    service_close_by_id(jid, sid);
}


/*
 * These functions are called by dynamic_configuration thread: Used to notify when a configuracion setting is changed
 *
 */


void powermon_inc_th(uint p_id, double th)
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
    for (cc = 0; cc <= max_context_created; cc++) {
        if (current_ear_app[cc] != NULL) {

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

    verbose(VJOBPMON, "New default freq for policy %u set to %lu", p_id, def);

    ps = frequency_closest_pstate(def);

    /* Job running and not EAR-aware */
    for (cc = 0; cc <= max_context_created; cc++) {
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
    }

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


// TODO: Thread-save here.
void update_pmapps(power_data_t *last_pmon, nm_data_t *nm)
{
    verbose(VEARD_NMGR, "--- Updating power monitor apps Power ---");

    powermon_app_t *pmapp;
    ulong lcpus, tcpus = 0;
    float cpu_ratio;
    ulong num_jobs;
    ulong time_consumed;
    double my_dc_power;
    job_context_t *alloc = NULL;

    time_consumed = (ulong) difftime(last_pmon->end, last_pmon->begin);

    /* we must detect which ones are using the node, filtering ssh etc */

    num_jobs = num_jobs_in_node();
    if (num_jobs == 0) return;

    /* Set to 0 the num_cpus per job, we will compute again.
     * Oriol: Why we can't know the correct number of CPUs from the beginning? */
    clean_cpus_in_jobs();

    for (uint cc = 1; cc <= max_context_created; cc++) {

        if (current_ear_app[cc] != NULL) {

            pmapp = current_ear_app[cc];

            state_t lock_st;
            if ((lock_st = ear_trylock(&powermon_app_mutex[cc])) != EAR_SUCCESS) {
                error("Locking context %lu/%lu for updating its mask: %s",
                      pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
								uint ID;
								ID = create_ID(pmapp->app.job.id, pmapp->app.job.step_id);
								if (test_job_folder(ID) != EAR_SUCCESS){
									error("Application %lu/%lu exists for folder does not", pmapp->app.job.id, pmapp->app.job.step_id);
								}
                 continue;
            }

            /* We must update the current signature */
            pmapp->exclusive = (num_jobs == 1) && pmapp->exclusive;

            int job_in_node = is_job_in_node(pmapp->app.job.id, &alloc);

            if (pmapp->app.is_mpi) {
                // * is_mpi: = step + EARL *
                pmapp->earl_num_cpus = cpumask_count(&pmapp->plug_mask);
            } else if (!pmapp->is_job || // * else, !is_job = step without EARL *
                       (pmapp->is_job && // * else, is_job and 1 context = srun without sbatch *
                        job_in_node &&
                        alloc->num_ctx == 1)) {
                pmapp->earl_num_cpus  = ear_min(cpumask_count(&pmapp->plug_mask), pmapp->plug_num_cpus);
            }

            /* earl_num_cpus is > 0 when the app uses EARL */
            if (pmapp->earl_num_cpus && job_in_node) {

                verbose(VEARD_NMGR, "Job %lu found, aggregating %u CPUs... ",
                        pmapp->app.job.id, pmapp->earl_num_cpus);

                tcpus += pmapp->earl_num_cpus;
                alloc->num_cpus += pmapp->earl_num_cpus;
            }

            ear_unlock(&powermon_app_mutex[cc]);
        }
    }

    verbose_jobs_in_node(VCONF);

    for (uint cc = 1; cc <= max_context_created; cc++) {

        verbose(VEARD_NMGR, "Testing context %d", cc);

        if (current_ear_app[cc] != NULL) {

            pmapp = current_ear_app[cc];

            state_t lock_st;
            if ((lock_st = ear_trylock(&powermon_app_mutex[cc])) != EAR_SUCCESS) {
                error("Locking context %lu/%lu for testing its power: %s",
                      pmapp->app.job.id, pmapp->app.job.step_id, state_msg);
                return;
            }

            /* We must update the current signature */
            lcpus = pmapp->earl_num_cpus;
            if (lcpus) {

                cpu_ratio = (num_jobs > 1) ? (float) lcpus / (float) tcpus : 1;

                verbose(VEARD_NMGR,"Job %lu/%lu has %lu of %lu CPUs. Ratio: %f",
                        pmapp->app.job.id, pmapp->app.job.step_id, lcpus, tcpus, cpu_ratio);
            } else {

                if (is_job_in_node(pmapp->app.job.id, &alloc)) {

                    // cpu_ratio = (float) alloc->num_cpus / (float) tcpus;
										cpu_ratio = ((tcpus && alloc->num_cpus) ? (float) alloc->num_cpus / (float) tcpus : 1);

                    verbose(VEARD_NMGR, "Job %lu/%lu without node mask, using ratio %f = %u/%lu",
                            pmapp->app.job.id, pmapp->app.job.step_id, cpu_ratio, alloc->num_cpus, tcpus); 
                } else {
                    verbose(VEARD_NMGR,"Warning, no cpus detected using num_jobs %lu", num_jobs);
                    //cpu_ratio = 1.0 / (float) num_jobs;
                    cpu_ratio = (num_jobs ? 1.0 / (float) num_jobs : 1);
                }
            }

            my_dc_power = accum_node_power(last_pmon) * cpu_ratio;
            pmapp->accum_ps.DC_energy 	+= accum_node_power(last_pmon) * cpu_ratio * time_consumed;
            pmapp->accum_ps.DRAM_energy += accum_dram_power(last_pmon) * cpu_ratio * time_consumed;
            pmapp->accum_ps.PCK_energy 	+= accum_cpu_power(last_pmon)  * cpu_ratio * time_consumed;


            if (pmapp->app.is_mpi) {
                pmapp->accum_ps.avg_f += get_nm_cpufreq_with_mask(&my_nm_id, nm, pmapp->plug_mask) *
                                         time_consumed;
            } else {

                pmapp->accum_ps.avg_f += get_nm_cpufreq_with_mask(&my_nm_id, nm, pmapp->plug_mask) *
                                         time_consumed;
            }


            pmapp->accum_ps.max = ear_max(pmapp->accum_ps.max, my_dc_power);

            if (pmapp->accum_ps.min > 0) {
                pmapp->accum_ps.min = ear_min(pmapp->accum_ps.min, my_dc_power);
            }
            else {
                pmapp->accum_ps.min = my_dc_power;			
            }
#if USE_GPUS
            float gpu_ratio = 1;
            pmapp->accum_ps.GPU_energy = accum_gpu_power(last_pmon) * gpu_ratio * time_consumed;
#endif
            ear_unlock(&powermon_app_mutex[cc]);
        }
    }

    verbose(VEARD_NMGR,"-----------------------------------------");
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
    ulong jid, mpi, sid;
    ulong time_consumed;
    double maxpower, minpower, rapl_dram, rapl_pck, corrected_power,pckp,dramp, gpup, gpu_power, node_dom_power;
    powermon_app_t *pmapp;
    uint current_jobs_running_in_node = 0;

    /* last_pmon is the last short power monitor period info. pmon_info can be larger. Use last_pmon  for fine grain power metrics*/

    pmapp 	  = current_ear_app[max_context_created];
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

    while (pthread_mutex_trylock(&app_lock)); // TODO: Remove

    /* We compute max and mins */
    if (pmapp->app.job.id > 0) {
        if ((last_pmon->avg_dc > maxpower) && (last_pmon->avg_dc < my_node_conf->max_error_power)){
            pmapp->app.power_sig.max_DC_power = last_pmon->avg_dc;
        }
        if ((last_pmon->avg_dc < minpower) && (minpower>= my_node_conf->min_sig_power)){
            pmapp->app.power_sig.min_DC_power = last_pmon->avg_dc;
        }
    }

    pthread_mutex_unlock(&app_lock); // TODO: Remove

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
		if (isnormal(gpu_power)) current_sample.GPU_energy = gpu_power * time_consumed;
		else                      current_sample.GPU_energy = 0;
#endif
    current_sample.DC_energy = corrected_power * time_consumed;
    current_sample.temp = (ulong) get_nm_temp(&my_nm_id, nm);

    /* To be usd by status */
    copy_node_metrics(&my_nm_id, &last_nm, nm);

    /* Error control */
#if SYSLOG_MSG
    if ((last_pmon->avg_dc == 0)  ||
        (last_pmon->avg_dc < my_node_conf->min_sig_power) ||
        (last_pmon->avg_dc > my_node_conf->max_sig_power) ||
        (last_pmon->avg_dc > my_node_conf->max_error_power))
    {
        syslog(LOG_DAEMON|LOG_ERR,"Node %s reports %.2lf as avg power in last period\n",nodename,last_pmon->avg_dc);
        log_report_eard_rt_error(&rid, jid, sid, DC_POWER_ERROR, (llong) last_pmon->avg_dc);
    }
    if (current_sample.temp > my_node_conf->max_temp){
        syslog(LOG_DAEMON|LOG_ERR,"Node %s reports %lu degress (max %lu)\n",nodename,current_sample.temp,my_node_conf->max_temp);
        log_report_eard_rt_error(&rid, jid, sid, TEMP_ERROR, (llong) current_sample.temp);
    }
    if ((rapl_dram + rapl_pck) == 0) {
        log_report_eard_rt_error(&rid,jid,sid,RAPL_ERROR,0);
    }
    if ((current_sample.avg_f == 0) || ((current_sample.avg_f > frequency_get_nominal_freq()) && (mpi))){
        log_report_eard_rt_error(&rid, jid, sid, FREQ_ERROR, (llong) current_sample.avg_f);
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
    if (report) {
				periodic_metric_clean_before_db(&current_sample);
        report_connection_status = report_periodic_metrics(&rid, &current_sample, 1);
        last_power_reported      = corrected_power;
    }
    last_node_power              = last_pmon->avg_dc;

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
        if (fd_powermon >= 0) {
            if (write(fd_powermon, header, strlen(header)) != strlen(header)) {
                // Do something with the error
            }
        }
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

    jid = app->job.id;
    sid = app->job.step_id;

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

// TODO: Thread-save here.
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

		pmapp->app.job.local_id = app->job.local_id;
    pmapp->app.job.def_f = app->job.def_f;
    pmapp->app.job.th = app->job.th;
    pmapp->app.job.procs = app->job.procs;

    strcpy(pmapp->app.job.policy, app->job.policy);
    strcpy(pmapp->app.job.app_id, app->job.app_id);

    pmapp->sig_reported = 1;

    verbose(VJOBPMON, "MPI signature reported for context %d (%lu/%lu), avg. freq (kHz) = %lu",
            cc, app->job.id, app->job.step_id, pmapp->app.signature.avg_f);

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
    cpumask_aggregate(&in_jobs_mask, newjobmask);
}


void powermon_end_jobmask(cpu_set_t *endjobmask)
{
    cpumask_remove(&in_jobs_mask, endjobmask);
}


/*
 *
 *
 * MAIN POWER MONITORING THREAD
 *
 *
 */
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

    verbose(VJOBPMON, "[%ld] Power monitoring thread UP", syscall(SYS_gettid));

    init_contexts();
    powermon_create_idle_context();

    if (pthread_setname_np(pthread_self(), TH_NAME))
        error("Setting name for %s thread %s", TH_NAME, strerror(errno));

    if (init_power_monitoring(&my_eh_pm, &node_desc) != EAR_SUCCESS) {
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

    // We will collect and report avg power until eard finishes...

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
    openlog("eard", LOG_PID|LOG_PERROR, LOG_DAEMON);
#endif

    powermon_freq_list   = frequency_get_freq_rank_list();
    powermon_num_pstates = frequency_get_num_pstates();

    powermon_restore_cpufreq_list = calloc(man.cpu.devs_count, sizeof(uint));

#if USE_GPUS
    if (gpu_mgr_init() != EAR_SUCCESS) {
        error("Error initializing GPU management");
    }
#endif

    /* MAIN LOOP */
    debug("Starting power monitoring loop, reporting metrics every %d seconds", f_monitoring);

    verbose(VNODEPMON, "Power monitor thread set up. Waiting for other threads...");
    pthread_barrier_wait(&setup_barrier);

    while (!eard_must_exit) {
	// TEST	
        // finish_pending_contexts(&my_eh_pm);	

        verbose(VNODEPMON, "\n%s------------------- NEW PERIOD -------------------%s", COL_BLU,COL_CLR);
        // Wait for N secs
        sleep(f_monitoring);

        // Get time and Energy
        read_enegy_data(&my_eh_pm, &e_end);

        if (e_end.DC_node_energy == 0) {
            warning("Power monitor period invalid energy reading, continue");
        } else {

            // Get node metrics
            end_compute_node_metrics(&my_nm_id, &nm_end);
            diff_node_metrics(&my_nm_id, &nm_init, &nm_end, &nm_diff);
            start_compute_node_metrics(&my_nm_id, &nm_init);

            // Compute the power
            compute_power(&e_begin, &e_end, &my_current_power);
            // print_power(&my_current_power,1,-1);

            // Powercap
            current_periods++;

            report_data = (current_periods == report_periods);
            if (report_data) {
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


void powermon_report_event(uint event_type, llong value)
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
    int contexts_created = 0;
    int contexts_created_finished = 0;

    for (int i = 0; i <= max_context_created; i++)
    {
        powermon_app_t *pmapp = current_ear_app[i];
        if (pmapp)
        {
            contexts_created++;
            if (pmapp->state == APP_FINISHED)
            {
                contexts_created_finished++;
            }
        }
    }
    return (!max_context_created || contexts_created_finished == contexts_created);
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


static void set_default_powermon_init(powermon_app_t *app)
{
    /* IMC freq must be set to default conf. EAR can change in eUFS is ON */
    /* CPUfreq must be changed only if policy_freq is set , otherwise EARD is 
     * not "forcing freqs " */
    /* GPU freq should be improved, for now, we set the default freq to all the 
     * GPUS */

#if POWERMON_RESTORE
    /* IMC freq */
    verbose(VCONF, "Setting default IMC frequency...");
    if (node_desc.vendor == VENDOR_INTEL) {
        mgt_imcfreq_set_auto(no_ctx);
    } else if (node_desc.vendor == VENDOR_AMD) {
        mgt_imcfreq_set_current(no_ctx, 0, all_sockets);
    }

    /* If policy_freq is zero, we don't need to change the current configuration:
     * EARL is not used and !ForceFrequencies */
    if (app->policy_freq)
    {
#if USE_GPUS
        /* GPU freq. */
        if (my_node_conf->gpu_def_freq > 0)
        {
            verbose(VCONF, "Setting default GPU freq.: %.2f GHz",
                    (float) my_node_conf->gpu_def_freq / 1000000);

            gpu_mgr_set_freq_all_gpus(my_node_conf->gpu_def_freq);
        }
#endif
    }
#endif // POWERMON_RESTORE
}


#if !NAIVE_SETTINGS_SAVE
static void set_default_powermon_end(powermon_app_t *app, uint idle)
{
#if POWERMON_RESTORE

  /* Two scenarios are possible for CPU freq:
   * 1) End job: Set idle gov and freq
   * 2) No end job : if prev set --> restore
   */

  uint change_gov = 0;
  uint rest_pstate;

  /* Going to idle: Reset CPU freq. */
  if (idle) mgt_cpufreq_reset(no_ctx);

  char *cgov = app->governor.name; // Governor before job had started.
  uint gov;

	// TODO: Filter below conditional block if policy_freq true

  // Old condition: app->app.job.step_id != (job_id) BATCH_STEP
  if (app->is_job) { // End of a job

    if (strcmp(my_node_conf->idle_governor, "default") != 0) {

                /* We'll set what it is configured in the ear.conf (idle governor and P-State). */
                cgov        = my_node_conf->idle_governor;
                rest_pstate = ear_min(my_node_conf->idle_pstate, man.cpu.list1_count - 1);

    } else {
      /* We'll restore the P-State we found before job had started. */
      mgt_cpufreq_get_index(no_ctx, app->current_freq, &rest_pstate, 1);
    }

    /* --- Setting governor --- */

		// TODO: Check whether this function returns SUCCESS
		// and then do the below conditional block.
    governor_toint(cgov, &gov);

    if (idle) {

      verbose(VCONF, "(Idle) Restoring all the node CPUs to governor `%s`...", cgov);

      mgt_cpufreq_governor_set(no_ctx, gov);

    } else {

      verbose(VCONF, "[End job %lu/%lu] Restoring to governor `%s` the CPU set in the job's mask...",
          app->app.job.id, app->app.job.step_id, cgov);
      verbose_affinity_mask(VCONF + 2, &app->plug_mask, man.cpu.devs_count);

      mgt_cpufreq_governor_set_mask(no_ctx, gov, app->plug_mask);
    }

    if (strcmp(cgov, "userspace") == 0) {
      change_gov = 1;
    }

    /* ------------------------ */

  } else if (app->policy_freq) { // Not the end of a job but EARD set a frequency

    /* --- Setting governor --- */

    governor_toint(cgov, &gov);

    verbose(VCONF, "[%lu/%lu] Restoring to governor `%s` the CPU set in the mask...",
        app->app.job.id, app->app.job.step_id, cgov);
    verbose_affinity_mask(VCONF + 2, &app->plug_mask, man.cpu.devs_count);

    mgt_cpufreq_governor_set_mask(no_ctx, gov, app->plug_mask);

    if (strcmp(cgov, "userspace") == 0) change_gov = 1;

    /* ------------------------ */

    /* We'll restore the P-State we found before job had started. */
    mgt_cpufreq_get_index(no_ctx, app->current_freq, &rest_pstate, 1);

  }

  /* If the governor restored is `userspace`, we must manually set a CPU freq. */
  if (change_gov) {

    if (idle) {

      verbose(VCONF, "(Idle) Restoring to all CPUs pstate %u all_cpus", rest_pstate);

      mgt_cpufreq_set_current(no_ctx, rest_pstate, all_cpus);

    } else {

      verbose(VCONF, "[%lu/%lu] Restoring to CPU pstate %u with mask",
              app->app.job.id, app->app.job.step_id, rest_pstate);
      verbose_affinity_mask(VCONF + 2, &app->plug_mask, man.cpu.devs_count);

      fill_cpufreq_list(&app->plug_mask, man.cpu.devs_count, rest_pstate,
                        ps_nothing, powermon_restore_cpufreq_list);

      mgt_cpufreq_set_current_list(no_ctx, powermon_restore_cpufreq_list);
    }
  }

  /* If the priority module is enabled, we restore the priority of the CPUs of the job. */
  if (mgt_cpuprio_is_enabled()) {
    for (int i = 0; i < man.pri.devs_count; i++) {
      // If the CPU is not in the job mask, we won't touch the priority (PRIO_SAME).
      app->prio_idx_list[i] = (CPU_ISSET(i, &app->plug_mask)) ? app->prio_idx_list[i] : PRIO_SAME; // TODO: Thread-save?
    }

    if (VERB_ON(VPMON_DEBUG)) {
      verbose(VPMON_DEBUG, "Restoring priority list...");
      mgt_cpuprio_data_print((cpuprio_t *) man.pri.list1, app->prio_idx_list, verb_channel);
    }
    if (state_fail(mgt_cpuprio_set_current_list(app->prio_idx_list))) {
      error("Restoring the priority list for job %lu/%lu.", app->app.job.id, app->app.job.step_id);
    }

    if (idle) {
      verbose(VPMON_DEBUG, "Disabling priority system...");
      if (state_fail(mgt_cpuprio_disable())) {
        error("Disabling priority system.");
      }
    }
  }

#if USE_GPUS
  /* GPUs */
#if 0
  if (idle && app->policy_freq && my_node_conf->gpu_def_freq)
  {
    /* EARD is not aware of the GPUs used */
    ulong idle_gpu_freq;
    /* We assume all the gpus are the same */
    gpu_mgr_get_min(0, &idle_gpu_freq);
    verbose(VCONF, "Set idle GPU freq %.2f GHz", (float) idle_gpu_freq / 1000000.0);
    gpu_mgr_set_freq_all_gpus(idle_gpu_freq);
  }
#endif
#endif

  if (idle)
  {
    /* IMC freq : It's already done at init, but it's included here for simplicity*/
    verbose(VCONF, "Restoring IMC freq");
    if (node_desc.vendor == VENDOR_INTEL)
    {
      mgt_imcfreq_set_auto(no_ctx);
    } else if (node_desc.vendor == VENDOR_AMD)
    {
      mgt_imcfreq_set_current(no_ctx, 0, all_sockets);
    }
  }
#endif // POWERMON_RESTORE
}
#endif // !NAIVE_SETTINGS_SAVE


#if !NAIVE_SETTINGS_SAVE
static state_t store_current_task_governor(powermon_app_t *powermon_app, cpu_set_t *mask)
{
    if (powermon_app != NULL && mask != NULL) {
        // Clear
        powermon_app->governor.max_f = 0LU;
        powermon_app->governor.min_f = 0LU;

        sprintf(powermon_app->governor.name, "%s", Goverstr.unknown);

        uint *governor_list = (uint *) malloc(man.cpu.devs_count * sizeof(uint));
        if (governor_list) {
            if (state_ok(mgt_cpufreq_governor_get_list(no_ctx, governor_list))) {

                int i = 0;
                while (i < man.cpu.devs_count && !CPU_ISSET(i, mask)) {
                    i++;
                }

                if (i < man.cpu.devs_count) {
                    mgt_governor_tostr(governor_list[i], powermon_app->governor.name);

                    powermon_app->governor.max_f = frequency_pstate_to_freq(0);
                    powermon_app->governor.min_f = frequency_pstate_to_freq(man.cpu.list1_count - 1);

                } else {
                    verbose(VCONF, "%sWARNING%s No CPU found in mask.", COL_YLW, COL_CLR);
                }
            }
        } else {
            return_msg(EAR_ERROR, Generr.alloc_error); // Error on malloc
        }

        free(governor_list);

        return EAR_SUCCESS;
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}
#endif // !NAIVE_SETTINGS_SAVE


/* This function is called to validate the jobs in the list that are still alive.
 * Only usable in SLURM systems. */
static void check_status_of_jobs(job_id current)
{
		return; // Disabled
    powermon_app_t *pmapp; 
    for (uint cc = 1; cc <= max_context_created; cc++)
    {
        if (current_ear_app[cc] != NULL)
        {
            pmapp = current_ear_app[cc];
            verbose(VJOBPMON_BASIC, "checking app %lu curr job: %lu", pmapp->app.job.id, current);
            /* No need to check the new one :) */
            if (pmapp->app.job.id != current)
            {
                // if (is_job_running(ear_tmp, pmapp->app.job.id) == 0)
								if (is_job_step_valid(pmapp->app.job.id, pmapp->app.job.step_id) == 0)
                {
                    verbose(VJOBPMON_BASIC, "Warning, we must clean  job %lu", pmapp->app.job.id);
										pmapp->state = APP_FINISHED;
                }
            }
        }
    }
}


#if NAIVE_SETTINGS_SAVE
static state_t save_global_cpu_settings(powermon_app_t *powermon_app)
{
    if (powermon_app)
    {
        // Saving the governor found at CPU 0

        powermon_app->governor.max_f = 0LU;
        powermon_app->governor.min_f = 0LU;

        sprintf(powermon_app->governor.name, "%s", Goverstr.unknown);

        uint governor_id;
        if (state_ok(mgt_cpufreq_get_governor(no_ctx, &governor_id)))
        {
            mgt_governor_tostr(governor_id, powermon_app->governor.name);

            powermon_app->governor.max_f = frequency_pstate_to_freq(0);
            powermon_app->governor.min_f = frequency_pstate_to_freq(man.cpu.list1_count - 1);
        }

        // Saving the frequency of CPU 0
        powermon_app->current_freq = frequency_get_cpu_freq(0);
    } else
    {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}


static state_t restore_global_cpu_settings(powermon_app_t *powermon_app)
{
    if (powermon_app)
    {
        // Saving the governor found at CPU 0

        uint change_gov = 0;
        uint rest_pstate;

        /* Going to idle: Reset CPU freq. */
        if (powermon_is_idle()) mgt_cpufreq_reset(no_ctx);

        char *cgov = powermon_app->governor.name; // Governor before job had started.
        uint gov_id;

        if (powermon_app->is_job || powermon_app->policy_freq) { 
            // End of a job or Not the end of a job but EARD set a frequency

            if (powermon_app->is_job && strcmp(my_node_conf->idle_governor, "default") != 0) {

                /* We'll set what it is configured in the ear.conf (idle governor and P-State). */
                cgov        = my_node_conf->idle_governor;
                rest_pstate = ear_min(my_node_conf->idle_pstate, man.cpu.list1_count - 1);

            } else {
                /* We'll restore the P-State we found before job had started. */
                mgt_cpufreq_get_index(no_ctx, powermon_app->current_freq, &rest_pstate, 1);
            }

            /* --- Setting governor --- */

            governor_toint(cgov, &gov_id);

            verbose(VCONF, "Restoring all the node CPUs to governor `%s`...", cgov);

            mgt_cpufreq_governor_set(no_ctx, gov_id);

            if (strcmp(cgov, "userspace") == 0) {
                change_gov = 1;
            }

            /* ------------------------ */

        }

        /* If the governor restored is `userspace`, we must manually set a CPU freq. */
        if (change_gov) {

            verbose(VCONF, "Restoring to all CPUs pstate %u...", rest_pstate);

            mgt_cpufreq_set_current(no_ctx, rest_pstate, all_cpus);

        }

        /* If the priority module is enabled, we restore the priority of the CPUs of the job. */
        if (mgt_cpuprio_is_enabled()) {
            for (int i = 0; i < man.pri.devs_count; i++) {
                // If the CPU is not in the job mask, we won't touch the priority (PRIO_SAME).
                powermon_app->prio_idx_list[i] = (CPU_ISSET(i, &powermon_app->plug_mask)) ? powermon_app->prio_idx_list[i] : PRIO_SAME; // TODO: Thread-save?
            }

            if (VERB_ON(VPMON_DEBUG)) {
                verbose(VPMON_DEBUG, "Restoring priority list...");
                mgt_cpuprio_data_print((cpuprio_t *) man.pri.list1, powermon_app->prio_idx_list, verb_channel);
            }

            if (state_fail(mgt_cpuprio_set_current_list(powermon_app->prio_idx_list))) {
                error("Restoring the priority list for job %lu/%lu.", powermon_app->app.job.id, powermon_app->app.job.step_id);
            }

            if (powermon_is_idle()) {
                verbose(VPMON_DEBUG, "Disabling priority system...");
                if (state_fail(mgt_cpuprio_disable())) {
                    error("Disabling priority system.");
                }
            }
        }

#if USE_GPUS
        /* GPUs */
        if (powermon_is_idle() && my_node_conf->gpu_def_freq) { /* EARD is not aware of the GPUs used */
            ulong idle_gpu_freq;
            /* We assume all the gpus are the same */
            gpu_mgr_get_min(0, &idle_gpu_freq);
            verbose(VCONF, "Restoring to GPU freq %.2f GHz", (float) idle_gpu_freq / 1000000.0);
            gpu_mgr_set_freq_all_gpus(idle_gpu_freq);
        }
#endif

        if (powermon_is_idle()) {
            /* IMC freq : It's already done at init, but it's included here for simplicity */
            verbose(VCONF, "Restoring IMC freq");
            if (node_desc.vendor == VENDOR_INTEL) {
                mgt_imcfreq_set_auto(no_ctx);
            } else if (node_desc.vendor == VENDOR_AMD) {
                mgt_imcfreq_set_current(no_ctx, 0, all_sockets);
            }
        }
    } else
    {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}
#endif // NAIVE_SETTINGS_SAVE
