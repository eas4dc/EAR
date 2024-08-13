/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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