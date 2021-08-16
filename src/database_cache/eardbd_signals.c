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

#include <database_cache/eardbd.h>
#include <database_cache/eardbd_body.h>
#include <database_cache/eardbd_sync.h>
#include <database_cache/eardbd_signals.h>
#include <database_cache/eardbd_storage.h>

//
extern cluster_conf_t          conf_clus;
//
extern int                     master_iam; // Master is who speaks
extern int                     server_iam;
extern int                     mirror_iam;
//
extern int                     server_enabled;
extern int                     mirror_enabled;
extern pid_t                   server_pid;
extern pid_t                   mirror_pid;
extern pid_t                   others_pid;
//
extern int                     listening;
extern int                     releasing;
extern int                     exitting;
extern int                     updating;
extern int                     dreaming;
extern int                     veteran;
extern int                     forked;
extern int                     forced;
//
extern char                   *str_who[2];
extern int                     verbosity;

void log_handler(cluster_conf_t *conf_clus, uint close_previous)
{
	int fd_output = -1;

	if (close_previous) {
		close(fd_output);
	}
	// create_log also cleans old files
	if (conf_clus->db_manager.use_log) {
		if (server_iam) {
			fd_output = create_log(conf_clus->install.dir_temp, "eardbd.server");
		} else {
			fd_output = create_log(conf_clus->install.dir_temp, "eardbd.mirror");
		}
	}
	if (fd_output < 0) {
		return;
	}
	// Setting file descriptors
	VERB_SET_FD (fd_output);
	WARN_SET_FD (fd_output);
	ERROR_SET_FD(fd_output);
	DEBUG_SET_FD(fd_output);
	TIMESTAMP_SET_EN(conf_clus->db_manager.use_log);
}

void signal_handler(int signal, siginfo_t *info, void *context)
{
	int propagating = 0;
	int waiting = 0;

	// Verbose signal
	if (signal == SIGUSR1) {
		verb_who("signal SIGUSR1 received, switching verbosity to '%d'", verbosity);
		verbosity   = (verbosity != 2) * 2;
		updating    = 1;
	}
	// Log rotation signal
	if (signal == SIGUSR2) {
		verb_who_noarg("signal SIGUSR2 received, re-opening log file");
		log_handler(&conf_clus, 1);
		propagating = others_pid > 0 && info->si_pid != others_pid;
		updating    = 1;
	}
	// Case exit
	if ((signal == SIGTERM || signal == SIGINT || signal == SIGHUP) && !exitting) {
		verb_who_noarg("signal SIGTERM/SIGINT/SIGHUP received, exitting");
		propagating = others_pid > 0 && info->si_pid != others_pid;
		//waiting   = others_pid > 0 && server_iam;
		listening   = 0;
		releasing   = 1;
		dreaming    = 0;
		exitting    = 1;
	}
	if (signal == SIGCHLD) {
		verb_who_noarg("signal SIGCHLD received");
		updating    = 1;
		waiting     = server_iam & (others_pid > 0);
		dreaming    = 0;
		exitting    = 1;
	}
	// Propagate signals
	if (propagating) {
		kill(others_pid, signal);
	}
	// Wait for the children (mirror)
	if (waiting) {
		waitpid(mirror_pid, NULL, 0);
	}
}

void error_handler()
{
	// Terminate inmediately
	if (!forked) {
		release();
		exit(1);
	}
	// If I'am the server, inform and dream before exit
	// If I'am the mirror, just exit
	listening = 0;
	releasing = 1;
	dreaming  = server_iam & (others_pid > 0);
	exitting  = 1;
}
