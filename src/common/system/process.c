/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/sizes.h>
#include <common/system/file.h>
#include <common/system/process.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void process_data_initialize(process_data_t *prodata, char *name, char *path_pid)
{
    sprintf(prodata->path_pid, "%s/%s.pid", path_pid, name);
    strcpy(prodata->name, name);
    process_update_pid(prodata);
}

void process_update_pid(process_data_t *prodata)
{
    prodata->pid = getpid();
}

int process_exists(const process_data_t *prodata, char *bin_name, pid_t *pid)
{
    int value = 0;
    state_t state;

    //
    state = process_pid_file_load(prodata, pid);

    if (state_fail(state)) {
        return 0;
    }

    //
    value = !((kill(*pid, 0) < 0) && (errno == ESRCH));

    if (value) {
        char *buffer1 = malloc(SZ_PATH);
        char *buffer2 = malloc(SZ_PATH);
        char *p;

        sprintf(buffer1, "/proc/%d/cmdline", *pid);
        ear_file_read(buffer1, buffer2, SZ_PATH, 0);
        p = strstr(buffer2, bin_name);

        free(buffer1);
        free(buffer2);

        return (p != NULL);
    }

    return 0;
}

state_t process_pid_file_save(const process_data_t *prodata)
{
    char buffer[SZ_NAME_SHORT];
    state_t state;

    sprintf(buffer, "%d\n", prodata->pid);
    state = ear_file_write(prodata->path_pid, buffer, strlen(buffer));

    if (state_fail(state)) {
        return state;
    }

    state_return(EAR_SUCCESS);
}

state_t process_pid_file_load(const process_data_t *prodata, pid_t *pid)
{
    char buffer[SZ_NAME_SHORT];
    state_t state;

    state = ear_file_read(prodata->path_pid, buffer, SZ_NAME_SHORT, 0);

    if (state_fail(state)) {
        state_return(state);
    }

    *pid = (pid_t) atoi(buffer);
    state_return(EAR_SUCCESS);
}

state_t process_pid_file_clean(process_data_t *prodata)
{
    ear_file_clean(prodata->path_pid);
    state_return(EAR_SUCCESS);
}
