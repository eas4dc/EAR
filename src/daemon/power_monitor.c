/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#define _GNU_SOURCE

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

#include <common/config.h>
#include <common/system/sockets.h>

//#define SHOW_DEBUGS 0

#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/application.h>
#include <common/types/periodic_metric.h>
#include <common/types/log_eard.h>
#include <common/types/configuration/cluster_conf.h>
#include <metrics/frequency/frequency_cpu.h>
#include <common/hardware/hardware_info.h>
#include <common/hardware/frequency.h>
#include <daemon/power_monitor.h>
#include <daemon/node_metrics.h>
#include <daemon/eard_checkpoint.h>
#include <daemon/shared_configuration.h>

#if SYSLOG_MSG
#include <syslog.h>
#endif

#if USE_DB
#include <database_cache/eardbd_api.h>
#include <common/database/db_helper.h>
#endif
static pthread_mutex_t app_lock = PTHREAD_MUTEX_INITIALIZER;
static ehandler_t my_eh_pm;
static char *TH_NAME = "PowerMon";
static int num_packs;

nm_t my_nm_id;
nm_data_t nm_init, nm_end, nm_diff, last_nm;

extern topology_t node_desc;

extern int eard_must_exit;
extern char ear_tmp[MAX_PATH_SIZE];
extern my_node_conf_t *my_node_conf;
extern cluster_conf_t my_cluster_conf;
extern eard_dyn_conf_t eard_dyn_conf;
extern policy_conf_t default_policy_context, energy_tag_context, authorized_context;
extern int ear_ping_fd;
extern int ear_fd_ack[1];
extern int application_id;

/* This variable controls the frequency for periodic power monitoring */
extern uint f_monitoring;
extern ulong current_node_freq;
int idleNode = 1;


extern char nodename[MAX_PATH_SIZE];
static int fd_powermon = -1;
static int fd_periodic = -1;
extern settings_conf_t *dyn_conf;
extern resched_t *resched_conf;
static int sig_reported = 0;

periodic_metric_t current_sample;
double last_power_reported = 0;
static energy_data_t c_energy;



/****************** CONTEXT MANAGEMENT ********************/
#define MAX_NESTED_LEVELS 16384
#define NULL_SID 4294967294
static powermon_app_t *current_ear_app[MAX_NESTED_LEVELS], default_app;
static int current_batch;
static int ccontext = -1;
static int num_contexts = 0;

#define check_context(msg) \
if (ccontext<0){ \
    error(msg);\
    return;\
}

void init_contexts() {
	int i;
	for (i = 0; i < MAX_NESTED_LEVELS; i++) current_ear_app[i] = NULL;
	current_batch = -1;
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
	while ((i < MAX_NESTED_LEVELS) && (pos < 0)) {
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
void end_context(int cc) {
	debug("end_context %d", cc);
	check_context("end_context: invalid job context");
	if (current_ear_app[cc] != NULL) {
		#ifndef EAR_CPUPOWER
		if (current_ear_app[cc]->governor.governor != NULL) free(current_ear_app[cc]->governor.governor);
		#endif
	  free_energy_data(&current_ear_app[cc]->energy_init);
		free(current_ear_app[cc]);
		current_ear_app[cc] = NULL;
		num_contexts--;
	}
}

/* This function will be called when job ends to be sure there is not steps pending to clean*/
void clean_job_contexts(job_id id) {
	int i, cleaned = 0;
	debug("clean_job_contexts for job %lu", id);
	for (i = 0; i < MAX_NESTED_LEVELS; i++) {
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
	for (i = 0; i < MAX_NESTED_LEVELS; i++) {
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


int new_context(job_id id, job_id sid) {
	debug("new_context for %lu %lu", id, sid);
	if (num_contexts == MAX_NESTED_LEVELS) {
		error("Panic: Maximum number of levels reached in new_job %d", ccontext);
		return EAR_ERROR;
	}
	int pos = find_empty_context();
	if (pos < 0) {
		error("Panic: Maximum number of levels reached in new_job %d", ccontext);
		return EAR_ERROR;
	}
	ccontext = pos;
	current_ear_app[ccontext] = (powermon_app_t *) malloc(sizeof(powermon_app_t));
	if (current_ear_app[ccontext] == NULL) {
		error("Panic: malloc returns NULL for current context");
		return EAR_ERROR;
	}
	memset(current_ear_app[ccontext], 0, sizeof(powermon_app_t));
	alloc_energy_data(&current_ear_app[ccontext]->energy_init);
	/* This info will be overwritten later */
	current_ear_app[ccontext]->app.job.id = id;
	current_ear_app[ccontext]->app.job.step_id = sid;
	debug("New context created %d", ccontext);
	if ((sid == NULL_SID) && (num_contexts > 0)) {
		/* This case means some messages have been lost, we must clean them */
		clean_contexts_diff_than(id);
	}
	num_contexts++;
	return EAR_SUCCESS;
}


/****************** END CONTEXT MANAGEMENT ********************/

powermon_app_t *get_powermon_app() {
	if (ccontext >= 0) return current_ear_app[ccontext];
	else return &default_app;
}

static void PM_my_sigusr1(int signun) {
	verbose(VNODEPMON, " thread %u receives sigusr1", (uint) pthread_self());
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

void reset_shared_memory()
{
    policy_conf_t *my_policy;
    my_policy=get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
    dyn_conf->user_type=NORMAL;
    dyn_conf->learning=0;
    dyn_conf->lib_enabled=1;
    dyn_conf->policy=my_cluster_conf.default_policy;
    dyn_conf->def_freq=frequency_pstate_to_freq(my_policy->p_state);
    dyn_conf->def_p_state=my_policy->p_state;
    strcpy(dyn_conf->policy_name, my_policy->name);
    memcpy(dyn_conf->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
}

void clean_job_environment(int id, int step_id) {
	char ear_ping[MAX_PATH_SIZE * 2], fd_lock_filename[MAX_PATH_SIZE * 2], ear_commack[MAX_PATH_SIZE * 2];
	uint pid = create_ID(id, step_id);

	if (ear_ping_fd >= 0) {
		close(ear_ping_fd);
		ear_ping_fd = -1;
	}
	if (ear_fd_ack[0] >= 0) {
		close(ear_fd_ack[0]);
		ear_fd_ack[0] = -1;
	}
	application_id = -1;
	sprintf(ear_ping, "%s/.ear_comm.ping.%u", ear_tmp, pid);
	sprintf(fd_lock_filename, "%s/.ear_app_lock.%u", ear_tmp, pid);
	sprintf(ear_commack, "%s/.ear_comm.ack_0.%u", ear_tmp, pid);
	verbose(VJOBPMON + 1, "Cleanning the environment");
	verbose(VJOBPMON + 1, "Releasing %s - %s - %s ", ear_ping, fd_lock_filename, ear_commack);
	unlink(ear_ping);
	unlink(fd_lock_filename);
	unlink(ear_commack);
#if SYSLOG_MSG
	closelog();
#endif


}

void copy_powermon_app(powermon_app_t *dest, powermon_app_t *src) {
	if (dest == NULL || src == NULL) {
		error("copy_powermon_app: NULL prointers provided");
		return;
	}
	debug("copy_powermon_app");
	dest->job_created = src->job_created;
	copy_energy_data(&dest->energy_init,&src->energy_init);
	copy_application(&(dest->app), &(src->app));
	#ifndef EAR_CPUPOWER
	dest->governor.min = src->governor.min;
	dest->governor.max = src->governor.max;
	dest->governor.governor = (char *) malloc(strlen(src->governor.governor) + 1);
	strcpy(dest->governor.governor, src->governor.governor);
	#else
	dest->governor.min_f = src->governor.min_f;
	dest->governor.max_f = src->governor.max_f;
	strcpy(dest->governor.name, src->governor.name);
	#endif
	dest->current_freq = src->current_freq;

}


#if DB_FILES
char database_csv_path[PATH_MAX];

void report_application_in_file(application_t *app)
{
	int ret1,ret2;
	ret2 = append_application_text_file(database_csv_path, app,app->is_mpi);
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

void job_init_powermon_app(ehandler_t *ceh, application_t *new_app, uint from_mpi) {
	check_context("job_init_powermon_app: ccontext<0, not initialized");
	current_ear_app[ccontext]->job_created = !from_mpi;
	copy_application(&current_ear_app[ccontext]->app, new_app);
	strcpy(current_ear_app[ccontext]->app.node_id, nodename);
	time(&current_ear_app[ccontext]->app.job.start_time);
	current_ear_app[ccontext]->app.job.start_mpi_time = 0;
	current_ear_app[ccontext]->app.job.end_mpi_time = 0;
	current_ear_app[ccontext]->app.job.end_time = 0;
	// reset signature
	signature_init(&current_ear_app[ccontext]->app.signature);
	init_power_signature(&current_ear_app[ccontext]->app.power_sig);
	current_ear_app[ccontext]->app.power_sig.max_DC_power = 0;
	current_ear_app[ccontext]->app.power_sig.min_DC_power = 500;
	current_ear_app[ccontext]->app.power_sig.def_f = dyn_conf->def_freq;
	// Initialize energy
	read_enegy_data(ceh, &c_energy);
	copy_energy_data(&current_ear_app[ccontext]->energy_init, &c_energy);
	aperf_job_avg_frequency_init_all_cpus();
}


void job_end_powermon_app(ehandler_t *ceh) {
	power_data_t app_power;
	alloc_power_data(&app_power);
	check_context("job_end_powermon_app");
	time(&current_ear_app[ccontext]->app.job.end_time);
	if (current_ear_app[ccontext]->app.job.end_time == current_ear_app[ccontext]->app.job.start_time) {
		current_ear_app[ccontext]->app.job.end_time = current_ear_app[ccontext]->app.job.start_time + 1;
	}
	// Get the energy
	read_enegy_data(ceh, &c_energy);
	compute_power(&current_ear_app[ccontext]->energy_init, &c_energy, &app_power);

	current_ear_app[ccontext]->app.power_sig.DC_power = accum_node_power(&app_power);
	if ((app_power.avg_dc > current_ear_app[ccontext]->app.power_sig.max_DC_power) && (app_power.avg_dc<my_node_conf->max_error_power)){
		current_ear_app[ccontext]->app.power_sig.max_DC_power = app_power.avg_dc;
	}
	if (app_power.avg_dc < current_ear_app[ccontext]->app.power_sig.min_DC_power){
		current_ear_app[ccontext]->app.power_sig.min_DC_power = app_power.avg_dc;
	}
	current_ear_app[ccontext]->app.power_sig.DRAM_power = accum_dram_power(&app_power);
	current_ear_app[ccontext]->app.power_sig.PCK_power = accum_cpu_power(&app_power);
	current_ear_app[ccontext]->app.power_sig.time = difftime(app_power.end, app_power.begin);
	current_ear_app[ccontext]->app.power_sig.avg_f = aperf_job_avg_frequency_end_all_cpus();

	// nominal is still pending

	// Metrics are not reported in this function
	free_power_data(&app_power);
}

void report_powermon_app(powermon_app_t *app) {
	// We can write here power information for this job
	if (app == NULL) {
		error("report_powermon_app: NULL app");
		return;
	}
	if (sig_reported == 0) {
		verbose(VJOBPMON + 1, "Reporting not mpi application");
		app->app.is_mpi = 0;
	}

	verbose_application_data(0, &app->app);
	report_application_in_file(&app->app);

#if USE_DB
	if (my_cluster_conf.eard.use_mysql)
	{
		if (!my_cluster_conf.eard.use_eardbd) {
			if (!db_insert_application(&app->app)){
				debug( "Application signature correctly written");
			}
		}
		else {
			edb_state_t state = eardbd_send_application(&app->app);

			if (edb_state_fail(state)) {
				error("Error when sending application to eardb");
				eardbd_reconnect(&my_cluster_conf, my_node_conf, state);
			}
		}
	}
#endif
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
	debug("configuring policy for user %u policy %s freq %lu th %lf is_learning %u is_mpi %d force_freq %d",user_type,appID->job.policy,appID->job.def_f,appID->job.th,appID->is_learning,appID->is_mpi,my_cluster_conf.eard.force_frequencies);
	switch (user_type){
	case NORMAL:
		appID->is_learning=0;
		debug("Policy requested %s",appID->job.policy);
    p_id=policy_name_to_id(appID->job.policy, &my_cluster_conf);
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
            int mo_pid = policy_name_to_id("monitoring", &my_cluster_conf);
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
            int mo_pid = policy_name_to_id("MONITORING_ONLY", &my_cluster_conf);
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
			p_id=policy_name_to_id(appID->job.policy, &my_cluster_conf);
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
                    int mo_pid = policy_name_to_id("MONITORING_ONLY", &my_cluster_conf);
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
        int mo_pid = policy_name_to_id("monitoring", &my_cluster_conf);
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
		verbose(VJOBPMON, "Setting userspace governor pstate=%u", my_policy->p_state);
		frequency_set_userspace_governor_all_cpus();
		f = frequency_pstate_to_freq(my_policy->p_state);
		frequency_set_all_cpus(f);
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
// That functions controls the mpi init/end of jobs . These functions are called by eard when application executes mpi_init/mpi_finalized and contacts eard

void powermon_mpi_init(ehandler_t *eh, application_t *appID) {
	if (appID == NULL) {
		error("powermon_mpi_init: NULL appID");
		return;
	}
	verbose(VJOBPMON + 1, "powermon_mpi_init job_id %lu step_id %lu (is_mpi %u)", appID->job.id, appID->job.step_id,
			appID->is_mpi);
	// As special case, we will detect if not job init has been specified
	if ((ccontext < 0) || ((ccontext >= 0) &&
						   (!current_ear_app[ccontext]->job_created))) {    // If the job is nt submitted through slurm, new_job would not be submitted
		powermon_new_job(eh, appID, 1);
	}
	// MPI_init : It only changes mpi_init time, we don't need to acquire the lock
	start_mpi(&current_ear_app[ccontext]->app.job);
	current_ear_app[ccontext]->app.is_mpi = 1;
	save_eard_conf(&eard_dyn_conf);
}

void powermon_mpi_finalize(ehandler_t *eh) {
	check_context("powermon_mpi_finalize and not job context created");
	verbose(VJOBPMON + 1, "powermon_mpi_finalize (%lu,%lu)", current_ear_app[ccontext]->app.job.id,
			current_ear_app[ccontext]->app.job.step_id);
	end_mpi(&current_ear_app[ccontext]->app.job);
	if (!current_ear_app[ccontext]->job_created) {  // If the job is not submitted through slurm, end_job would not be submitted
		powermon_end_job(eh, current_ear_app[ccontext]->app.job.id, current_ear_app[ccontext]->app.job.step_id);
	}
}

/*
*
*
*   JOB PART
*
*/

/* This functiono is called by dynamic_configuration thread when a new_job command arrives */


void powermon_new_job(ehandler_t *eh, application_t *appID, uint from_mpi) {
	// New application connected
	if (appID == NULL) {
		error("powermon_new_job: NULL appID");
		return;
	}
	uint new_app_id = create_ID(appID->job.id, appID->job.step_id);
	energy_tag_t *my_tag;
	policy_conf_t *my_policy;
	ulong f;
	uint user_type;
	verbose(VJOBPMON, "powermon_new_job (%lu,%lu)", appID->job.id, appID->job.step_id);
	if (new_context(appID->job.id, appID->job.step_id) != EAR_SUCCESS) {
		error("Maximum number of contexts reached, no more concurrent jobs supported");
		return;
	}
	/* Saving the context */
	check_context("powermon_new_job: after new_context!");
	current_ear_app[ccontext]->current_freq = frequency_get_cpu_freq(0);
	get_governor(&current_ear_app[ccontext]->governor);
	#ifndef EAR_CPUPOWER
	verbose(VJOBPMON, "Saving governor %s", current_ear_app[ccontext]->governor.governor);
	#else
	debug("Saving governor %s", current_ear_app[ccontext]->governor.name);
	#endif
	/* Setting userspace */
	user_type = get_user_type(&my_cluster_conf, appID->job.energy_tag, appID->job.user_id, appID->job.group_id,
							  appID->job.user_acc, &my_tag);
	verbose(VJOBPMON + 1, "New job USER type is %u", user_type);
	if (my_tag != NULL) print_energy_tag(my_tag);
	/* Given a user type, application, and energy_tag, my_policy is the cofiguration for this user and application */
	my_policy=configure_context(user_type, my_tag, appID);
	debug("Node configuration for policy %u p_state %d th %lf",my_policy->policy,my_policy->p_state,my_policy->settings[0]);
	/* Updating info in shared memory region */
	f=frequency_pstate_to_freq(my_policy->p_state);
	dyn_conf->id=new_app_id;
	dyn_conf->user_type=user_type;
	if (user_type==AUTHORIZED) dyn_conf->learning=appID->is_learning;
	else dyn_conf->learning=0;
	dyn_conf->lib_enabled=(user_type!=ENERGY_TAG);
	dyn_conf->policy=my_policy->policy;
    strcpy(dyn_conf->policy_name,  my_policy->name);
	dyn_conf->def_freq=f;
	dyn_conf->def_p_state=my_policy->p_state;
    memcpy(dyn_conf->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
	/* End app configuration */
	current_node_freq = f;
	appID->job.def_f = dyn_conf->def_freq;
	while (pthread_mutex_trylock(&app_lock));
	idleNode = 0;
	job_init_powermon_app(eh, appID, from_mpi);
	/* We must report the energy beforesetting the job_id: PENDING */
	new_job_for_period(&current_sample, appID->job.id, appID->job.step_id);
	pthread_mutex_unlock(&app_lock);
	save_eard_conf(&eard_dyn_conf);
	verbose(VJOBPMON + 1, "Job created (%lu,%lu) is_mpi %d", current_ear_app[ccontext]->app.job.id,
			current_ear_app[ccontext]->app.job.step_id, current_ear_app[ccontext]->app.is_mpi);
	verbose(VJOBPMON + 1, "*******************");
	sig_reported = 0;

}


/* This function is called by dynamic_configuration thread when a end_job command arrives */

void powermon_end_job(ehandler_t *eh, job_id jid, job_id sid) {
	// Application disconnected
	powermon_app_t summary;
	int cc, pcontext;
	/* Saving ccontext index */
	pcontext = ccontext;
	/*check_context("powermon_end_job: and not current context");*/
	/* jid,sid can finish in a different order than expected */
	debug("powermon_end_job %lu %lu", jid, sid);
	cc = find_context_for_job(jid, sid);
	if (cc < 0) {
		error("powermon_end_job %lu,%lu and no context created for it", jid, sid);
		return;
	}
	if (cc != ccontext) {
		warning("End job (%lu,%lu) is not current context selected=%d current=%d", jid, sid, cc, ccontext);
	}
	/* We set ccontex to the specific one */
	ccontext = cc;
	verbose(VJOBPMON, "powermon_end_job (%lu,%lu)", current_ear_app[ccontext]->app.job.id,
			current_ear_app[ccontext]->app.job.step_id);
	while (pthread_mutex_trylock(&app_lock));
	idleNode = 1;
	job_end_powermon_app(eh);
	// After that function, the avg power is computed
	memcpy(&summary, current_ear_app[ccontext], sizeof(powermon_app_t));
	// we must report the cur.rent period for that job before the reset:PENDING
	end_job_for_period(&current_sample);
	pthread_mutex_unlock(&app_lock);
	report_powermon_app(&summary);
	save_eard_conf(&eard_dyn_conf);
	/* RESTORE FREQUENCY */
	#ifndef EAR_CPUPOWER
	verbose(VJOBPMON, "restoring governor %s", current_ear_app[ccontext]->governor.governor);
	#else
	verbose(VJOBPMON, "restoring governor %s", current_ear_app[ccontext]->governor.name);
	#endif
	set_governor(&current_ear_app[ccontext]->governor);
	#ifndef EAR_CPUPOWER
	if (strcmp(current_ear_app[ccontext]->governor.governor, "userspace") == 0) {
	#else
	if (strcmp(current_ear_app[ccontext]->governor.name, "userspace") == 0) {
	#endif
		frequency_set_all_cpus(current_ear_app[ccontext]->current_freq);
	}
	current_node_freq = frequency_get_cpu_freq(0);
	clean_job_environment(jid, sid);
	reset_shared_memory();
	end_context(ccontext);
	/* At this point we have to set a new one as default */
	/* This context is removed, now we have to check environment is ok and select a new context */
	/* If previousy selected is not the one that has finished, we set again the previous one */
	if (ccontext != pcontext) {
		verbose(VJOBPMON, "Setting ccontext to previous one %d", pcontext);
		ccontext = pcontext;
	} else {
		/* If the ccontext has finished we must select a new one */
		/* Last job was an sbatch */
		if (sid == NULL_SID) {
			clean_job_contexts(jid);
		}
		ccontext = select_last_context();
	}
}

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
	save_eard_conf(&eard_dyn_conf);
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
	save_eard_conf(&eard_dyn_conf);
}

/* Sets the maximum frequency in the node */
void powermon_new_max_freq(ulong maxf) {
	uint ps;
	/* Job running and not EAR-aware */
	if ((ccontext >= 0) && (current_ear_app[ccontext]->app.job.id > 0) &&
		(current_ear_app[ccontext]->app.is_mpi == 0)) {
		if (maxf < current_node_freq) {
			verbose(VJOBPMON, "MaxFreq: Application is not mpi, automatically changing freq from %lu to %lu",
					current_node_freq, maxf);
			frequency_set_all_cpus(maxf);
			current_node_freq = maxf;
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
	ps = frequency_closest_pstate(def);
	if ((ccontext >= 0) && (current_ear_app[ccontext]->app.is_mpi == 0)) {
		cpolicy = policy_name_to_id(current_ear_app[ccontext]->app.job.policy, &my_cluster_conf);
		if (cpolicy == p_id) { /* If the process runs at selected policy */
			if (def < current_node_freq) {
				verbose(VJOBPMON, "DefFreq: Application is not mpi, automatically changing freq from %lu to %lu",
						current_node_freq, def);
				frequency_set_all_cpus(def);
				current_node_freq = def;
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

/** Reduces the current freq (default and max) based on new values. If application is not mpi, automatically chages the node freq if needed */

void powermon_red_freq(ulong max_freq, ulong def_freq) {
	int nump;
	uint ps_def, ps_max;
	ps_def = frequency_closest_pstate(def_freq);
	ps_max = frequency_closest_pstate(max_freq);
	if ((ccontext >= 0) && (current_ear_app[ccontext]->app.is_mpi == 0)) {
		if (def_freq < current_node_freq) {
			verbose(VJOBPMON, "RedFreq: Application is not mpi, automatically changing freq from %lu to %lu",
					current_node_freq, def_freq);
			frequency_set_all_cpus(def_freq);
			current_node_freq = def_freq;
		}
	}
	my_node_conf->max_pstate = ps_max;
	for (nump = 0; nump < my_node_conf->num_policies; nump++) {
		verbose(VJOBPMON + 1, "New default pstate %u for policy %u freq=%lu", ps_def,
				my_node_conf->policies[nump].policy, def_freq);
		my_node_conf->policies[nump].p_state = ps_def;
	}
	save_eard_conf(&eard_dyn_conf);
}

/** Sets temporally the default and max frequency to the same value. When application is not mpi, automatically chages the node freq if needed */

void powermon_set_freq(ulong freq) {
	int nump;
	uint ps;
	ps = frequency_closest_pstate(freq);
	if (ccontext >= 0) {
		if (freq != current_node_freq) {
			if ((my_cluster_conf.eard.force_frequencies) || (current_ear_app[ccontext]->app.is_mpi)) {
				verbose(VJOBPMON, "SetFreq:  changing freq from %lu to %lu", current_node_freq, freq);
				frequency_set_all_cpus(freq);
				current_node_freq = freq;
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


// Each sample is processed by this function
void update_historic_info(power_data_t *my_current_power, nm_data_t *nm) {
	ulong jid, mpi,sid;
	uint usedb,useeardbd;
	double maxpower, minpower, RAPL, corrected_power;
	usedb=my_cluster_conf.eard.use_mysql;
	useeardbd=my_cluster_conf.eard.use_eardbd;
	if (ccontext >= 0) {
		jid = current_ear_app[ccontext]->app.job.id;
		sid = current_ear_app[ccontext]->app.job.step_id; 
		mpi = current_ear_app[ccontext]->app.is_mpi;
		maxpower = current_ear_app[ccontext]->app.power_sig.max_DC_power;
		minpower = current_ear_app[ccontext]->app.power_sig.min_DC_power;
	} else {
		jid = 0;
		sid = 0;
		mpi = 0;
		maxpower = minpower = 0;
	}
	verbosen(VNODEPMON, "ID %lu MPI=%lu  Current power %.1lf max %.1lf min %.1lf ",
			jid, mpi, my_current_power->avg_dc, maxpower, minpower);
	verbose_node_metrics(&my_nm_id, nm);


	while (pthread_mutex_trylock(&app_lock));

	if ((ccontext >= 0) && (current_ear_app[ccontext]->app.job.id > 0)) {
		if ((my_current_power->avg_dc > maxpower) && (my_current_power->avg_dc<my_node_conf->max_error_power)){
			current_ear_app[ccontext]->app.power_sig.max_DC_power = my_current_power->avg_dc;
		}
		if (my_current_power->avg_dc < minpower){
			current_ear_app[ccontext]->app.power_sig.min_DC_power = my_current_power->avg_dc;
		}
	}

	pthread_mutex_unlock(&app_lock);

	report_periodic_power(fd_periodic, my_current_power);

	current_sample.start_time = my_current_power->begin;
	current_sample.end_time = my_current_power->end;
	corrected_power = my_current_power->avg_dc;
	RAPL =accum_dram_power(my_current_power)+accum_cpu_power(my_current_power); 
	if (my_current_power->avg_dc < RAPL) {
		corrected_power = RAPL;
	}
    ulong time_consumed = (ulong) difftime(my_current_power->end, my_current_power->begin);
#if USE_GPUS
    current_sample.DRAM_energy=accum_dram_power(my_current_power)*time_consumed;
    current_sample.PCK_energy=accum_cpu_power(my_current_power)*time_consumed;
    current_sample.GPU_energy=accum_gpu_power(my_current_power)*time_consumed;
#endif


	current_sample.DC_energy = corrected_power * (ulong) difftime(my_current_power->end, my_current_power->begin);

	/* To be usd by status */
	last_power_reported = my_current_power->avg_dc;
	copy_node_metrics(&my_nm_id, &last_nm, nm);

	current_sample.avg_f = (ulong) get_nm_cpufreq(&my_nm_id, nm);;
	current_sample.temp = (ulong) get_nm_temp(&my_nm_id, nm);

#if SYSLOG_MSG
	if ((my_current_power->avg_dc==0) || (my_current_power->avg_dc< my_node_conf->min_sig_power) || (my_current_power->avg_dc>my_node_conf->max_sig_power)){
		syslog(LOG_DAEMON|LOG_ERR,"Node %s reports %.2lf as avg power in last period\n",nodename,my_current_power->avg_dc);
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,DC_POWER_ERROR,my_current_power->avg_dc);
	}
	if (current_sample.temp>my_node_conf->max_temp){
		syslog(LOG_DAEMON|LOG_ERR,"Node %s reports %lu degress (max %lu)\n",nodename,current_sample.temp,my_node_conf->max_temp);
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,TEMP_ERROR,current_sample.temp);
	}
	if (RAPL == 0) {
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,RAPL_ERROR,RAPL);
	}
	if ((current_sample.avg_f==0) || ((current_sample.avg_f>frequency_get_nominal_freq()) && (mpi))){
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,FREQ_ERROR,current_sample.avg_f);
	}
#endif
	if ((my_current_power->avg_dc == 0) || (my_current_power->avg_dc > my_node_conf->max_error_power)) {
		warning("Resetting IPMI interface since power is %.2lf", my_current_power->avg_dc);

		if (energy_dispose(&my_eh_pm) != EAR_SUCCESS) {
			error("when resetting IPMI interface 'energy_dispose'");
		}

		if (energy_init(NULL, &my_eh_pm) != EAR_SUCCESS) {
			error("when resetting IPMI interface in 'energy_init'");
		}
	}

#if USE_DB
	if ((my_current_power->avg_dc>=0) && (my_current_power->avg_dc<my_node_conf->max_error_power)){
	if (my_cluster_conf.eard.use_mysql)
	{
		if (!my_cluster_conf.eard.use_eardbd) {
			/* current sample reports the value of job_id and step_id active at this moment */
			/* If we want to be strict, we must report intermediate samples at job start and job end */
			if (!db_insert_periodic_metric(&current_sample))
				debug( "Periodic power monitoring sample correctly written");
		} else {
			edb_state_t state = eardbd_send_periodic_metric(&current_sample);

			if (edb_state_fail(state)) {
				if (state_fail(state.server)){
					error("Error when sending periodic power metric to eardb main server");
				}else{
					error("Error when sending periodic power metric to eardb mirror server");
				}
				state = eardbd_reconnect(&my_cluster_conf, my_node_conf, state);

				if (edb_state_fail(state)) {
					error("Error re-connecting with EARDB errnum:%d errmsg:%s",
						intern_error_num, intern_error_str);
				}
			}
		}
	}
	}/*if ((my_current_power->avg_dc==0)...*/
#endif

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
	uint usedb,useeardbd;
	job_id jid,sid;
	jid=app->job.id;
	sid=app->job.step_id;
	usedb=my_cluster_conf.eard.use_mysql;
	useeardbd=my_cluster_conf.eard.use_eardbd;
	if (app->signature.GBS==0)
	{
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,GBS_ERROR,0);
	}
	if (app->signature.CPI==0){
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,CPI_ERROR,0);
	}
	if (app->signature.DC_power==0){
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,DC_POWER_ERROR,0);
	}
	if (app->signature.avg_f==0){
		log_report_eard_rt_error(usedb,useeardbd,jid,sid,FREQ_ERROR,0);
	}
}

void powermon_mpi_signature(application_t *app) {
	check_context("powermon_mpi_signature and not current context");
	if (app == NULL) {
		error("powermon_mpi_signature: and NULL app provided");
		return;
	}
	check_rt_error_for_signature(app);
	signature_copy(&current_ear_app[ccontext]->app.signature, &app->signature);
	current_ear_app[ccontext]->app.job.def_f = app->job.def_f;
	current_ear_app[ccontext]->app.job.th = app->job.th;
	current_ear_app[ccontext]->app.job.procs = app->job.procs;
	strcpy(current_ear_app[ccontext]->app.job.policy, app->job.policy);
	strcpy(current_ear_app[ccontext]->app.job.app_id, app->job.app_id);
	sig_reported = 1;
	save_eard_conf(&eard_dyn_conf);
}

void powermon_init_nm() {
	if (init_node_metrics(&my_nm_id, node_desc.sockets, node_desc.cores, get_model(), frequency_get_nominal_freq()) !=
		EAR_SUCCESS) {
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

/*
*	MAIN POWER MONITORING THREAD
*/


// frequency_monitoring will be expressed in usecs
void *eard_power_monitoring(void *noinfo) {
	energy_data_t e_begin;
	energy_data_t e_end;
	power_data_t my_current_power;

	PM_set_sigusr1();
	form_database_paths();

	init_contexts();

	num_packs=detect_packages(NULL);
	if (num_packs==0){
		error("Packages cannot be detected");
	}

	memset((void *) &default_app, 0, sizeof(powermon_app_t));

	verbose(VJOBPMON, "Power monitoring thread UP");
	if (pthread_setname_np(pthread_self(), TH_NAME)) error("Setting name for %s thread %s", TH_NAME, strerror(errno));
	if (init_power_ponitoring(&my_eh_pm) != EAR_SUCCESS) {
		error("Error in init_power_ponitoring");
	}
	debug("Allocating memory for energy data");	
	alloc_energy_data(&c_energy);
	alloc_energy_data(&e_begin);
	alloc_energy_data(&e_end);
	alloc_power_data(&my_current_power);

	alloc_energy_data(&default_app.energy_init);

	// current_sample is the current powermonitoring period
	debug("init_periodic_metric");
	init_periodic_metric(&current_sample);
	create_powermon_out();

	// We will collect and report avg power until eard finishes
	// Get time and Energy

	debug("read_enegy_data");
	read_enegy_data(&my_eh_pm, &e_begin);
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

	/*
	*	MAIN LOOP
	*/
	debug("Starting power monitoring loop, reporting metrics every %d seconds",f_monitoring);

	while (!eard_must_exit) {
		debug("------------------- NEW PERIOD -------------------");
		// Wait for N usecs
		sleep(f_monitoring);

		// Get time and Energy
		debug("Reading energy");
		read_enegy_data(&my_eh_pm, &e_end);

		if (e_end.DC_node_energy == 0) {
			warning("Power monitor period invalid energy reading, continue");
		} else {

			// Get node metrics
			debug("Reading node metrics");
			end_compute_node_metrics(&my_nm_id, &nm_end);
			diff_node_metrics(&my_nm_id, &nm_init, &nm_end, &nm_diff);
			start_compute_node_metrics(&my_nm_id, &nm_init);


			// Compute the power
			compute_power(&e_begin, &e_end, &my_current_power);

			print_power(&my_current_power);

			// Save current power
			update_historic_info(&my_current_power, &nm_diff);

			// Set values for next iteration
			copy_energy_data(&e_begin, &e_end);
		}

	}
	debug("Power monitor thread EXITs");
	if (dispose_node_metrics(&my_nm_id) != EAR_SUCCESS) {
		error("dispose_node_metrics ");
	}


	pthread_exit(0);
	//exit(0);
}

void powermon_get_status(status_t *my_status) {
	int i;
	int max_p; // More than TOTAL_POLICIES is not yet supported
	/* Policies */
	if (my_node_conf->num_policies>TOTAL_POLICIES) max_p=TOTAL_POLICIES;
	else max_p=my_node_conf->num_policies;
	my_status->num_policies=max_p;
	for (i=0;i<max_p;i++){
		my_status->policy_conf[i].freq=frequency_pstate_to_freq(my_node_conf->policies[i].p_state);
		my_status->policy_conf[i].th=(uint)(my_node_conf->policies[i].settings[0]*100.0);
	}
	/* Current app info */
	if (ccontext >= 0) {
		my_status->app.job_id = current_ear_app[ccontext]->app.job.id;
		my_status->app.step_id = current_ear_app[ccontext]->app.job.step_id;
	} else {
		/* No job running */
		my_status->app.job_id = 0;
	}
	/* Node info */
	my_status->node.avg_freq = (ulong)(last_nm.avg_cpu_freq);
	my_status->node.temp = (ulong) get_nm_temp(&my_nm_id, &last_nm);
	my_status->node.power = (ulong) last_power_reported;
	my_status->node.max_freq = (ulong) frequency_pstate_to_freq(my_node_conf->max_pstate);
}


void print_powermon_app(powermon_app_t *app) {
	print_application(&app->app);
	print_energy_data(&app->energy_init);
}

