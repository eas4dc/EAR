/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/shared_areas.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <daemon/power_monitor_app.h>

/******************** NODE JOB LIST **********************/

/* This is the job of jobs the EARD is aware of */
state_t get_joblist_path(char *tmp, char *path)
{
    if ((tmp == NULL) || (path == NULL))
        return EAR_ERROR;
    sprintf(path, "%s/.ear_joblist", tmp);
    return EAR_SUCCESS;
}

uint *create_joblist_shared_area(char *path, int *fd, uint *joblist, int joblist_elems)
{
    uint *sh_joblist;
    int fd_joblist;

    if (path == NULL)
        return NULL;
    mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    sh_joblist =
        (uint *) create_shared_area(path, perms, (char *) joblist, joblist_elems * sizeof(uint), &fd_joblist, 0);

    if (fd != NULL)
        *fd = fd_joblist;
    return sh_joblist;
}

uint *attach_joblist_shared_area(char *path, int *fd, int *size)
{
    int fd_joblist;
    uint *sh_joblist;
    sh_joblist = (uint *) attach_shared_area(path, 0, O_RDONLY, &fd_joblist, size);
    if (fd != NULL)
        *fd = fd_joblist;
    return sh_joblist;
}

void dettach_joblib_shared_area(int fd)
{
    dettach_shared_area(fd);
}

void joblist_shared_area_dispose(char *path, uint *mem, int joblist_elems, int fd)
{
    if (mem != NULL) {
        munmap(mem, joblist_elems * sizeof(uint));
    }

    dispose_shared_area(path, fd);
}

/* This is the pmapp are for a given job context */
state_t get_jobmon_path(char *tmp, uint ID, char *path)
{
    if ((tmp == NULL) || (path == NULL))
        return EAR_ERROR;
    sprintf(path, "%s/%u/.powermon_app", tmp, ID);
    return EAR_SUCCESS;
}

powermon_app_t *create_jobmon_shared_area(char *path, powermon_app_t *pmapp, int *fd)
{
    if (path == NULL)
        return NULL;

    int fd_pmapp;
    mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    powermon_app_t *sh_pmapp =
        (powermon_app_t *) create_shared_area(path, perms, (char *) pmapp, sizeof(powermon_app_t), &fd_pmapp, 0);

    /* We don't the fd when we create the shared area. So the current fd
     * is 0. It is needed to update it. */
    if (sh_pmapp) {
        sh_pmapp->fd_shared_areas[SELF] = fd_pmapp;
    }

    if (fd != NULL)
        *fd = fd_pmapp;
    return sh_pmapp;
}

powermon_app_t *attach_jobmon_shared_area(char *path, int *fd)
{
    int fd_pmapp;
    powermon_app_t *sh_pmapp;
    sh_pmapp = (powermon_app_t *) attach_shared_area(path, sizeof(powermon_app_t), O_RDWR, &fd_pmapp, NULL);
    if (fd != NULL)
        *fd = fd_pmapp;
    return sh_pmapp;
}

void dettach_jobmon_shared_area(int fd)
{
    dettach_shared_area(fd);
}

void jobmon_shared_area_dispose(char *path, powermon_app_t *mem, int fd)
{
    if (mem != NULL) {
        munmap(mem, sizeof(powermon_app_t));
    }

    dispose_shared_area(path, fd);
}
