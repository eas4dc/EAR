/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/system/plugin_manager.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>  // memset

static void handle_signal(int sig, siginfo_t *si, void *unused)
{
    (void)si;    // Suppress unused parameter warning
    (void)unused; // Suppress unused parameter warning

    verbose(0,"Received signal %d", sig);
    plugin_manager_close();
}

int main(int argc, char *argv[])
{
    printf("%s: Started....\n", argv[0]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handle_signal;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        error("Failed to set SIGINT handler");
        return 1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        error("Failed to set SIGTERM handler");
        return 1;
    }

    /** Change the open file limit */
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    debug("current limit %lu max %lu", rl.rlim_cur, rl.rlim_max);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rl);

    plugin_manager_main(argc, argv);
    plugin_manager_wait();
    return 0;
}
