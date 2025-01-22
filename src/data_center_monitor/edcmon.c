/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <common/system/plugin_manager.h>
#include <unistd.h>
#include <sys/resource.h>


int main(int argc, char *argv[])
{
    printf("%s: Started....\n", argv[0]);

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
