/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/output/error.h>
#include <common/output/verbose.h>

int log_open(char *path, char *service_name)
{
        char logfile[1024];
        mode_t my_mask;
        int fd;

        sprintf(logfile, "%s/%s.log", path, service_name);
        my_mask = umask(0);
        unlink(logfile);
        fd = open(logfile, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        chmod(logfile, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        umask(my_mask);

        if (fd >= 0) {
                VERB_SET_FD (fd);
                WARN_SET_FD (fd);
                ERROR_SET_FD(fd);
                DEBUG_SET_FD(fd);
        } else {
                error("can't open file %s (%s)", logfile, strerror(errno));
        }

        return fd;
}
