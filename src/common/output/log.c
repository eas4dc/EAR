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
