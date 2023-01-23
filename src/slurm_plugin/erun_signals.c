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
#include <string.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// 
extern plug_serialization_t sd;

// Buffers
extern char buffer[SZ_PATH];

//
extern char **_argv;
extern int    _argc;

//
extern int _sp;


void signals_handler(int signal, siginfo_t *info, void *context)
{
    plug_verbose(_sp, 2, "received signal '%s'", strsignal(signal));
    if ((signal == SIGTERM || signal == SIGINT || signal == SIGHUP)) {
    }
}

void signals()
{
    struct sigaction action;
    sigset_t sigset;

    action.sa_handler   = NULL;
    action.sa_sigaction = signals_handler;
    action.sa_flags     = SA_SIGINFO;

    sigfillset(&sigset);

    sigdelset(&sigset, SIGTERM);
    sigdelset(&sigset, SIGCHLD);
    sigdelset(&sigset, SIGINT);
    sigdelset(&sigset, SIGHUP);

    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGCHLD, &action, NULL);
    sigaction(SIGINT,  &action, NULL);
    sigaction(SIGHUP,  &action, NULL);
}