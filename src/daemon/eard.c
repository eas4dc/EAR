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

#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/resource.h>
#include <linux/limits.h>
#include <strings.h>
#include <string.h>
#include <math.h>

//#define SHOW_DEBUGS 1
#include <common/includes.h>
#include <common/environment.h>
#include <common/types/pc_app_info.h>
#include <common/hardware/hardware_info.h>
#include <common/system/monitor.h>
#include <common/system/execute.h>

#include <report/report.h>

#include <management/cpufreq/frequency.h>
#include <management/imcfreq/imcfreq.h>
#include <management/cpupow/cpupow.h>
#include <metrics/common/msr.h>
#include <metrics/common/hsmp.h>
#include <metrics/gpu/gpu.h>
#include <metrics/energy/cpu.h>
#include <metrics/energy/energy_node.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>

#include <daemon/eard.h>
#include <daemon/eard_node_services.h>
#include <daemon/local_api/eard_api_conf.h>
#include <daemon/remote_api/dynamic_configuration.h>
#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/eard_checkpoint.h>
#include <daemon/log_eard.h>

#include <daemon/app_api/app_server_api.h>
#if USE_GPUS
#include <daemon/gpu/gpu_mgt.h>
#endif

#define MIN_INTERVAL_RT_ERROR 3600

#if APP_API_THREAD
pthread_t app_eard_api_th;
#endif

#if POWERMON_THREAD
pthread_t power_mon_th; // It is pending to see whether it works with threads
#endif

#if EXTERNAL_COMMANDS_THREAD
pthread_t dyn_conf_th;
#endif

pthread_barrier_t setup_barrier; // This barrier is for set-up synchronization
                                 // between power monitor, dynamic configuration and
                                 // "main" threads.

int num_nodes_cluster=0;
cluster_conf_t my_cluster_conf;
char *ser_cluster_conf;
char * my_ser_cluster_conf;
size_t ser_cluster_conf_size;
char ser_cluster_conf_path[GENERIC_NAME];
my_node_conf_t *my_node_conf;
my_node_conf_t my_original_node_conf;
eard_dyn_conf_t eard_dyn_conf; // This variable is for eard checkpoint
policy_conf_t default_policy_context, energy_tag_context, authorized_context;
/* Shared memory regions */
services_conf_t *my_services_conf;
ulong *shared_frequencies;
ulong *frequencies;
/* END Shared memory regions */

#if USE_GPUS
uint eard_num_gpus;
uint eard_gpu_model;
char gpu_str[256];
#endif

coefficient_t *my_coefficients;
coefficient_t *my_coefficients_default;
coefficient_t *coeffs_conf;
coefficient_t *coeffs_default_conf;

/** These paths are used for shared memory regions */
char my_ear_conf_path[GENERIC_NAME];
char coeffs_path[GENERIC_NAME];
char coeffs_default_path[GENERIC_NAME];
char services_conf_path[GENERIC_NAME];
char frequencies_path[GENERIC_NAME];
int coeffs_size;
int coeffs_default_size;
uint signal_sighup = 0;
uint f_monitoring;

loop_t current_loop_data;

#define RAPL_METRICS    4

/* New report data */
uint report_eard = 1;
report_id_t rid;
state_t report_connection_status = EAR_SUCCESS;

static int fd_my_log = 2;

unsigned long eard_max_freq;
int eard_max_pstate;

char ear_tmp[MAX_PATH_SIZE];
char nodename[MAX_PATH_SIZE];
int eard_lockf;
char eard_lock_file[MAX_PATH_SIZE * 2];
int application_id = -1;

ehandler_t eard_handler_energy,handler_energy;
ulong current_node_freq;
int eard_must_exit = 0;
topology_t node_desc;
int fd_rapl[MAX_PACKAGES];

#define EARD_RESTORE 1

/*************** To recover hardware settings **************/

void set_default_management_init()
{
  #if EARD_RESTORE
  /* CPU freq */
  mgt_cpufreq_reset(no_ctx);
  /* IMC freq */
  if (node_desc.vendor == VENDOR_INTEL){
    mgt_imcfreq_set_auto(no_ctx);
  }else if (node_desc.vendor == VENDOR_AMD){
    mgt_imcfreq_set_current(no_ctx, 0, all_sockets);
  }
  #endif
}

static void set_default_management_exit()
{
  #if EARD_RESTORE
  /* CPU freq */
  mgt_cpufreq_reset(no_ctx);
  /* IMC freq */
  if (node_desc.vendor == VENDOR_INTEL){
    mgt_imcfreq_set_auto(no_ctx);
  }else if (node_desc.vendor == VENDOR_AMD){
    mgt_imcfreq_set_current(no_ctx, 0, all_sockets);
  }
  #endif

}
/**********************************************************/



/* EARD init errors control */
static uint error_rapl=0;
static uint error_energy=0;
static uint error_connector=0;

/* This function checks a policy configuration is valid */
void check_policy(policy_conf_t *p)
{
	unsigned long f;
	double df;
    /* We are using pstates */
    if (p->def_freq==(float)0){
        if (p->p_state>=frequency_get_num_pstates()) p->p_state=frequency_get_nominal_pstate();
    }
    else
    {
        /* We are using frequencies */
				df = round(p->def_freq * 100.0);
				f = (unsigned long)df*10000;
				if (!frequency_is_valid_frequency(f))
				{
								error("Default frequency %lu for policy %s is not valid",f,p->name);
								p->def_freq=(float)frequency_closest_frequency(f)/(float)1000000;
								error("New def_freq %f",p->def_freq);
				}
		}
}

void check_policy_values(policy_conf_t *p,int nump)
{
				int i=0;
				for (i=0;i<nump;i++){
								check_policy(&p[i]);
				}
}

/* This function checks pstates and frequencies specified for each policy are valid in the current node */

void compute_policy_def_freq(policy_conf_t *p)
{
				if (p->def_freq==(float)0){
								float tmp;
								tmp = truncf((float)frequency_pstate_to_freq(p->p_state)/1000.0);
								tmp = tmp/1000.0;
								verbose(VEARD+1, "Def freq is %f",tmp);
								p->def_freq= tmp;
				}else{
								p->p_state=frequency_closest_pstate((unsigned long)(p->def_freq*1000000));
				}
}

void compute_default_pstates_per_policy(uint num_policies, policy_conf_t *plist)
{
				uint i;
				for (i=0;i<num_policies;i++){
								check_policy(&plist[i]);
								compute_policy_def_freq(&plist[i]);
				}
}

/* Copies the list of available frequencies in the shared memory */
void init_frequency_list()
{
				int ps, i;
				ps = frequency_get_num_pstates();
				int size = ps * sizeof(ulong);
				frequencies = (ulong *) malloc(size);
				memcpy(frequencies, frequency_get_freq_rank_list(), size);
				get_frequencies_path(my_cluster_conf.install.dir_temp, frequencies_path);
				shared_frequencies = create_frequencies_shared_area(frequencies_path, frequencies, size);
				for (i = 0; i < ps; i++) {
								verbose(VEARD_INIT, "shared_frequencies[%d]=%lu", i, shared_frequencies[i]);
				}
}


void set_verbose_variables() {
	verb_level = my_cluster_conf.eard.verbose;
	TIMESTAMP_SET_EN(my_cluster_conf.eard.use_log);
}

void set_global_eard_variables() {
	strcpy(ear_tmp, my_cluster_conf.install.dir_temp);
}

// Lock unlock functions are used to be sure a single daemon is running per node

void create_tmp(char *tmp_dir) {
				int ret;
				ret = mkdir(tmp_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
				if ((ret < 0) && (errno != EEXIST)) {
								error("ear tmp dir cannot be created (%s)", strerror(errno));
								_exit(0);
				}

				if (chmod(tmp_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) < 0) {
								warning("ear_tmp permissions cannot be set (%s)", strerror(errno));
								_exit(0);
				}
}



/*
 *	Restarts again EARD
 */

void eard_restart()
{
    char *ear_install;
    char my_bin[MAX_PATH_SIZE];
    ear_install = ear_getenv(ENV_PATH_EAR);
    if (ear_install == NULL) {
        error("%s is NULL\n", ENV_PATH_EAR);
        sprintf(my_bin, "eard");
    } else sprintf(my_bin, "%s/sbin/eard", ear_install);
    verbose(VCONF, "LD_LIBRARY_PATH=%s\n", ear_getenv("LD_LIBRARY_PATH"));
    verbose(VCONF, "Restarting to %s p_state=1 ear_tmp=%s verbose=0\n", my_bin, ear_tmp);
    end_service("eard");
    // Do we want to maintain verbose level?
    execlp(my_bin, my_bin, "1", NULL);
    verbose(VCONF, "Restarting EARD %s\n", strerror(errno));
    _exit(1);
}

/*
 *	Depending on restart argument, exits eard or restart it
 */
void eard_exit(uint restart)
{
				verbose(VCONF, "Exiting");
#if EARD_LOCK
				eard_unlock();
#endif
				// Come disposes
				//print_rapl_metrics();
				// Recovering old frequency and governor configurations.
				verbose(VCONF, "frequency_dispose");

        /* Sets to default CPU freq and IMC freq */
        set_default_management_exit();

				frequency_dispose();
				powercap_end();
				verbose(VCONF, "Releasing node resources");
				// More disposes
				if (state_fail(energy_dispose(&eard_handler_energy))) {
								error("Error disposing energy for eard");
				}
				report_dispose(&rid);
				verbose(VCONF, "Releasing files");
				clean_global_connector();
				// RAM Bandwidth
                services_dispose();

                /* Releasing shared memory */
				coeffs_shared_area_dispose(coeffs_path);
				coeffs_default_shared_area_dispose(coeffs_default_path);
				services_conf_shared_area_dispose(services_conf_path);
				/* end releasing shared memory */
				if (restart) {
								verbose(VCONF, "Restarting EARD\n");
				} else {
								verbose(VCONF, "Exiting EARD\n");
				}
				if (restart == 0) {
								end_service("eard");
								_exit(0);
				} else if (restart == 2) {
								end_service("eard");
								_exit(1);
				} else {
								eard_restart();
				}
}


void reopen_log()
{
	log_close(fd_my_log);
	create_log(my_cluster_conf.install.dir_temp, "eard", fd_my_log);
}

//
// MAIN eard: eard default_freq [path for communication files]
//
// eard creates different pipes to receive requests and to send values requested
void Usage(char *app) {
				verbose(VEARD_INIT, "Usage: %s [-h|verbose_level]", app);
				verbose(VEARD_INIT, "\tear.conf file is used to define node settings. It must be available at");
				verbose(VEARD_INIT, "\t $EAR_ETC/ear/ear.conf");
				_exit(1);
}

//region SIGNALS
void signal_handler(int sig) 
{
				if (sig == SIGPIPE) { verbose(VCONF, "signal SIGPIPE received. Application terminated abnormally"); }
				if (sig == SIGTERM) { verbose(VCONF, "signal SIGTERM received. Finishing"); }
				if (sig == SIGINT) { verbose(VCONF, "signal SIGINT received. Finishing"); }
        if (sig == SIGSEGV){
          verbose(VCONF,"signal SIGSEGV received in main EARD thread. Stack.....");
          print_stack(verb_channel);
        }
				if (sig == SIGHUP) {
								verbose(VCONF, "signal SIGHUP received. Reloading EAR conf");
								signal_sighup = 1;
				}
				if (sig == SIGUSR2){
								reopen_log();
								return;
				}

				// The PIPE was closed, so the daemon connection ends
				if (sig == SIGPIPE) {
								eard_close_comm(CLOSE_PIPE,CLOSE_PIPE);
				}


				// Someone wants EARD to get closed or restarted
				if ((sig == SIGTERM) || (sig == SIGINT)) {
								eard_must_exit = 1;
				}
				if ((sig == SIGTERM) || (sig == SIGINT)) {
								eard_exit(0);
				}
				if (sig == SIGHUP) {
								free_cluster_conf(&my_cluster_conf);
								// Reading the configuration
								if (read_cluster_conf(my_ear_conf_path, &my_cluster_conf) != EAR_SUCCESS) {
												error(" Error reading cluster configuration\n");
								} else {
												verbose(VCONF, "Loading EAR configuration");
												verbose(VCONF, "There are %d Nodes in the cluster",my_cluster_conf.num_nodes);
												compute_default_pstates_per_policy(my_cluster_conf.num_policies, my_cluster_conf.power_policies);
												print_cluster_conf(&my_cluster_conf);
												free(my_node_conf);
												my_node_conf = get_my_node_conf(&my_cluster_conf, nodename);
												if (my_node_conf == NULL) {
																error(" Error in cluster configuration, node %s not found\n", nodename);
																exit(0);
												} else {
																check_policy_values(my_node_conf->policies,my_node_conf->num_policies);
																eard_dyn_conf.nconf = my_node_conf;
																print_my_node_conf(my_node_conf);
																copy_my_node_conf(&my_original_node_conf, my_node_conf);
																set_global_eard_variables();
																configure_new_values(&my_cluster_conf,my_node_conf);
																powermon_new_configuration();
																save_eard_conf(&eard_dyn_conf);
												}

								}
								verbose(VCONF, "Configuration reloaded");
								report_dispose(&rid);
								report_connection_status = report_init(&rid, &my_cluster_conf);
				}

}

void signal_catcher() 
{
				struct sigaction action;
				int signal;
				sigset_t eard_mask;

				sigfillset(&eard_mask);
				sigdelset(&eard_mask, SIGPIPE);
				sigdelset(&eard_mask, SIGTERM);
				sigdelset(&eard_mask, SIGINT);
				sigdelset(&eard_mask, SIGHUP);
				sigdelset(&eard_mask, SIGUSR2);
				sigdelset(&eard_mask, SIGSEGV);
				//sigprocmask(SIG_SETMASK,&eard_mask,NULL);
				pthread_sigmask(SIG_SETMASK, &eard_mask, NULL);


				sigemptyset(&action.sa_mask);
				action.sa_handler = signal_handler;
				action.sa_flags = 0;

				signal = SIGPIPE;
				if (sigaction(signal, &action, NULL) < 0) {
								error("sigaction error on signal s=%d (%s)", signal, strerror(errno));
				}

				signal = SIGTERM;
				if (sigaction(signal, &action, NULL) < 0) {
								error("sigaction error on signal s=%d (%s)", signal, strerror(errno));
				}

				signal = SIGINT;
				if (sigaction(signal, &action, NULL) < 0) {
								error("sigaction error on signal s=%d (%s)", signal, strerror(errno));
				}

				signal = SIGHUP;
				if (sigaction(signal, &action, NULL) < 0) {
								error("sigaction error on signal s=%d (%s)", signal, strerror(errno));
				}
				signal = SIGUSR2;
				if (sigaction(signal, &action, NULL) < 0) {
								error("sigaction error on signal s=%d (%s)", signal, strerror(errno));
				}
				signal = SIGSEGV;
				if (sigaction(signal, &action, NULL) < 0) {
								error("sigaction error on signal s=%d (%s)", signal, strerror(errno));
				}

}
//endregion

/* Shared memory regions are privatized, this is default context */

void configure_new_values( cluster_conf_t *cluster, my_node_conf_t *node) {
				policy_conf_t *my_policy;
				eard_max_pstate = node->max_pstate;
				// Default policy is just in case
				default_policy_context.policy  = MONITORING_ONLY;
				default_policy_context.p_state = EAR_MIN_P_STATE;
				default_policy_context.settings[0] = 0;
				my_policy = get_my_policy_conf(node,cluster->default_policy);
				if (my_policy==NULL){
								// This should not happen
								error("Default policy  not found in ear.conf");
								my_policy = &default_policy_context;
				}else{
								default_policy_context.policy = my_policy->policy;
								default_policy_context.p_state = my_policy->p_state;
								default_policy_context.settings[0] = my_policy->settings[0];
				}
				/* deff is not used, warning */
				#if 0
				ulong deff;
				deff = frequency_pstate_to_freq(my_policy->p_state);
				#endif
				f_monitoring = my_cluster_conf.eard.period_powermon;
				copy_eard_conf(&my_services_conf->eard,&my_cluster_conf.eard);
				copy_eargmd_conf(&my_services_conf->eargmd,&my_cluster_conf.eargm);
				copy_eardb_conf(&my_services_conf->db,&my_cluster_conf.database);
				copy_eardbd_conf(&my_services_conf->eardbd,&my_cluster_conf.db_manager);
				strcpy(my_services_conf->net_ext,my_cluster_conf.net_ext);
				save_eard_conf(&eard_dyn_conf);
}

void configure_default_values( cluster_conf_t *cluster, my_node_conf_t *node) {
				policy_conf_t *my_policy;
				eard_max_pstate = node->max_pstate;
				verbose(VEARD+1,"Max pstate for this node is %d",eard_max_pstate);
				// Default policy is just in case
				default_policy_context.policy  = MONITORING_ONLY;
				default_policy_context.p_state = EAR_MIN_P_STATE;
				default_policy_context.settings[0] = 0;
				my_policy = get_my_policy_conf(node,cluster->default_policy);
				if (my_policy == NULL){
								// This should not happen
								error("Default policy  not found in ear.conf");
								my_policy = &default_policy_context;
				}else{
								default_policy_context.policy = my_policy->policy;
								default_policy_context.p_state = my_policy->p_state;
								default_policy_context.settings[0] = my_policy->settings[0];
				}
				verbose(VEARD+1,"configure_default_values. Looking for pstate %d\n",my_policy->p_state);
				//deff = frequency_pstate_to_freq(my_policy->p_state);
				f_monitoring = my_cluster_conf.eard.period_powermon;
				copy_eard_conf(&my_services_conf->eard,&my_cluster_conf.eard);
				copy_eargmd_conf(&my_services_conf->eargmd,&my_cluster_conf.eargm);
				copy_eardb_conf(&my_services_conf->db,&my_cluster_conf.database);
				copy_eardbd_conf(&my_services_conf->eardbd,&my_cluster_conf.db_manager);
				strcpy(my_services_conf->net_ext,my_cluster_conf.net_ext);
				save_eard_conf(&eard_dyn_conf);
}

/** Coefficients management : Basic functions to check if coeffs exist */
int coeffs_per_node_exist(char *filename) {
				int file_size = 0;
				sprintf(filename, "%s/island%d/coeffs.%s", my_cluster_conf.earlib.coefficients_pathname, my_node_conf->island,
												nodename);
				verbose(VCONF, "Looking for per-node %s coefficients file", filename);
				file_size = coeff_file_size(filename);
				return file_size;
}

int coeffs_per_node_default_exist(char *filename) {
				int file_size;
				if (my_node_conf->coef_file != NULL) {
								sprintf(filename, "%s/island%d/%s", my_cluster_conf.earlib.coefficients_pathname, my_node_conf->island,
																my_node_conf->coef_file);
								verbose(VCONF, "Looking for per-node special default %s coefficients file", filename);
								file_size = coeff_file_size(filename);
								return file_size;
				} else return EAR_OPEN_ERROR;
}

int coeffs_per_island_default_exist(char *filename) {
				int file_size;
				sprintf(filename, "%s/island%d/coeffs.default", my_cluster_conf.earlib.coefficients_pathname, my_node_conf->island);
				verbose(VCONF, "Looking for per-island default %s coefficients file", filename);
				file_size = coeff_file_size(filename);
				return file_size;
}

int read_coefficients_default()
{
				char my_coefficients_file[GENERIC_NAME];
				int file_size = 0;
				file_size = coeffs_per_node_default_exist(my_coefficients_file);
				if (file_size == EAR_OPEN_ERROR) {
								file_size = coeffs_per_island_default_exist(my_coefficients_file);
								if (file_size == EAR_OPEN_ERROR) {
												warning("Warning, coefficients not found");
												my_coefficients_default = (coefficient_t *) calloc(1, sizeof(coefficient_t));
												coeff_reset(my_coefficients_default);
												return sizeof(coefficient_t);
								}
				}
				int entries = file_size / sizeof(coefficient_t);
				verbose(VCONF, "%d default coefficients found", entries);
				my_coefficients_default = (coefficient_t *) calloc(entries, sizeof(coefficient_t));
				coeff_file_read_no_alloc(my_coefficients_file, my_coefficients_default, file_size);
				return file_size;
}

/* Read coefficients for current node */
int read_coefficients() {
				char my_coefficients_file[GENERIC_NAME];
				int file_size = 0;
				file_size = coeffs_per_node_exist(my_coefficients_file);
				if (file_size == EAR_OPEN_ERROR) {
								file_size = coeffs_per_node_default_exist(my_coefficients_file);
								if (file_size == EAR_OPEN_ERROR) {
												file_size = coeffs_per_island_default_exist(my_coefficients_file);
												if (file_size == EAR_OPEN_ERROR) {
																warning("Warning, coefficients not found");
																my_coefficients = (coefficient_t *) calloc(1, sizeof(coefficient_t));
																coeff_reset(my_coefficients);
																return sizeof(coefficient_t);
												}
								}
				}
				int entries = file_size / sizeof(coefficient_t);
				verbose(VCONF, "%d coefficients found", entries);
				my_coefficients = (coefficient_t *) calloc(entries, sizeof(coefficient_t));
				coeff_file_read_no_alloc(my_coefficients_file, my_coefficients, file_size);
				return file_size;
}


void report_eard_init_error()
{
    if (error_rapl) {
        log_report_eard_init_error(&rid,RAPL_INIT_ERROR,0);
    }
    if (error_energy) {
        log_report_eard_init_error(&rid,ENERGY_INIT_ERROR,0);
    }
    if (error_connector){
        log_report_eard_init_error(&rid,CONNECTOR_INIT_ERROR,0);
    }
}

void     update_global_configuration_with_local_ssettings(cluster_conf_t *my_cluster_conf,my_node_conf_t *my_node_conf)
{
				strcpy(my_cluster_conf->install.obj_ener,my_node_conf->energy_plugin);
				strcpy(my_cluster_conf->install.obj_power_model,my_node_conf->energy_model);
}
int get_exec_name(char *name,int name_size)
{
				char path[1024]={0};
				int ret = readlink("/proc/self/exe",path,sizeof(path)-1);
				if(ret == -1)
				{
								printf("---- get exec name fail!!\n");
								return -1;
				}
				path[ret]= '\0';

				char *ptr = strrchr(path,'/');
				memset(name,0, name_size); //Clear the buffer area
				strncpy(name,ptr+1,name_size-1);
				return 0;
}


/*
 *
 *
 *	EARD MAIN
 *
 *
 */

int main(int argc, char *argv[]) 
{
				char my_name[1024];
				unsigned long ear_node_freq;
				int ret;

				// Usage
				if (argc > 2) {
								Usage(argv[0]);
				}



				verbose(VCONF, "Starting eard...................pid %d", getpid());
				create_log("/tmp", "eard_journal", fd_my_log);

				// We get nodename to create per_node files
				if (gethostname(nodename, sizeof(nodename)) < 0) {
								error("Error getting node name (%s)", strerror(errno));
								_exit(1);
				}
				strtok(nodename, ".");
				verbose(VEARD_INIT,"Executed %s in node name %s", argv[0],nodename);
				get_exec_name(my_name,sizeof(my_name));
				verbose(VEARD_INIT,"My name is %s",my_name);

				/** CONFIGURATION **/
				int node_size;
				state_t s;

				init_eard_rt_log();
				log_report_eard_min_interval(MIN_INTERVAL_RT_ERROR);
				verbose(VEARD_INIT,"Reading hardware topology");

                /** Change the open file limit */
                struct rlimit rl;
                getrlimit(RLIMIT_NOFILE, &rl);
                debug("current limit %lu max %lu", rl.rlim_cur, rl.rlim_max);
                rl.rlim_cur = rl.rlim_max;
                setrlimit(RLIMIT_NOFILE, &rl);
                
                /* We initialize topology */
                s = topology_init(&node_desc);
                node_size = node_desc.cpu_count;

                if (state_fail(s) || node_size <= 0 || node_desc.socket_count <= 0) {
                    error("topology information can't be initialized (%d)", s);
                    _exit(1);
                }
                topology_print(&node_desc, verb_channel);

                verbose(VEARD_INIT,"Topology detected: sockets %d, cores %d, threads %d",
                        node_desc.socket_count, node_desc.core_count, node_desc.cpu_count);

								#ifndef __ARCH_ARM
                if (msr_test(&node_desc, MSR_WR) != EAR_SUCCESS){
                    error("msr files are not available, please check the msr kernel module is loaded (modprobe msr)");
                    _exit(1);
                }
								#endif

                //
                verbose(VEARD_INIT,"Initializing frequency list");
                /* We initialize frecuency */
                if (frequency_init(0) < 0) {
                    error("frequency information can't be initialized");
                    verbose(VEARD_INIT, "frequency information can't be initialized");
                    _exit(1);
                }
                verbose(VEARD_INIT,"Reading ear.conf configuration");
                // We read the cluster configuration and sets default values in the shared memory
                if (get_ear_conf_path(my_ear_conf_path) == EAR_ERROR) {
                    error("Error opening ear.conf file, not available at regular paths ($EAR_ETC/ear/ear.conf)");
                    _exit(0);
                }
                if (read_cluster_conf(my_ear_conf_path, &my_cluster_conf) != EAR_SUCCESS) {
                    error(" Error reading cluster configuration\n");
                    _exit(1);
                } else {
                    verbose(VEARD_INIT,"%d Nodes detected in ear.conf configuration",my_cluster_conf.cluster_num_nodes);
                    compute_default_pstates_per_policy(my_cluster_conf.num_policies, my_cluster_conf.power_policies);
                    print_cluster_conf(&my_cluster_conf);
                    my_node_conf = get_my_node_conf(&my_cluster_conf, nodename);
                    if (my_node_conf == NULL) {
                        error( " Node %s not found in ear.conf, exiting\n", nodename);
                        _exit(0);
                    }
                    check_policy_values(my_node_conf->policies,my_node_conf->num_policies);
                    print_my_node_conf(my_node_conf);
                    copy_my_node_conf(&my_original_node_conf, my_node_conf);
                    update_global_configuration_with_local_ssettings(&my_cluster_conf,my_node_conf);
                }
                verbose(VEARD_INIT,"Serializing cluster_conf");
                serialize_cluster_conf(&my_cluster_conf, &ser_cluster_conf, &ser_cluster_conf_size);
                verbose(VEARD_INIT,"Serialized cluster conf requires %lu bytes", ser_cluster_conf_size);

                verbose(VEARD_INIT,"Initializing dynamic information and creating tmp");
                /* This info is used for eard checkpointing */
                eard_dyn_conf.cconf = &my_cluster_conf;
                eard_dyn_conf.nconf = my_node_conf;
                eard_dyn_conf.pm_app = get_powermon_app();
                set_global_eard_variables();
                verbose(VEARD_INIT,"Creating tmp dir");
                create_tmp(ear_tmp);
                if (my_cluster_conf.eard.use_log) {
                    verbose(VEARD_INIT,"Creating log file");
                    create_log(my_cluster_conf.install.dir_temp, "eard", fd_my_log);
                }
                set_verbose_variables();

                verbose(VEARD_INIT,"Creating frequency list in shared memory");
                /** Shared memory is used between EARD and EARL **/
                init_frequency_list();

                /**** SHARED MEMORY REGIONS ****/
                /* This area is for shared info */
                verbose(VEARD_INIT, "creating shared memory regions");
                /* Coefficients */
                verbose(VEARD_INIT,"Loading coefficients");
                coeffs_size = read_coefficients();
                verbose(VCONF, "Coefficients loaded");
                get_coeffs_path(my_cluster_conf.install.dir_temp, coeffs_path);
                verbose(VCONF, "Using %s as coeff path (shared memory region)", coeffs_path);
                coeffs_conf = create_coeffs_shared_area(coeffs_path, my_coefficients, coeffs_size);
                if (coeffs_conf == NULL) {
                    error("Error creating shared memory for coefficients\n");
                    _exit(0);
                }
                /* Default coefficients */
                verbose(VEARD_INIT,"Loading default coefficients");
                coeffs_default_size = read_coefficients_default();
                verbose(VCONF, "Coefficients by default loaded");
                if ((coeffs_size==0) && (coeffs_default_size==0)) verbose(VEARD_INIT,"No coefficients found, power policies will not be applied");
                get_coeffs_default_path(my_cluster_conf.install.dir_temp, coeffs_default_path);
                verbose(VCONF + 1, "Using %s as coeff by default path (shared memory region)", coeffs_default_path);
                coeffs_default_conf = create_coeffs_default_shared_area(coeffs_default_path, my_coefficients_default,
                        coeffs_default_size);
                if (coeffs_default_conf == NULL) {
                    error("Error creating shared memory for coefficients by default\n");
                    _exit(0);
                }

                /* This area incldues services details */
                get_services_conf_path(my_cluster_conf.install.dir_temp, services_conf_path);
                verbose(VCONF + 1, "Using %s as services_conf path (shared memory region)", services_conf_path);
                my_services_conf = create_services_conf_shared_area(services_conf_path);
                if (my_services_conf == NULL) {
                    error("Error creating shared memory\n");
                    _exit(0);
                }
                /* Serialized cluster conf */
                get_ser_cluster_conf_path(my_cluster_conf.install.dir_temp, ser_cluster_conf_path);
                verbose(VCONF + 1, "Using %s as serialized cluster conf path", ser_cluster_conf_path);
                my_ser_cluster_conf = create_ser_cluster_conf_shared_area(ser_cluster_conf_path, ser_cluster_conf, ser_cluster_conf_size);
                /**** END CREATION SHARED MEMORY REGIONS ****/
                verbose(VEARD_INIT,"Shared memory regions created");

                /** We must control if we come from a crash **/
                int must_recover = new_service("eard");
                if (must_recover) {
                    verbose(VCONF, "We must recover from a crash");
                    restore_eard_conf(&eard_dyn_conf);
                }
                /* After potential recoveries, we set the info in the shared memory */
                configure_default_values(&my_cluster_conf,my_node_conf);

                // Check
                if (argc == 2) {
                    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) Usage(argv[0]);
                    verb_level = atoi(argv[1]);


                    if ((verb_level < 0) || (verb_level > 4)) {
                        Usage(argv[0]);
                    }

                }
                set_ear_verbose(verb_level);
                VERB_SET_LV(verb_level);
                verbose(VEARD_INIT,"Catching signals");
                // We catch signals
                signal_catcher();

                state_assert(s, monitor_init(), _exit(0));

                if (eard_max_pstate < 0) Usage(argv[0]);
                if (eard_max_pstate >= frequency_get_num_pstates()) Usage(argv[0]);

                ear_node_freq = frequency_pstate_to_freq(eard_max_pstate);
                eard_max_freq = ear_node_freq;
                verbose(VCONF, "Default max frequency defined to %lu", eard_max_freq);
                // IMC frequency
                verbose(VCONF, "Initializing IMCfreq metrics API ");
                imcfreq_load(&node_desc, NO_EARD);
                state_assert(s, imcfreq_init(no_ctx), _exit(0));

                // Services
                services_init(&node_desc, my_node_conf);

#if EARD_LOCK
                eard_lock(ear_tmp);
#endif
                // At this point, only one daemon is running
                // PAST: we had here a frequency set
                verbose(VEARD_INIT,"Initializing RAPL counters");
								#ifndef __ARCH_ARM
                // We initialize rapl counters
                if (init_rapl_msr(fd_rapl)<0){ 
                    error("Error initializing rapl");
                    error_rapl=1;
                }
								#endif
                verbose(VEARD_INIT,"Initialzing energy plugin");
                // We initilize energy_node
                debug("Initializing energy in main EARD thread");

                if (state_fail(energy_init(&my_cluster_conf, &eard_handler_energy))) {
                    error("While initializing energy plugin: %s\n", state_msg);
                    error_energy=1;
                }
                if (state_fail(s = init_power_monitoring(&handler_energy, &node_desc))) {
                    error("While initializing power monitor: %s\n", state_msg);
                    _exit(0);	
                }

                verbose(VEARD_INIT,"Connecting with report plugin");
                /* This must be replaced by the report_eard option in eard */
                if (state_fail(report_load(my_cluster_conf.install.dir_plug,my_cluster_conf.eard.plugins))){
                    report_create_id(&rid, -1, -1, -1);
                }else{
                    report_create_id(&rid, 0, 0, 0);
                }
                report_init(&rid, &my_cluster_conf);

                report_eard_init_error();

                uint thread_setup_count = 1 + POWERMON_THREAD + EXTERNAL_COMMANDS_THREAD; // eard + power mon + dyn conf
                pthread_barrier_init(&setup_barrier, NULL, thread_setup_count);

                verbose(VEARD_INIT,"Creating EARD threads");
                verbose(VEARD_INIT,"Creating power  monitor thread");
#if POWERMON_THREAD

                if ((ret=pthread_create(&power_mon_th, NULL, eard_power_monitoring, NULL))){
                    errno=ret;
                    error("error creating power_monitoring thread %s\n",strerror(errno));
                    log_report_eard_init_error(&rid,PM_CREATION_ERROR,ret);
                }
#endif

                verbose(VEARD_INIT,"Creating the remote connections server");

#if EXTERNAL_COMMANDS_THREAD

                if ((ret=pthread_create(&dyn_conf_th, NULL, eard_dynamic_configuration, (void *)ear_tmp))){
                    error("error creating dynamic_configuration thread \n");
                    log_report_eard_init_error(&rid,DYN_CREATION_ERROR,ret);
                }
#endif

                verbose(VEARD_INIT,"Creating APP-API thread");

#if APP_API_THREAD

                if ((ret=pthread_create(&app_eard_api_th,NULL,eard_non_earl_api_service,NULL))){
                    error("error creating server thread for non-earl api\n");
                    log_report_eard_init_error(&rid,APP_API_CREATION_ERROR,ret);
                }
#endif


                verbose(VEARD_INIT,"EARD initialized succesfully");
                /* This function never ends, EARD exits with SIGTERM o SIGINT */
                eard_local_api();
                //eard is closed by SIGTERM signal, we should not reach this point
                verbose(VCONF, "END EARD\n");
                eard_close_comm(CLOSE_EARD,CLOSE_EARD);
                eard_exit(0);
                return 0;
}
