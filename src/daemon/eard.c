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

#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <linux/limits.h>
//#define SHOW_DEBUGS 1
#include <common/includes.h>
#include <common/environment.h>
#include <common/types/log_eard.h>
#include <common/hardware/frequency.h>
#include <metrics/frequency/cpu.h>
#include <metrics/energy/energy_cpu.h>
#include <metrics/energy/energy_node.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/gpu/gpu.h>
#include <common/hardware/hardware_info.h>
#include <daemon/eard_conf_api.h>
#include <daemon/power_monitor.h>
#if POWERCAP
#include <daemon/powercap.h>
#include <common/types/pc_app_info.h>
#endif
#include <daemon/eard_checkpoint.h>
#include <daemon/shared_configuration.h>
#include <daemon/dynamic_configuration.h>
#if USE_DB
#include <database_cache/eardbd_api.h>
#include <common/database/db_helper.h>
#endif
#include <daemon/eard.h>
#include <daemon/app_api/app_server_api.h>


#define MIN_INTERVAL_RT_ERROR 3600


#if APP_API_THREAD
pthread_t app_eard_api_th;
#endif

static ehandler_t eard_handler_energy,handler_energy;
static ctx_t eard_main_gpu_ctx;
static uint eard_gpu_initialized=0;
static ulong node_energy_datasize;
static edata_t node_energy_data;
unsigned int power_mon_freq = POWERMON_FREQ;
static int fd_rapl[MAX_PACKAGES];
static int num_packs=0;
static unsigned long long *values_rapl;
pthread_t power_mon_th; // It is pending to see whether it works with threads
pthread_t dyn_conf_th;
cluster_conf_t my_cluster_conf;
my_node_conf_t *my_node_conf;
my_node_conf_t my_original_node_conf;
eard_dyn_conf_t eard_dyn_conf; // This variable is for eard checkpoint
policy_conf_t default_policy_context, energy_tag_context, authorized_context;
/* Shared memory regions */
settings_conf_t *dyn_conf;
resched_t *resched_conf;
services_conf_t *my_services_conf;
ulong *shared_frequencies;
ulong *frequencies;
#ifdef POWERCAP
app_mgt_t *app_mgt_info;
pc_app_info_t *pc_app_info_data;
#endif
/* END Shared memory regions */

coefficient_t *my_coefficients;
coefficient_t *my_coefficients_default;
coefficient_t *coeffs_conf;
coefficient_t *coeffs_default_conf;
char my_ear_conf_path[GENERIC_NAME];
char my_eardbd_conf_path[GENERIC_NAME];
char eardbd_user[GENERIC_NAME];
char eardbd_pass[GENERIC_NAME];
char dyn_conf_path[GENERIC_NAME];
char resched_path[GENERIC_NAME];
char app_mgt_path[GENERIC_NAME];
char pc_app_info_path[GENERIC_NAME];
char coeffs_path[GENERIC_NAME];
char coeffs_default_path[GENERIC_NAME];
char services_conf_path[GENERIC_NAME];
char frequencies_path[GENERIC_NAME];
int coeffs_size;
int coeffs_default_size;
uint signal_sighup = 0;
uint f_monitoring;

loop_t current_loop_data;

#define max(a, b) (a>b?a:b)
#define min(a, b) (a<b?a:b)
#define RAPL_METRICS    4

static int eardbd_connected = 0;
static int db_helper_connected = 0;
static int fd_my_log = 2;

int ear_fd_req[ear_daemon_client_requests];
int ear_fd_ack[ear_daemon_client_requests];
unsigned long eard_max_freq;
int eard_max_pstate;
int num_uncore_counters;

char ear_ping[MAX_PATH_SIZE * 2];
char ear_tmp[MAX_PATH_SIZE];
char nodename[MAX_PATH_SIZE];
int ear_ping_fd = -1;
int eard_lockf;
char eard_lock_file[MAX_PATH_SIZE * 2];
int application_id = -1;

ulong energy_freq;
ulong current_node_freq;
struct daemon_req req;

int RAPL_counting = 0;
int eard_must_exit = 0;
static freq_cpu_t freq_global2;
static freq_cpu_t freq_global1;
static freq_cpu_t freq_local2;
static freq_cpu_t freq_local1;
freq_cpu_t freq_job2;
freq_cpu_t freq_job1;
topology_t node_desc;

/* EARD init errors control */
static uint error_uncore=0;
static uint error_rapl=0;
static uint error_energy=0;
static uint error_connector=0;



void compute_default_pstates_per_policy(uint num_policies, policy_conf_t *plist)
{
	uint i;
	for (i=0;i<num_policies;i++){
		check_policy(&plist[i]);
		compute_policy_def_freq(&plist[i]);
	}
}

void init_frequency_list() {
	int ps, i;
	ps = frequency_get_num_pstates();
	int size = ps * sizeof(ulong);
	frequencies = (ulong *) malloc(size);
	memcpy(frequencies, frequency_get_freq_rank_list(), size);
	get_frequencies_path(my_cluster_conf.install.dir_temp, frequencies_path);
	shared_frequencies = create_frequencies_shared_area(frequencies_path, frequencies, size);
	for (i = 0; i < ps; i++) {
		verbose(0, "shared_frequencies[%d]=%lu", i, shared_frequencies[i]);
	}
}

int is_valid_sec_tag(ulong tag) {
#if 1
	// You make define a constant at makefile time called SEC_KEY to use that option
	return ((ulong) SEC_KEY == tag);
#endif
}

void set_verbose_variables() {
	verb_level = my_cluster_conf.eard.verbose;
	VERB_SET_FD(fd_my_log);
	ERROR_SET_FD(fd_my_log);
	WARN_SET_FD(fd_my_log);
	DEBUG_SET_FD(fd_my_log);
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

#if 0
void eard_lock(char *tmp_dir)
{
	sprintf(eard_lock_file, "%s/%s.eard_lock", tmp_dir, nodename);

	if ((eard_lockf=open(eard_lock_file,O_WRONLY|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR))<0)
	{
		if (errno != EEXIST) {
			error("Error, creating lock file %s fails (%s)\n",
						eard_lock_file, strerror(errno));
		} else {
			error("Error opening daemon lock file in %s (%s)\n",
						eard_lock_file, strerror(errno));}
		_exit(0);
	}
}

void eard_unlock()
{
	close(eard_lockf);
	verbose(VEARD,"removing file %s\n",eard_lock_file);
	if (unlink(eard_lock_file)<0) error("Error when removing lock file %s:%s\n",eard_lock_file,strerror(errno));
}
#endif

// Creates a pipe to receive requests for applications. eard creates 1 pipe (per node) and service to receive requests
void create_connector(char *ear_tmp, char *nodename, int i) {
	char ear_commreq[MAX_PATH_SIZE * 2];
	sprintf(ear_commreq, "%s/.ear_comm.req_%d", ear_tmp, i);
	// ear_comreq files will be used to send requests from the library to the eard
	if (mknod(ear_commreq, S_IFIFO | S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP | S_IROTH | S_IWOTH, 0) < 0) {
		if (errno != EEXIST) {
			error("Error creating ear communicator for requests %s\n", strerror(errno));
			error_connector=1;
		}
	}
	if (chmod(ear_commreq, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)<0){
		error("Error setting privileges for eard-earl connection");
		error_connector=1;
	}
}

void connect_service(int req, application_t *new_app) {
	char ear_commack[MAX_PATH_SIZE * 2];
	unsigned long ack;
	int connect = 1;
	int alive;
	job_t *new_job = &new_app->job;
	int pid = create_ID(new_job->id, new_job->step_id);
	// Let's check if there is another application
	verbose(1, "request for connection at service %d (%lu,%lu)", req,new_job->id,new_job->step_id);
	
	if (is_new_application() || is_new_service(req, pid)) {
		connect = 1;
	} else {
		connect = 0;

		//FIXME: implicit
		if (check_ping()) alive = application_timeout();
		if (alive == 0) connect = 1;
	}


	// Creates 1 pipe (per node) to send acks.
	if (connect) {
		verbose(VEARD + 1, "Connected new job job_id=%lu step_id=%lu\n", new_job->id, new_job->step_id);
		sprintf(ear_commack, "%s/.ear_comm.ack_%d.%d", ear_tmp, req, pid);
		application_id = pid;

		// ear_commack will be used to send ack's or values (depending on the
		// requests) from eard to the library
		verbose(VEARD + 1, "Creating ack comm %s pid=%d", ear_commack, pid);

		if (mknod(ear_commack, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0) < 0) {
			if (errno != EEXIST) {
				error("Error when creating ear communicator for ack %s", strerror(errno));
				eard_close_comm();
			}
		}

		chmod(ear_commack, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		// At first service connection, we use the ping conn file
		if (req == 0) {
			// We open ping connection  for writting
			sprintf(ear_ping, "%s/.ear_comm.ping.%d", ear_tmp, pid);

			debug("application %d connected", pid);
			debug("opening ping conn for %d", pid);
			ear_ping_fd = open(ear_ping, O_WRONLY);

			if (ear_ping_fd < 0) {
				error("ERROR while opening ping pipe %s (%s)", ear_ping, strerror(errno));
				eard_close_comm();
			}

			// We must modify the client api to send more information
			powermon_mpi_init(&handler_energy, new_app);

			debug("sending ack for service %d", req);
			if (write(ear_ping_fd, &ack, sizeof(ack)) != sizeof(ack)) {
				warning("WARNING while writting for ping conn for %d", pid);
			}

			debug("connecting service %s", ear_commack);
			if ((ear_fd_ack[req] = open(ear_commack, O_WRONLY)) < 0) {
				error("ERROR when opening ear communicator for ack (%s)", strerror(errno));
				eard_close_comm();
			}
		} else {
			// eard only suppports one application connected, the second one will block
			verbose(VEARD + 1, "Process pid %d rejected as master", pid);
		}
		verbose(VEARD + 1, "Process pid %d selected as master", pid);
		verbose(VEARD + 1, "service %d connected", req);
	}
	verbose(1,"Application connected with local API");
}

// Checks application connections
int is_new_application() {
	if (application_id == -1) return 1;
	else return 0;
}

// Application is already connected but it is a new service
int is_new_service(int req, int pid) {
	if ((application_id == pid) && (ear_fd_ack[req] == -1)) return 1;
	else return 0;
}

// Checks ping fd is ok
int check_ping() {
	return (ear_ping_fd >= 0);
}

// Checks the application is still alive
int application_timeout() {
	int ret;
	int alive = 1;
	if (check_ping()) {
		ret = write(ear_ping_fd, "1", 1);
		// If we can write, application is still alive
		if (ret == 1) {
			int fd;
			char c;
			fd = open(ear_ping, O_RDONLY);
			if (fd < 0) {
				alive = 0;
				ear_ping_fd = -1;
			} else {
				read(fd, &c, 1);
				close(fd);
			}
		} else alive = 0;
	} else alive = 0;
	return alive;
}

/*
*	Restarts again EARD
*/

void eard_restart() {
	char *ear_install;
	char my_bin[MAX_PATH_SIZE];
	ear_install = getenv("EAR_INSTALL_PATH");
	if (ear_install == NULL) {
		error("EAR_INSTALL_PATH is NULL\n");
		sprintf(my_bin, "eard");
	} else sprintf(my_bin, "%s/sbin/eard", ear_install);
	verbose(VCONF, "LD_LIBRARY_PATH=%s\n", getenv("LD_LIBRARY_PATH"));
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
	state_t s;
	int i;

	verbose(VCONF, "Exiting");
#if EARD_LOCK
	eard_unlock();
#endif

	// Come disposes
	//print_rapl_metrics();

	// Recovering old frequency and governor configurations.
	verbose(VCONF, "frequency_dispose");
	frequency_dispose();

#if POWERCAP
  powercap_end();
#endif


	verbose(VCONF, "Releasing node resources");

	// More disposes
	if (state_fail(energy_dispose(&eard_handler_energy))) {
		error("Error disposing energy for eard");
	}

	// Database cache daemon disconnect
#if USE_EARDB
	if (my_cluster_conf.eard.use_eardbd){
		eardbd_disconnect();
	}
#endif

	verbose(VCONF, "Releasing files");

	// Removing files (services)
	for (i = 0; i < ear_daemon_client_requests; i++) {
		close(ear_fd_req[i]);

	}

	// CPU Frequency
	state_assert(s, freq_cpu_dispose(), );
	// RAM Bandwidth
	dispose_uncores();

	/* Releasing shared memory */
	settings_conf_shared_area_dispose(dyn_conf_path);
	resched_shared_area_dispose(resched_path);
	coeffs_shared_area_dispose(coeffs_path);
	coeffs_default_shared_area_dispose(coeffs_default_path);
	services_conf_shared_area_dispose(services_conf_path);
	#if POWERCAP
	app_mgt_shared_area_dispose(app_mgt_path);
	#endif
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

// HIGHLIGHT: LIBRARY DISPOSE (2/2)
void eard_close_comm() {
	char ear_commack[MAX_PATH_SIZE * 2];
	int dis_pid = application_id;
	int i;

	// closes communication with an application
	verbose(VEARD + 1, "closing comm in %s for %d", nodename, application_id);

	// Stop counting
	if (RAPL_counting) {
		//reset_rapl_metrics();
		RAPL_counting = 0;
	}

	for (i = 0; i < ear_daemon_client_requests; i++) {
		close(ear_fd_ack[i]);
		ear_fd_ack[i] = -1;

		sprintf(ear_commack, "%s/.ear_comm.ack_%d.%d", ear_tmp, i, application_id);
		verbose(VEARD + 1, "removing file %s", ear_commack);

		if (unlink(ear_commack) < 0) {
			error("ERROR when removing ack file %s (%s)", ear_commack, strerror(errno));
		}
	}

	close(ear_ping_fd);

	// We must send the app signature to the powermonitoring thread
	powermon_mpi_finalize(&handler_energy);

	application_id = -1;
	ear_ping_fd = -1;

	verbose(VEARD + 1, "removing file %s", ear_ping);

	if (unlink(ear_ping) < 0) {
		error("ERROR when removing ping file %s (%s)", ear_ping, strerror(errno));
	}
	verbose(VEARD + 1, "application %d disconnected", dis_pid);
}

/************************* Node_energy services ******************/
int eard_node_energy(int must_read) {
	unsigned long ack;
	if (must_read) {
		if (read(ear_fd_req[node_energy_req], &req, sizeof(req)) != sizeof(req))
			error("error reading info at eard_node_energy (%s)\n", strerror(errno));
	}
	switch (req.req_service) {
		case CONNECT_ENERGY:
			connect_service(node_energy_req, &req.req_data.app);
			break;
		case READ_DC_ENERGY:
			energy_dc_read(&handler_energy, node_energy_data);
			write(ear_fd_ack[node_energy_req], node_energy_data, node_energy_datasize);
			break;
		case DATA_SIZE_ENERGY_NODE:
			energy_datasize(&handler_energy, &ack);
			write(ear_fd_ack[node_energy_req], &ack, sizeof(unsigned long));
			break;
		case ENERGY_FREQ:
			ack = (ulong) energy_freq;
			write(ear_fd_ack[node_energy_req], &ack, sizeof(unsigned long));
			break;
		default:
			return 0;
	}
	return 1;

}


int eard_system(int must_read) {
	unsigned long ack;
	int ret1;

	if (must_read) {
		if (read(ear_fd_req[system_req], &req, sizeof(req)) != sizeof(req))
			error("error reading info at eard_system(%s)\n", strerror(errno));
	}

	switch (req.req_service) {
		case CONNECT_SYSTEM:
			connect_service(system_req, &req.req_data.app);
			break;
		case WRITE_APP_SIGNATURE:
			powermon_mpi_signature(&req.req_data.app);
			ack = EAR_COM_OK;
			if (write(ear_fd_ack[system_req], &ack, sizeof(ulong)) != sizeof(ulong)) {
				error("ERROR while writing system service ack, closing connection...");
				eard_close_comm();
			}

			break;
		case WRITE_EVENT:
			ack = EAR_COM_OK;
			ret1 = EAR_SUCCESS;
#if USE_DB
		if (my_cluster_conf.eard.use_mysql)
		{
			if (!my_cluster_conf.eard.use_eardbd) {
				ret1 = db_insert_ear_event(&req.req_data.event);
			} else {
				edb_state_t state = eardbd_send_event(&req.req_data.event);

				if (edb_state_fail(state)) {
					error("Error sending event to eardb");
					eardbd_reconnect(&my_cluster_conf, my_node_conf, state);
					ret1 = EAR_ERROR;
				}
			}
		}
#endif

			if (ret1 == EAR_SUCCESS) ack = EAR_COM_OK;
			else ack = EAR_COM_ERROR;

			write(ear_fd_ack[system_req], &ack, sizeof(ulong));
			break;
		case WRITE_LOOP_SIGNATURE:
			ack = EAR_COM_OK;
			ret1 = EAR_SUCCESS;
			copy_loop(&current_loop_data,&req.req_data.loop);
			// print_loop_fd(1,&req.req_data.loop);
			if (my_cluster_conf.database.report_loops) {
#if USE_DB
				if (my_cluster_conf.eard.use_mysql)
				{
					if (!my_cluster_conf.eard.use_eardbd){
						ret1 = db_insert_loop (&req.req_data.loop);
					} else {
						edb_state_t state = eardbd_send_loop(&req.req_data.loop);

						if (edb_state_fail(state)){
							error("Error sending loop to eardb");
							eardbd_reconnect(&my_cluster_conf, my_node_conf, state);
							ret1 = EAR_ERROR;
						}
					}
				}
#endif
			}

			if (ret1 == EAR_SUCCESS) ack = EAR_COM_OK;
			else ack = EAR_COM_ERROR;

			write(ear_fd_ack[system_req], &ack, sizeof(ulong));
			break;
		default:
			return 0;
	}
	return 1;
}

/************************ FREQUENCY FUNCTIONALLITY: max_freq is the limit *******************/
void eard_set_freq(unsigned long new_freq, unsigned long max_freq) {
	unsigned long ear_ok, freq;
	verbose(VCONF, "setting node frequency . requested %lu, max %lu\n", new_freq, max_freq);
	if (new_freq <= max_freq) {
		freq = new_freq;
	} else {
		warning(" warning, maximum freq is limited to %lu\n", max_freq);
		freq = max_freq;
	}
	ear_ok = frequency_set_all_cpus(freq);
	current_node_freq = freq;
	if (ear_ok != freq) warning(" warning, frequency is not correctly changed\n");
	write(ear_fd_ack[freq_req], &ear_ok, sizeof(unsigned long));
}

int eard_freq(int must_read)
{
	ulong ack;
	state_t s;

	if (must_read) {
		if (read(ear_fd_req[freq_req], &req, sizeof(req)) != sizeof(req))
			error("error when reading info at eard_freq (%s)\n", strerror(errno));
	}

	switch (req.req_service) {
		case CONNECT_FREQ:
			// HIGHLIGHT: LIBRARY INIT
			connect_service(freq_req, &req.req_data.app);
			break;
		case SET_FREQ:
			eard_set_freq(req.req_data.req_value, min(eard_max_freq, max_dyn_freq()));
			break;
		case START_GET_FREQ:
			ack = EAR_COM_OK;
			if (xtate_fail(s, freq_cpu_read(&freq_local1))) {
				error("when reading CPU local frequency (%d, %s)", s, state_msg);
			}
			write(ear_fd_ack[freq_req], &ack, sizeof(unsigned long));
			break;
		case END_GET_FREQ:
			if (xtate_fail(s, freq_cpu_read_diff(&freq_local2, &freq_local1, NULL, &ack))) {
				error("when reading CPU local frequency (%d, %s)", s, state_msg);
			}
			write(ear_fd_ack[freq_req], &ack, sizeof(unsigned long));
			break;
		case SET_TURBO:
			frequency_set_performance_governor_all_cpus();
			write(ear_fd_ack[freq_req], &ack, sizeof(unsigned long));
			break;
		case END_COMM:
			// HIGHLIGHT: LIBRARY DISPOSE (1/2)
			eard_close_comm();
			break;
		case DATA_SIZE_FREQ:
			ack = sizeof(unsigned long);
			write(ear_fd_ack[freq_req], &ack, sizeof(unsigned long));
			break;
		case START_APP_COMP_FREQ:
			ack = EAR_COM_OK;
			if (xtate_fail(s, freq_cpu_read(&freq_global1))) {
				error("when reading CPU global frequency (%d, %s)", s, state_msg);
			}
			write(ear_fd_ack[freq_req], &ack, sizeof(unsigned long));
			break;
		case END_APP_COMP_FREQ:
			if (xtate_fail(s, freq_cpu_read_diff(&freq_global2, &freq_global1, NULL, &ack))) {
				error("when reading CPU global frequency (%d, %s)", s, state_msg);
			}
			write(ear_fd_ack[freq_req], &ack, sizeof(unsigned long));
			break;

		default:
			return 0;

	}
	return 1;
}

int eard_uncore(int must_read) {

	unsigned long ack = EAR_SUCCESS;
	unsigned long long values[num_uncore_counters];

	if (must_read) {
		if (read(ear_fd_req[uncore_req], &req, sizeof(req)) != sizeof(req))
			error("eard error when reading info at eard_uncore \n");
	}

	switch (req.req_service) {
		case CONNECT_UNCORE:
			connect_service(uncore_req, &req.req_data.app);
			break;
		case START_UNCORE:
			start_uncores();
			write(ear_fd_ack[uncore_req], &ack, sizeof(ack));
			break;
		case RESET_UNCORE:
			printf("RESET_UNCORE in eard\n");
			reset_uncores();
			write(ear_fd_ack[uncore_req], &ack, sizeof(ack));
			break;
		case READ_UNCORE: {
			unsigned long demon_cas = 0;
			int i;
			read_uncores(values);
			write(ear_fd_ack[uncore_req], values, sizeof(unsigned long long) * num_uncore_counters);
			for (i = 0; i < num_uncore_counters; i++) demon_cas += values[i];
			break;
		}
		case STOP_UNCORE: {
			unsigned long demon_cas = 0;
			int i;
			stop_uncores(values);
			write(ear_fd_ack[uncore_req], values, sizeof(unsigned long long) * num_uncore_counters);
			for (i = 0; i < num_uncore_counters; i++) demon_cas += values[i];
			break;
		}

		case DATA_SIZE_UNCORE:
			ack = sizeof(unsigned long long) * num_uncore_counters;
			write(ear_fd_ack[uncore_req], &ack, sizeof(unsigned long));
			break;
		default:
			return 0;

	}
	return 1;
}


/*******************+ RAPL SERVICES *******************/
int eard_rapl(int must_read) {
	unsigned long comm_req = rapl_req;
	unsigned long ack = 0;
	if (must_read) {
		if (read(ear_fd_req[comm_req], &req, sizeof(req)) != sizeof(req))
			error("error when reading info at eard_rapl\n");
	}
	switch (req.req_service) {
		case CONNECT_RAPL:
			connect_service(rapl_req, &req.req_data.app);
			break;
		case START_RAPL:
			error("starting rapl msr not supported");
			RAPL_counting = 1;
			write(ear_fd_ack[comm_req], &ack, sizeof(ack));
			break;
		case RESET_RAPL:
			error("Reset RAPL counters not provided");
			write(ear_fd_ack[comm_req], &ack, sizeof(ack));
			break;
		case READ_RAPL:
			if (read_rapl_msr(fd_rapl,values_rapl)<0) error("Error reading rapl msr counters");
			RAPL_counting = 0;
			write(ear_fd_ack[comm_req], values_rapl, sizeof(unsigned long long) * RAPL_POWER_EVS*num_packs);
			break;
		case DATA_SIZE_RAPL:
			ack = sizeof(unsigned long long) * RAPL_POWER_EVS*num_packs;
			write(ear_fd_ack[comm_req], &ack, sizeof(unsigned long));
			break;
		default:
			return 0;

	}
	return 1;
}

/***************** END RAPL SERVICES ***********/
/***************** GPU SERVICES ****************/
int eard_gpu(int must_read) 
{
  unsigned long comm_req = gpu_req;
  unsigned long ack = 0;
	unsigned int model=0;
	unsigned int dev_count=0;
	gpu_t my_gpu;
	state_t ret;
  if (must_read) {
    if (read(ear_fd_req[comm_req], &req, sizeof(req)) != sizeof(req))
      error("error when reading info at eard_gpu\n");
  }
  switch (req.req_service) {
    case GPU_MODEL:
			//if (eard_gpu_initialized) ret=gpu_model(&eard_main_gpu_ctx,&model);
			if (eard_gpu_initialized) model=MODEL_NVML;
			else model=MODEL_UNDEFINED;
			write(ear_fd_ack[comm_req],&model,sizeof(model));	
      break;
    case GPU_DEV_COUNT:
			if (eard_gpu_initialized) gpu_count(&eard_main_gpu_ctx,&dev_count);
			write(ear_fd_ack[comm_req],&dev_count,sizeof(dev_count));	
			break;
		case GPU_DATA_READ:
			if (eard_gpu_initialized){
				gpu_read(&eard_main_gpu_ctx,&my_gpu);
			}else{
				memset(&my_gpu,0,sizeof(gpu_t));
			}
			write(ear_fd_ack[comm_req],&my_gpu,sizeof(my_gpu));
			break;
	  default:
			error("Invalid GPU command");
      return 0;
  }
  return 1;
}

void select_service(int fd) {
	if (read(ear_fd_req[freq_req], &req, sizeof(req)) != sizeof(req))
		error("error when reading info at select_service\n");
	if (!is_valid_sec_tag(req.sec)) {
		error("Invalid connection with eard %lu", req.sec);
		return;
	}
	if (eard_freq(0)) {
		return;
	}
	if (eard_uncore(0)) {
		return;
	}
	if (eard_rapl(0)) {
		return;
	}
	if (eard_system(0)) {
		return;
	}
	if (eard_node_energy(0)) {
		return;
	}
	if (eard_gpu(0)){
		return;
	}
	error(" Error, request received not supported\n");
	eard_close_comm();
}

//
// MAIN eard: eard default_freq [path for communication files]
//
// eard creates different pipes to receive requests and to send values requested
void Usage(char *app) {
	verbose(0, "Usage: %s [-h|verbose_level]", app);
	verbose(0, "\tear.conf file is used to define node settings. It must be available at");
	verbose(0, "\t $EAR_ETC/ear/ear.conf");
	_exit(1);
}

//region SIGNALS
void signal_handler(int sig) {
	if (sig == SIGPIPE) { verbose(VCONF, "signal SIGPIPE received. Application terminated abnormally"); }
	if (sig == SIGTERM) { verbose(VCONF, "signal SIGTERM received. Finishing"); }
	if (sig == SIGINT) { verbose(VCONF, "signal SIGINT received. Finishing"); }
	if (sig == SIGHUP) {
		verbose(VCONF, "signal SIGHUP received. Reloading EAR conf");
		signal_sighup = 1;
	}

	// The PIPE was closed, so the daemon connection ends
	if (sig == SIGPIPE) {
		eard_close_comm();
	}


	// Someone wants EARD to get closed or restarted
	if ((sig == SIGTERM) || (sig == SIGINT)) {
		eard_must_exit = 1;

		if (ear_ping_fd > 0) {
			verbose(VCONF, "application status = connected");
			eard_close_comm();
		}
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
    			configure_new_values(dyn_conf,resched_conf,&my_cluster_conf,my_node_conf);
    			verbose(VCONF,"shared memory updated max_freq %lu th %lf resched %d\n", dyn_conf->max_freq,
                    dyn_conf->settings[0], resched_conf->force_rescheduling);
				save_eard_conf(&eard_dyn_conf);
			}

#if USE_DB 
			if (!my_cluster_conf.eard.use_eardbd && eardbd_connected){
					eardbd_disconnect();
					eardbd_connected=0;
			}
			if (my_cluster_conf.eard.use_mysql)
			{
				if (my_cluster_conf.eard.use_eardbd)
				{
					eardbd_disconnect();
					edb_state_t state = eardbd_connect(&my_cluster_conf, my_node_conf);

					if (edb_state_fail(state)) {
						error("Error connecting with EARDB");
					} else {
						verbose(VCONF,"Connecting with EARDBD\n");
						eardbd_connected=1;
					}
				}

				if (!my_cluster_conf.eard.use_eardbd){
					verbose(VCONF,"Connecting with EAR DB directly");
#ifdef USE_EARDBD_CONF
					read_eardbd_conf(my_eardbd_conf_path,eardbd_user,eardbd_pass);
					strcpy(my_cluster_conf.database.user,eardbd_user);
					strcpy(my_cluster_conf.database.pass,eardbd_pass);
#endif
					init_db_helper(&my_cluster_conf.database);
					db_helper_connected=1;
				}
			}
#endif

		}
		verbose(VCONF, "Configuration reloaded");
	}

}

void signal_catcher() {
	struct sigaction action;
	int signal;
	sigset_t eard_mask;

	sigfillset(&eard_mask);
	sigdelset(&eard_mask, SIGPIPE);
	sigdelset(&eard_mask, SIGTERM);
	sigdelset(&eard_mask, SIGINT);
	sigdelset(&eard_mask, SIGHUP);
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

}
//endregion

void configure_new_values(settings_conf_t *dyn, resched_t *resched, cluster_conf_t *cluster, my_node_conf_t *node) {
	policy_conf_t *my_policy;
	ulong deff;
	eard_max_pstate = node->max_pstate;
	// Default policy is just in case
    default_policy_context.policy=MONITORING_ONLY;
    default_policy_context.p_state=EAR_MIN_P_STATE;
    default_policy_context.settings[0]=0;
    my_policy=get_my_policy_conf(node,cluster->default_policy);
    if (my_policy==NULL){
        // This should not happen
        error("Default policy  not found in ear.conf");
        my_policy=&default_policy_context;
    }else{
		default_policy_context.policy=my_policy->policy;
		default_policy_context.p_state=my_policy->p_state;
		default_policy_context.settings[0]=my_policy->settings[0];
	}
    deff=frequency_pstate_to_freq(my_policy->p_state);
	/* PENDING: we have to check we are not executing an application */
	dyn->policy=cluster->default_policy;
    dyn->max_freq=frequency_pstate_to_freq(node->max_pstate);
    dyn->def_freq=deff;
    memcpy(dyn->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
	dyn->min_sig_power=node->min_sig_power;
	dyn->max_sig_power=node->max_sig_power;
	dyn->max_power_cap=node->max_power_cap;
	dyn->report_loops=cluster->database.report_loops;
	memcpy(&dyn->installation,&cluster->install,sizeof(conf_install_t));
	resched->force_rescheduling=1;
	copy_ear_lib_conf(&dyn->lib_info,&cluster->earlib);
	f_monitoring=my_cluster_conf.eard.period_powermon;
	copy_eard_conf(&my_services_conf->eard,&my_cluster_conf.eard);
	copy_eargmd_conf(&my_services_conf->eargmd,&my_cluster_conf.eargm);
	copy_eardb_conf(&my_services_conf->db,&my_cluster_conf.database);
	copy_eardbd_conf(&my_services_conf->eardbd,&my_cluster_conf.db_manager);
	save_eard_conf(&eard_dyn_conf);
}

void configure_default_values(settings_conf_t *dyn, resched_t *resched, cluster_conf_t *cluster, my_node_conf_t *node) {
	policy_conf_t *my_policy;
	ulong deff;
	eard_max_pstate = node->max_pstate;
	verbose(1,"Max pstate for this node is %d",eard_max_pstate);
	// Default policy is just in case
	default_policy_context.policy=MONITORING_ONLY;
	default_policy_context.p_state=EAR_MIN_P_STATE;
	default_policy_context.settings[0]=0;
	my_policy=get_my_policy_conf(node,cluster->default_policy);
	if (my_policy==NULL){
		// This should not happen
		error("Default policy  not found in ear.conf");
		my_policy=&default_policy_context;
	}else{
		default_policy_context.policy=my_policy->policy;
		default_policy_context.p_state=my_policy->p_state;
		default_policy_context.settings[0]=my_policy->settings[0];
	}
		verbose(1,"configure_default_values. Looking for pstate %d\n",my_policy->p_state);
    deff=frequency_pstate_to_freq(my_policy->p_state);
    dyn->user_type=NORMAL;
	dyn->learning=0;
    dyn->lib_enabled=1;
	dyn->policy=cluster->default_policy;
	dyn->max_freq=frequency_pstate_to_freq(node->max_pstate);
    dyn->def_freq=deff;
    memcpy(dyn->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
	dyn->min_sig_power=node->min_sig_power;
	dyn->max_sig_power=node->max_sig_power;
	dyn->max_power_cap=node->max_power_cap;
	dyn->report_loops=cluster->database.report_loops;
	dyn->max_avx512_freq=my_node_conf->max_avx512_freq;
	dyn->max_avx2_freq=my_node_conf->max_avx2_freq;
	memcpy(&dyn->installation,&cluster->install,sizeof(conf_install_t));


	copy_ear_lib_conf(&dyn->lib_info,&cluster->earlib);
	f_monitoring=my_cluster_conf.eard.period_powermon;
	resched_conf->force_rescheduling=0;
    copy_eard_conf(&my_services_conf->eard,&my_cluster_conf.eard);
    copy_eargmd_conf(&my_services_conf->eargmd,&my_cluster_conf.eargm);
    copy_eardb_conf(&my_services_conf->db,&my_cluster_conf.database);
	copy_eardbd_conf(&my_services_conf->eardbd,&my_cluster_conf.db_manager);
	save_eard_conf(&eard_dyn_conf);
}

/** Basic functions to check if coeffs exist */
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

int read_coefficients_default() {
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
	if (error_uncore)
	{
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,UNCORE_INIT_ERROR,0);
	}
	if (error_rapl)
	{
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,RAPL_INIT_ERROR,0);
	}
	if (error_energy)
	{
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,ENERGY_INIT_ERROR,0);
	}
	if (error_connector)
	{
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,CONNECTOR_INIT_ERROR,0);
		
	}
}

void     update_global_configuration_with_local_ssettings(cluster_conf_t *my_cluster_conf,my_node_conf_t *my_node_conf)
{
	strcpy(my_cluster_conf->install.obj_ener,my_node_conf->energy_plugin);
	strcpy(my_cluster_conf->install.obj_power_model,my_node_conf->energy_model);
}



/*
*
*
*	EARD MAIN
*
*
*/

int main(int argc, char *argv[]) {
	struct timeval *my_to;
	struct timeval tv;

	char ear_commreq[MAX_PATH_SIZE * 2];

	unsigned long ear_node_freq;
	int cpu_model;

	int numfds_ready;
	int numfds_req = 0;

	fd_set rfds;
	fd_set rfds_basic;

	int max_fd = -1;
	int ret;
	int i;

	// Usage
	if (argc > 2) {
		Usage(argv[0]);
	}
	// We get nodename to create per_node files
	if (gethostname(nodename, sizeof(nodename)) < 0) {
		error("Error getting node name (%s)", strerror(errno));
		_exit(1);
	}
	strtok(nodename, ".");
	verbose(0,"Executed in node name %s", nodename);

	/** CONFIGURATION **/
	int node_size;
	state_t s;

	init_eard_rt_log();
	log_report_eard_min_interval(MIN_INTERVAL_RT_ERROR);
	verbose(0,"Reading hardware topology");

	/* We initialize topology */
	s = topology_init(&node_desc);
	node_size = node_desc.cpu_count;
	num_packs = node_desc.socket_count;

	if (state_fail(s) || node_size <= 0 || num_packs <= 0) {
		error("topology information can't be initialized (%d)", s);
		_exit(1);
	}

	verbose(0,"Topology detected: sockets %d, cores %d, threads %d",
			node_desc.socket_count, node_desc.core_count, node_desc.cpu_count);

	//
	verbose(0,"Initializing frequency list");
	values_rapl=(unsigned long long*)calloc(num_packs*RAPL_POWER_EVS,sizeof(unsigned long long));
	if (values_rapl==NULL) error("values_rapl returns NULL in eard initialization");

	/* We initialize frecuency */
	if (frequency_init(node_size) < 0) {
			error("frequency information can't be initialized");
			_exit(1);
	}



	verbose(0,"Reading ear.conf configuration");
	// We read the cluster configuration and sets default values in the shared memory
	if (get_ear_conf_path(my_ear_conf_path) == EAR_ERROR) {
		error("Error opening ear.conf file, not available at regular paths ($EAR_ETC/ear/ear.conf)");
		_exit(0);
	}
#if USE_DB
#ifdef USE_EARDBD_CONF
	if (get_eardbd_conf_path(my_eardbd_conf_path)==EAR_ERROR){
		error("Error opening eardbd.conf file, not available at regular path ( $EAR_ETC/ear/eardbd.conf)");
		_exit(0);
	}
#endif
#endif
	if (read_cluster_conf(my_ear_conf_path, &my_cluster_conf) != EAR_SUCCESS) {
		error(" Error reading cluster configuration\n");
		_exit(1);
	} else {
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
	verbose(0,"Initializing dynamic information and creating tmp");
	/* This info is used for eard checkpointing */
	eard_dyn_conf.cconf = &my_cluster_conf;
	eard_dyn_conf.nconf = my_node_conf;
	eard_dyn_conf.pm_app = get_powermon_app();
	set_global_eard_variables();
	verbose(0,"Creating tmp dir");
	create_tmp(ear_tmp);
	if (my_cluster_conf.eard.use_log) {
		verbose(0,"Creating log file");
		fd_my_log = create_log(my_cluster_conf.install.dir_temp, "eard");
		if (fd_my_log < 0) fd_my_log = 2;
	}
	set_verbose_variables();

	verbose(0,"Creating frequency list in shared memory");
	/** Shared memory is used between EARD and EARL **/
	init_frequency_list();

	/**** SHARED MEMORY REGIONS ****/
	/* This area is for shared info */
	verbose(0, "creating shared memory regions");
	get_settings_conf_path(my_cluster_conf.install.dir_temp, dyn_conf_path);
	verbose(VCONF + 1, "Using %s as settings path (shared memory region)", dyn_conf_path);
	dyn_conf = create_settings_conf_shared_area(dyn_conf_path);
	if (dyn_conf == NULL) {
		error("Error creating shared memory between EARD & EARL\n");
		_exit(0);
	}
	/* This area indicates EARL must resched */
	get_resched_path(my_cluster_conf.install.dir_temp, resched_path);
	verbose(VCONF + 1, "Using %s as resched path (shared memory region)", resched_path);
	resched_conf = create_resched_shared_area(resched_path);
	if (resched_conf == NULL) {
		error("Error creating shared memory between EARD & EARL");
		_exit(0);
	}
	/* This area is for application data */
	#if POWERCAP
	get_app_mgt_path(my_cluster_conf.install.dir_temp,app_mgt_path);
	verbose(VCONF + 1, "Using %s as app_mgt data path (shared memory region)", app_mgt_path);
	app_mgt_info=create_app_mgt_shared_area(app_mgt_path);
	if (app_mgt_info==NULL){
		error("Error creating shared memory between EARD & EARL for app_mgt");
		_exit(0);
	}
	get_pc_app_info_path(my_cluster_conf.install.dir_temp,pc_app_info_path);
	verbose(VCONF + 1, "Using %s as pc_app_info dat apath (shared memory region)",pc_app_info_path);
	pc_app_info_data=create_pc_app_info_shared_area(pc_app_info_path);
	if (pc_app_info_data==NULL){
		error("Error creating shared memory between EARD & EARL for pc_app_info");
		_exit(0);
	}
	#endif
	verbose(0, "Basic shared memory regions created");
	/* Coefficients */
	verbose(0,"Loading coefficients");
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
	verbose(0,"Loading default coefficients");
	coeffs_default_size = read_coefficients_default();
	verbose(VCONF, "Coefficients by default loaded");
	if ((coeffs_size==0) && (coeffs_default_size==0)) verbose(0,"No coefficients found, power policies will not be applied");
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
	/**** END CREATION SHARED MEMORY REGIONS ****/
	verbose(0,"Shared memory regions created");

	/** We must control if we come from a crash **/
	int must_recover = new_service("eard");
	if (must_recover) {
		verbose(VCONF, "We must recover from a crash");
		restore_eard_conf(&eard_dyn_conf);
	}
	/* After potential recoveries, we set the info in the shared memory */
    configure_default_values(dyn_conf,resched_conf,&my_cluster_conf,my_node_conf);
    verbose(VCONF,"shared memory created max_freq %lu th %lf resched %d",dyn_conf->max_freq,dyn_conf->settings[0],resched_conf->force_rescheduling);

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
	verbose(0,"Catching signals");
	// We catch signals
	signal_catcher();


	if (eard_max_pstate < 0) Usage(argv[0]);
	if (eard_max_pstate >= frequency_get_num_pstates()) Usage(argv[0]);

	ear_node_freq = frequency_pstate_to_freq(eard_max_pstate);
	eard_max_freq = ear_node_freq;
	verbose(VCONF, "Default max frequency defined to %lu", eard_max_freq);

	// CPU Frequency
	state_assert(s, freq_cpu_init(&node_desc), _exit(0));
	state_assert(s, freq_cpu_data_alloc(&freq_global2, NULL, NULL), _exit(0));
	state_assert(s, freq_cpu_data_alloc(&freq_global1, NULL, NULL), _exit(0));
	state_assert(s, freq_cpu_data_alloc(&freq_local2, NULL, NULL), _exit(0));
	state_assert(s, freq_cpu_data_alloc(&freq_local1, NULL, NULL), _exit(0));
	state_assert(s, freq_cpu_data_alloc(&freq_job2, NULL, NULL), _exit(0));
	state_assert(s, freq_cpu_data_alloc(&freq_job1, NULL, NULL), _exit(0));

#if EARD_LOCK
	eard_lock(ear_tmp);
#endif
	// At this point, only one daemon is running

	verbose(VCONF, "Starting eard...................pid %d", getpid());
	verbose(VCONF + 1, "Creating comm files in %s with default freq %lu verbose set to %d",
			nodename, ear_node_freq, verb_level);

	// PAST: we had here a frequency set

	verbose(0,"Initializing uncore counters to compute memory bandwith");
	// We initiaize uncore counters
	cpu_model = get_model();
	verbose(0,"CPU Model detected %d",cpu_model);
	num_uncore_counters = init_uncores(cpu_model);
	verbose(VCONF + 1, "eard %d imc uncore counters detected", num_uncore_counters);
	if (num_uncore_counters <= 0){
		error("Uncore counters to compute memory bandwith not detected");
		error_uncore=1;
		num_uncore_counters=0;
	}
	verbose(0,"%d uncore counters detected ",num_uncore_counters);
	reset_uncores();
	/* Start uncore to have counters ready for reading */
	start_uncores();

	verbose(0,"Initializing RAPL counters");
	// We initialize rapl counters
	if (init_rapl_msr(fd_rapl)<0){ 
		error("Error initializing rapl");
		error_rapl=1;
	}
	verbose(0,"Initialzing energy plugin");
	// We initilize energy_node
	debug("Initializing energy in main EARD thread");
	if (state_fail(energy_init(&my_cluster_conf, &eard_handler_energy))) {
		error("energy_init cannot be initialized,DC node emergy metrics will not be provided\n");
		error_energy=1;
	}
  if (init_power_ponitoring(&handler_energy) != EAR_SUCCESS) {
    error("Error initializing pm_energy in %s thread", "EARD main");
		_exit(0);	
  }
	energy_datasize(&handler_energy,&node_energy_datasize);
	debug("EARD main thread: node_energy_datasize %lu",node_energy_datasize);
	node_energy_data=(edata_t)malloc(node_energy_datasize);

	// Energy accuracy
  energy_frequency(&handler_energy, &energy_freq);

	verbose(VCONF, "energy: power performance accuracy %lu usec", energy_freq);

	verbose(0,"Creating pipe for EARL");

	// HW initialized HERE...creating communication channels
	verbose(VCONF + 1, "Creating comm in %s for node %s", ear_tmp, nodename);
	FD_ZERO(&rfds);
	// We support a set of "types" of requests
	for (i = 0; i < ear_daemon_client_requests; i++) {
		create_connector(ear_tmp, nodename, i);
		ear_fd_ack[i] = -1;
	}

	for (i = 0; i < ear_daemon_client_requests; i++) {
		sprintf(ear_commreq, "%s/.ear_comm.req_%d", ear_tmp, i);
		if ((ear_fd_req[i] = open(ear_commreq, O_RDWR)) < 0) {
			error("Error opening ear communicator (%s)for requests %s\n", ear_commreq, strerror(errno));
			eard_close_comm();
		}
		FD_SET(ear_fd_req[i], &rfds);
		max_fd = max(max_fd, ear_fd_req[i]);
		numfds_req = max_fd + 1;
	}
	rfds_basic = rfds;
	verbose(0,"Connecting with EARDBD");
	// Database cache daemon
#if USE_DB
	if (my_cluster_conf.eard.use_mysql)
	{
		if (my_cluster_conf.eard.use_eardbd)
		{
			edb_state_t state = eardbd_connect(&my_cluster_conf, my_node_conf);

			if (edb_state_fail(state)) {
				error("Error connecting with EARDB errnum:%d errmsg:%s\n",
					intern_error_num,intern_error_str);
			} else {
				verbose(VCONF,"Connected with EARDBD");
				eardbd_connected=1;
			}
		}

		if (!my_cluster_conf.eard.use_eardbd)
		{
			verbose(VCONF,"Connecting with EAR DB directly");
#ifdef USE_EARDBD_CONF
	  read_eardbd_conf(my_eardbd_conf_path,eardbd_user,eardbd_pass);
	  strcpy(my_cluster_conf.database.user,eardbd_user);
	  strcpy(my_cluster_conf.database.pass,eardbd_pass);
#endif
			init_db_helper(&my_cluster_conf.database);
			db_helper_connected=1;
		}
	}
#endif

	report_eard_init_error();
	verbose(0,"Creating EARD threads");
	verbose(0,"Creating power  monitor thread");
#if POWERMON_THREAD
	if ((ret=pthread_create(&power_mon_th, NULL, eard_power_monitoring, NULL))){
		errno=ret;
		error("error creating power_monitoring thread %s\n",strerror(errno));
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,PM_CREATION_ERROR,ret);
	}
#endif
	verbose(0,"Creating the remote connections server");
#if EXTERNAL_COMMANDS_THREAD
	if ((ret=pthread_create(&dyn_conf_th, NULL, eard_dynamic_configuration, (void *)ear_tmp))){
		error("error creating dynamic_configuration thread \n");
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,DYN_CREATION_ERROR,ret);
	}
#endif
	verbose(0,"Creating APP-API thread");
#if APP_API_THREAD
	if ((ret=pthread_create(&app_eard_api_th,NULL,eard_non_earl_api_service,NULL))){
		error("error creating server thread for non-earl api\n");
		log_report_eard_init_error(my_cluster_conf.eard.use_mysql,my_cluster_conf.eard.use_eardbd,APP_API_CREATION_ERROR,ret);
	}
#endif

	/* GPU Initialization for app requests */
	ret=gpu_init(&eard_main_gpu_ctx);
	if (ret!=EAR_SUCCESS){
		error("EARD error when initializing GPU context for EARD API queries");
		eard_gpu_initialized=0;	
	}else{
		eard_gpu_initialized=1;
	}

	verbose(VCONF + 1, "Communicator for %s ON", nodename);
	// we wait until EAR daemon receives a request
	// We support requests realted to frequency and to uncore counters
	// rapl support is pending to be supported
	// EAR daemon main loop
	tv.tv_sec = 20;
	tv.tv_usec = 0;
	my_to = NULL;
	/*
	*
	*	MAIN LOOP 
	*
	*/
	verbose(0,"EARD initialized succesfully");
	while (((numfds_ready = select(numfds_req, &rfds, NULL, NULL, my_to)) >= 0) ||
		   ((numfds_ready < 0) && (errno == EINTR)))
	{
		if (eard_must_exit) {
			verbose(0, "eard exiting");
			eard_close_comm();
			eard_exit(2);
		}
		if (numfds_ready >= 0) {
			if (numfds_ready > 0) {
				for (i = 0; i < ear_daemon_client_requests; i++) {
					if (FD_ISSET(ear_fd_req[i], &rfds)) {
						select_service(i);
					}    // IF FD_ISSET
				} //for
				// We have to check if there is something else
			} else { //timeout
				debug("eard timeout...checking for application status\n");
				debug("eard...application connected\n");
				if (check_ping()) application_timeout();
			}
			// If application is disconnected, we wait for a new connection
			if (check_ping()) {
				tv.tv_sec = 20;
				tv.tv_usec = 0;
				my_to = &tv;
			} else {
				my_to = NULL;
			}
		} else {// Signal received
			my_to = NULL;
			debug("signal received");
		}
		rfds = rfds_basic;
		debug("eard waiting.....\n");
	}//while
	//eard is closed by SIGTERM signal, we should not reach this point
	verbose(VCONF, "END EARD\n");
	eard_close_comm();
	eard_exit(0);
	return 0;
}
