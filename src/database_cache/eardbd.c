/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
*/

#include <database_cache/eardbd.h>
#include <database_cache/eardbd_body.h>
#include <database_cache/eardbd_sync.h>
#include <database_cache/eardbd_signals.h>
#include <database_cache/eardbd_storage.h>
#if !OFFLINE
#include <common/config.h>
#include <common/database/db_helper.h>
#endif

// Configuration
cluster_conf_t conf_clus;

// Input buffers
packet_header_t input_header;
char input_buffer[SZ_BUFF_BIG];
char extra_buffer[SZ_BUFF_BIG];

// Sockets
static socket_t sockets[4];
socket_t *smets_srv = &sockets[0];
socket_t *smets_mir = &sockets[1];
socket_t *ssync_srv = &sockets[2];
socket_t *ssync_mir = &sockets[3];

// Descriptors
struct sockaddr_storage addr_cli;
fd_set fds_incoming;
fd_set fds_active;
int fd_cli;
int fd_min;
int fd_max;

//
long fd_hosts[FD_SETSIZE];

// Nomenclature:
// 	- Server: main buffer of the gathered metrics. Inserts buffered metrics in
//	the database.
// 	- Mirror: the secondary buffer of the gather metrics. Inserts the buffered
//	metrics in the database if the server is offline.
//	- Master: if in a node there is a server and a mirror, the master is the
//	server. If the server is not enabled because the admin wants the node to
//	be just a mirror, the mirror is the master.

// Mirroring
char master_host[SZ_NAME_MEDIUM]; // This node name
char master_alia[SZ_NAME_MEDIUM];
char master_name[SZ_NAME_MEDIUM];
char server_host[SZ_NAME_MEDIUM]; // If i'm mirror, which is the server?
static int server_port;
static int mirror_port;
static int synchr_port;
int server_too;
int mirror_too;
int master_iam; // Master is who speaks
int server_iam;
int mirror_iam;

// PID
char *path_pid;
process_data_t proc_data_srv;
process_data_t proc_data_mir;
pid_t server_pid;
pid_t mirror_pid;
pid_t others_pid;

// Synchronization
packet_header_t sync_ans_header;
packet_header_t sync_qst_header;
sync_qst_t sync_qst_content;
sync_ans_t sync_ans_content;

// Data
time_t glb_time1[MAX_TYPES];
time_t glb_time2[MAX_TYPES];
time_t ins_time1[MAX_TYPES];
time_t ins_time2[MAX_TYPES];
size_t typ_sizof[MAX_TYPES];
float  typ_vecmb[MAX_TYPES];
uint   typ_prcnt[MAX_TYPES];
float  typ_prcnl[MAX_TYPES];
uint   typ_index[MAX_TYPES];
char  *typ_alloc[MAX_TYPES];
char  *sam_iname[MAX_TYPES];
ulong  sam_inmax[MAX_TYPES];
ulong  sam_index[MAX_TYPES];
uint   sam_recvd[MAX_TYPES];
uint   soc_accpt;
uint   soc_activ;
uint   soc_discn;
uint   soc_unkwn;
uint   soc_tmout;

// Times (alarms)
struct timeval timeout_insr;
struct timeval timeout_aggr;
struct timeval timeout_slct;
time_t time_insr;
time_t time_aggr;
time_t time_slct;

// State machine
int reconfiguring;
int listening;
int releasing;
int exitting;
int updating;
int dreaming;
int veteran;
int forked;

// Extras
static float alloc;
sigset_t sigset;

// Strings
char *str_who[2] = { "server", "mirror" };
int verbosity = 0;

/*
 *
 * Init
 *
 */

static void init_general_configuration(int argc, char **argv, cluster_conf_t *conf_clus)
{
	int mode;
	char *p;

	// Cleaning 0 (who am I? Ok I'm the server, which is also the master for now)
	mirror_iam = 0;
	mirror_too = 0;
	server_iam = 1;
	server_too = 0;
	master_iam = 1;
	forked     = 0;
	server_pid = getpid();
	mirror_pid = 0;
	others_pid = 0;

	log_open("eardbd");

	// Getting host name and host alias
	gethostname(master_host, SZ_NAME_MEDIUM);
	gethostname(master_alia, SZ_NAME_MEDIUM);
	
	// Finding a possible short form
	if ((p = strchr(master_alia, '.')) != NULL) {
		p[0] = '\0';
	}

	// Configuration
	#if !OFFLINE
	if (get_ear_conf_path(extra_buffer) == EAR_ERROR) {
		_error("while getting ear.conf path");
	}

	verbose_maxxx("reading '%s' configuration file", extra_buffer);
	read_cluster_conf(extra_buffer, conf_clus);

	#ifdef USE_EARDBD_CONF
	// Database configuration
	if (get_eardbd_conf_path(extra_buffer) == EAR_ERROR){
		_error("while getting eardbd.conf path");
	}

	verbose_maxxx("reading '%s' database configuration file", extra_buffer);
	read_eardbd_conf(extra_buffer, conf_clus->database.user, conf_clus->database.pass);
	#endif

	// Output redirection
	if (conf_clus->db_manager.use_log) {
		verbose_maxxx("redirecting output to '%s'", conf_clus->install.dir_temp);
	}

	// Database
	init_db_helper(&conf_clus->database);

	debug("dbcon info: '%s@%s' access", conf_clus->database.database, conf_clus->database.user);
	debug("dbcon info: '%s:%d' socket", conf_clus->database.ip, conf_clus->database.port);

	// Mirror finding
	mode = get_node_server_mirror(conf_clus, master_host, server_host);

	server_too = (mode & 0x01);
	mirror_too = (mode & 0x02) > 0;

	#if 0
	conf_clus->db_manager.insr_time = atoi(argv[4]);
	conf_clus->db_manager.aggr_time = 60;

	server_too = atoi(argv[1]);
	mirror_too = atoi(argv[2]);

	strcpy(server_host, argv[3]);
	#endif
	#else
	conf_clus->db_manager.tcp_port      = 8811;
	conf_clus->db_manager.sec_tcp_port  = 8812;
	conf_clus->db_manager.sync_tcp_port = 8813;
	conf_clus->db_manager.mem_size      = 20;
	conf_clus->db_manager.aggr_time     = 60;
	conf_clus->db_manager.insr_time     = atoi(argv[4]);

	server_too = atoi(argv[1]);
	mirror_too = atoi(argv[2]);

	strcpy(server_host, argv[3]);
	strcpy(conf_clus->install.dir_temp, argv[5]);
	#endif

	// Ports
	server_port = conf_clus->db_manager.tcp_port;
	mirror_port = conf_clus->db_manager.sec_tcp_port;
	synchr_port = conf_clus->db_manager.sync_tcp_port;

	// Allocation
	alloc = (float) conf_clus->db_manager.mem_size;

	// PID protection
	pid_t other_server_pid;
	pid_t other_mirror_pid;
	path_pid = conf_clus->install.dir_temp;

	// Neither server nor mirror
	if (!server_too && !mirror_too) {
		_error("this node is not configured as a server nor mirror");
	}

	// PID test
	process_data_initialize(&proc_data_srv, "eardbd_test", path_pid);

	if (state_fail(process_pid_file_save(&proc_data_srv))) {
		_error("while testing the temporal directory '%s' (%s)",
			   conf_clus->install.dir_temp, intern_error_str);
	}

	process_pid_file_clean(&proc_data_srv);
	process_data_initialize(&proc_data_srv, "eardbd_server", path_pid);
	process_data_initialize(&proc_data_mir, "eardbd_mirror", path_pid);

	int server_xst = process_exists(&proc_data_srv, "eardbd", &other_server_pid);
	int mirror_xst = process_exists(&proc_data_mir, "eardbd", &other_mirror_pid);

	server_too = server_too && !server_xst;
	mirror_too = mirror_too && !mirror_xst;

	if (!server_xst) process_pid_file_clean(&proc_data_srv);
	if (!mirror_xst) process_pid_file_clean(&proc_data_mir);
	if ( server_xst) server_pid = other_server_pid;
	if ( mirror_xst) mirror_pid = other_mirror_pid;

	if (!server_too && !mirror_too) {
		_error("another EARDBD process is currently running");
	}

	// Server & mirro verbosity
	verbose_maxxx("enabled cache server: %s",                  server_too ? "OK": "NO");
	verbose_maxxx("enabled cache mirror: %s (of server '%s')", mirror_too ? "OK" : "NO", server_host);
}

static void init_time_configuration(int argc, char **argv, cluster_conf_t *conf_clus)
{
	// Times
	time_insr = (time_t) conf_clus->db_manager.insr_time;
	time_aggr = (time_t) conf_clus->db_manager.aggr_time;

	if (time_insr == 0) {
		verbose_maxxx("insert time can't be 0, using 300 seconds (default)");
		time_insr = 300;
	}
	if (time_aggr == 0) {
		verbose_maxxx("aggregation time can't be 0, using 60 seconds (default)");
		time_aggr = 60;
	}

	// Looking for multiple value
	time_reset_timeout_insr(0);
	time_reset_timeout_aggr();
	time_reset_timeout_slct();

	// Times verbosity
	verbose_maxxx("insertion time:   %lu seconds", time_insr);
	verbose_maxxx("aggregation time: %lu seconds", time_aggr);
}

static int init_sockets_single(socket_t *socket, char *host, int port, int bind)
{
	sockets_clean(socket);

	sockets_init(socket, host, port, TCP);

	//
	if (!bind) {
		return 0;
	}
	if (state_fail(sockets_socket(socket))) {
		return intern_error_num;
	}
	if (state_fail(sockets_bind(socket, 60))) {
		return intern_error_num;
	}
	if (state_fail(sockets_listen(socket))) {
		return intern_error_num;
	}

	sockets_nonblock_set(socket->fd);

	return 0;
}

static void init_sockets(int argc, char **argv, cluster_conf_t *conf_clus)
{
	//
	int errno1 = init_sockets_single(smets_srv, NULL, server_port, 1);
	int errno2 = init_sockets_single(smets_mir, NULL, mirror_port, mirror_too);
	int errno3 = init_sockets_single(ssync_srv, NULL, synchr_port, 1);
	int errno4 = init_sockets_single(ssync_mir, server_host, synchr_port, 0);

	if (errno1 != 0 || errno2 != 0 || errno3 != 0 || errno4 != 0) {
		_error("while binding sockets (%d %d %d %d)", errno1, errno2, errno3, errno4);
	}

	// Verbosity
	char *str_sta[3] = { "listen", "closed", "error" };

	int fd1 = smets_srv->fd;
	int fd2 = smets_mir->fd;
	int fd3 = ssync_srv->fd;
	int fd4 = ssync_mir->fd;
	int st1 = (fd1 == -1) + ((fd1 == -1) * (errno1 != 0));
	int st2 = (fd2 == -1) + ((fd2 == -1) * (errno2 != 0));
	int st3 = (fd3 == -1) + ((fd3 == -1) * (errno3 != 0));
	int st4 = (fd4 == -1) + ((fd4 == -1) * (errno4 != 0));

	// Summary
	tprintf_init(fderr, STR_MODE_DEF, "18 8 7 10 8 8");

	tprintf("type||port||prot||stat||fd||err");
	tprintf("----||----||----||----||--||---");
	tprintf("server metrics||%d||TCP||%s||%d||%d", smets_srv->port, str_sta[st1], fd1, errno1);
	tprintf("mirror metrics||%d||TCP||%s||%d||%d", smets_mir->port, str_sta[st2], fd2, errno2);
	tprintf("server sync||%d||TCP||%s||%d||%d",    ssync_srv->port, str_sta[st3], fd3, errno3);
	tprintf("mirror sync||%d||TCP||%s||%d||%d",    ssync_mir->port, str_sta[st4], fd4, errno4);
	verbose_maxxx("TIP! mirror sync socket opens and closes intermittently");
	verbose_maxxx("maximum #connections per process: %u", MAX_CONNECTIONS);
}

static void init_fork(int argc, char **argv, cluster_conf_t *conf_clus)
{
	// Fork
	if (mirror_too)
	{
		mirror_pid = fork();

		if (mirror_pid == 0)
		{
			// Reconfiguring
			mirror_iam = 1;
			server_iam = 0;

			mirror_pid = getpid();
			others_pid = server_pid;
			sleep(2);
		}
		else if (mirror_pid > 0) {
			others_pid = mirror_pid;
		} else {
			_error("error while forking, terminating the program");
		}
	}

	forked = 1;
	master_iam = (server_iam && server_too) || (mirror_iam && !server_too);

	// Verbosity
	char *str_sta[2] = { "(just sleeps)", "" };

	verbose_maxxx("cache server pid: %d %s", server_pid, str_sta[server_too]);
	verbose_maxxx("cache mirror pid: %d", mirror_pid);
}

static void init_signals()
{
	struct sigaction action;

	// Blocking all signals except USR1, USR2, PIPE, TERM, CHLD, INT and HUP
	sigfillset(&sigset);
	sigdelset(&sigset, SIGUSR1);
	sigdelset(&sigset, SIGUSR2);
	sigdelset(&sigset, SIGTERM);
	sigdelset(&sigset, SIGCHLD);
	sigdelset(&sigset, SIGINT);
	sigdelset(&sigset, SIGHUP);

	sigprocmask(SIG_SETMASK, &sigset, NULL);

	// Editing signals individually
	sigfillset(&action.sa_mask);
	//sigemptyset(&action.sa_mask);
	//sigaddset(&action.sa_mask, SIGCHLD);

	action.sa_handler = NULL;
	action.sa_sigaction = signal_handler;
	action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGUSR1, &action, NULL) < 0) { 
        	verbose_xaxxw("sigaction error on signal %d (%s)", SIGUSR1, strerror(errno));
	}
	if (sigaction(SIGUSR2, &action, NULL) < 0) {
		verbose_xaxxw("sigaction error on signal %d (%s)", SIGUSR1, strerror(errno));
	}
	if (sigaction(SIGCHLD, &action, NULL) < 0) {
		verbose_xaxxw("sigaction error on signal %d (%s)", SIGUSR1, strerror(errno));
	}
	if (sigaction(SIGTERM, &action, NULL) < 0) {
		verbose_xaxxw("sigaction error on signal %d (%s)", SIGTERM, strerror(errno));
	}
	if (sigaction(SIGINT, &action, NULL) < 0) {
		verbose_xaxxw("sigaction error on signal %d (%s)", SIGINT, strerror(errno));
	}
	if (sigaction(SIGHUP, &action, NULL) < 0) {
		verbose_xaxxw("sigaction error on signal %d (%s)", SIGHUP, strerror(errno));
	}
}

static void init_types_single(int i, char *name, size_t size, uint percent)
{
	float mb_single;
	float mb_batch;
	ulong b_batch;

	//
	sam_iname[i] = name;
	typ_sizof[i] = size;
	typ_prcnt[i] = typ_prcnt[i] + ((typ_prcnt[i] == 0) * percent);
	typ_prcnl[i] = ((float) typ_prcnt[i]) / 100.0;

	//
	mb_batch = ceilf(alloc * typ_prcnl[i]);
	mb_single = ((float) typ_sizof[i]) / 1000000.0;
	sam_inmax[i] = (ulong) (mb_batch / mb_single);

	if (sam_inmax[i] < 100 && sam_inmax[i] > 0) {
		sam_inmax[i] = 100;
	}

	// Allocation and set
	b_batch = typ_sizof[i] * sam_inmax[i];
	typ_vecmb[i] = (double) (b_batch) / 1000000.0;

	typ_alloc[i] = malloc(b_batch);
	memset(typ_alloc[i], 0, b_batch);

	// Verbosity
	tprintf("%s||%0.2f MBs||%lu Bs||%lu||%0.2f",
			sam_iname[i], typ_vecmb[i], typ_sizof[i], sam_inmax[i], typ_prcnl[i]);
}

static void init_process_configuration(int argc, char **argv, cluster_conf_t *conf_clus)
{
	uint per_total = 0;
	int i;

	// Single process configuration
	listening = (server_iam && server_too) || (mirror_iam);
	updating  = 0;

	// is not a listening socket?
	dreaming  = (server_iam && !listening);
	releasing = dreaming;

	// Socket closing
	if (mirror_iam) {
		// Destroying main sockets
		sockets_dispose(smets_srv);
		sockets_dispose(ssync_srv);

		// Keep track of the biggest file descriptor
		fd_max = smets_mir->fd;
		fd_min = smets_mir->fd;

		FD_SET(smets_mir->fd, &fds_active);
	} else {
		// Destroying mirror soockets
		sockets_dispose(smets_mir);

		// Keep track of the biggest file descriptor
		fd_max = ssync_srv->fd;
		fd_min = ssync_srv->fd;

		if (smets_srv->fd > fd_max) {
			fd_max = smets_srv->fd;
		}
		if (smets_srv->fd < fd_min) {
			fd_min = smets_srv->fd;
		}

		FD_SET(smets_srv->fd, &fds_active);
		FD_SET(ssync_srv->fd, &fds_active);
	}

	// Ok, all here is done
	if (dreaming) {
		return;
	}

	// Setting master name
	if (server_iam) sprintf(master_name, "%s-s", master_alia);
	else sprintf(master_name, "%s-m", master_alia);

	// Types allocation counting
	for (i = 0; i < EARDBD_TYPES; ++i) {
		per_total += conf_clus->db_manager.mem_size_types[i];
	}

	if (per_total > 90 && per_total < 110) {
		typ_prcnt[i_appsm] = conf_clus->db_manager.mem_size_types[0];
		typ_prcnt[i_appsn] = conf_clus->db_manager.mem_size_types[1];
		typ_prcnt[i_appsl] = conf_clus->db_manager.mem_size_types[2];
		typ_prcnt[i_loops] = conf_clus->db_manager.mem_size_types[3];
		typ_prcnt[i_enrgy] = conf_clus->db_manager.mem_size_types[4];
		typ_prcnt[i_aggrs] = conf_clus->db_manager.mem_size_types[5];
		typ_prcnt[i_evnts] = conf_clus->db_manager.mem_size_types[6];
	}

	// Verbose
	tprintf_init(fderr, STR_MODE_DEF, "15 15 11 9 8");

	if(!master_iam) {
		tprintf_close();
	}

	// Summary
	tprintf("type||memory||sample||elems||%%");
	tprintf("----||------||------||-----||----");

	init_types_single(i_appsm, "apps mpi",     sizeof(application_t),          60);
	init_types_single(i_appsn, "apps non-mpi", sizeof(application_t),          22);
	init_types_single(i_appsl, "apps learn.",  sizeof(application_t),          05);
	init_types_single(i_loops, "loops",        sizeof(loop_t),                 00);
	init_types_single(i_evnts, "events",       sizeof(ear_event_t),            07);
	init_types_single(i_enrgy, "energy reps",  sizeof(periodic_metric_t),      05);
	init_types_single(i_aggrs, "energy aggrs", sizeof(periodic_aggregation_t), 01);

	float mb_total = typ_vecmb[i_appsm] + typ_vecmb[i_appsn] + typ_vecmb[i_appsl] +
		 typ_vecmb[i_loops] + typ_vecmb[i_evnts] + typ_vecmb[i_enrgy] + typ_vecmb[i_aggrs];

	tprintf("TOTAL||%0.2f MBs", mb_total);
	verbose_maxxx("TIP! this allocated space is per process server/mirror");

	// Synchronization headers
	sockets_header_clean(&sync_ans_header);
	sockets_header_clean(&sync_qst_header);

	sync_ans_header.content_type = CONTENT_TYPE_ANS;
	sync_ans_header.content_size = sizeof(sync_ans_t);
	sync_qst_header.content_type = CONTENT_TYPE_QST;
	sync_qst_header.content_size = sizeof(sync_qst_t);
}

static void init_pid_files(int argc, char **argv)
{
	// Process PID save file
	if (server_iam && server_too) {
		process_update_pid(&proc_data_mir);

		if (process_pid_file_save(&proc_data_srv)) {
			if (mirror_too) {
				_error("while creating server PID file, waiting the mirror");
			} else {
				_error("while creating server PID file");
			}
		}
	} else if (mirror_iam && mirror_too)
	{
		process_update_pid(&proc_data_mir);

		if (process_pid_file_save(&proc_data_mir)) {
			_error("while creating mirror PID file");
		}
	}
}

static void init_output_redirection(int argc, char **argv, cluster_conf_t *conf_clus)
{
	int fd_output = -1;

	if (listening) {
		verbose_maslx("phase 7: listening (processing every %lu s)", time_insr);
	}

	if (conf_clus->db_manager.use_log) {
		fd_output = create_log(conf_clus->install.dir_temp, "eardbd");
	}
	if (fd_output < 0) {
		fd_output = verb_channel;
	}

	VERB_SET_FD(fd_output);
	WARN_SET_FD(fd_output);
	ERROR_SET_FD(fd_output);
	DEBUG_SET_FD(fd_output);
	TIMESTAMP_SET_EN(conf_clus->db_manager.use_log);
}

/*
 *
 *
 *
 */

void usage(int argc, char **argv) {}

int main(int argc, char **argv)
{
	//
	usage(argc, argv);

	while(!exitting)
	{
		//
		verbose_maslx("phase 1: general configuration");
		init_general_configuration(argc, argv, &conf_clus);

		//
		verbose_maslx("phase 2: time configuration");
		init_time_configuration(argc, argv, &conf_clus);

		//
		verbose_maslx("phase 3: sockets initialization");
		init_sockets(argc, argv, &conf_clus);

		//
		verbose_maslx("phase 4: processes fork");
		init_fork(argc, argv, &conf_clus);

		/*
		 * Post fork
		 */

		// Initializing signal handler
		init_signals();

		//
		verbose_maslx("phase 6: process configuration & allocation");
		init_process_configuration(argc, argv, &conf_clus);

		// Creating PID files
		init_pid_files(argc, argv);

		// Output file
		init_output_redirection(argc, argv, &conf_clus);

		/*
 		 * Running
 		 */

		//
		body();

		//
		release();

		//
		dream();
	}

	verbose_maslx("Bye");

	return 0;
}
