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

// Mirroring
extern pid_t server_pid;
extern pid_t mirror_pid;
extern pid_t others_pid;
extern int master_iam; // Master is who speaks
extern int server_iam;
extern int mirror_iam;
extern int server_too;
extern int mirror_too;

// State machine
extern int reconfiguring;
extern int listening;
extern int releasing;
extern int exitting;
extern int updating;
extern int dreaming;
extern int veteran;
extern int forked;

// Strings
extern char *str_who[2];
extern int verbosity;

/*
 *
 *
 *
 */

void signal_handler(int signal, siginfo_t *info, void *context)
{
	int propagating = 0;
	int waiting = 0;

	if (signal == SIGUSR1)
	{
		verbosity = !verbosity;
		updating  = 1;

		verbose_xaxxw("signal SIGUSR1 received, switching verbosity to '%d'", verbosity);
	}

	if (signal == SIGUSR2)
	{
		verbosity = (verbosity != 2) * 2;
		updating  = 1;

		verbose_xaxxw("signal SIGUSR2 received, switching verbosity to '%d'", verbosity);
	}

	// Case exit
	if ((signal == SIGTERM || signal == SIGINT) && !exitting)
	{
		verbose_xxxxw("signal SIGTERM/SIGINT received, exitting");

		propagating   = others_pid > 0 && info->si_pid != others_pid;
		//waiting     = others_pid > 0 && server_iam;
		listening     = 0;
		releasing     = 1;
		dreaming      = 0;
		exitting      = 1;
	}

	// Case reconfigure
	if (signal == SIGHUP && !reconfiguring)
	{
		verbose_xxxxw("signal SIGHUP received, reconfiguring");

		propagating   = others_pid > 0 && info->si_pid != others_pid;
		//waiting       = others_pid > 0 && server_iam;
		listening     = 0;
		reconfiguring = server_iam;
		releasing     = 1;
		dreaming      = 0;
		exitting      = mirror_iam;
	}

	if (signal == SIGCHLD)
	{
		verbose_xxxxw("signal SIGCHLD received");

		updating   = 1;
		waiting    = server_iam & (others_pid > 0);
		dreaming   = 0;
	}

	// Propagate signals
	if (propagating) {
		kill(others_pid, signal);
	}

	// Wait for the children (mirror)
	if (waiting)
	{
		verbose_xxxxw("waiting 1");
		waitpid(mirror_pid, NULL, 0);
		verbose_xxxxw("waiting 2");

		others_pid = 0;
		mirror_pid = 0;
		mirror_too = 0;
	}
}

void error_handler()
{
	// Terminate inmediately
	if (!forked)
	{
		release();
		exit(1);
	}

	// If I'am the server, inform and dream before exit
	// If I'am the mirror, just exit
	listening     = 0;
	releasing     = 1;
	dreaming      = server_iam & (others_pid > 0);
	exitting      = 1;
}
