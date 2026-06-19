/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
// #define SHOW_DEBUGS 1
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/io/io.h>
#include <metrics/io/archs/dummy.h>
#include <metrics/io/archs/proc_file.h>

static uint      devs_count;
static pid_t    *pids_job;
static FILE    **fds_job;
static FILE     *fd_node;
static uint      scope;
static uint      granularity;

static state_t pid_add(pid_t pid)
{
    char buffer[32];
    FILE *fd;
    int i;

    // PID control, if already inserted
    for (i = 0; i < devs_count; ++i) {
        if (pids_job[i] == pid) {
            return_msg(EAR_ERROR, "PID already added");
        }
    }
    // PID control, looking for a place
    for (i = 0; i < devs_count; ++i) {
        if (pids_job[i] == 0) {
            break;
        }
    }
    if (i >= devs_count) {
        return_msg(EAR_ERROR, "max number of PIDs reached")
    }
    sprintf(buffer, "/proc/%d/io", pid);
    if ((fd = fopen(buffer, "r")) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    fds_job[i]  = fd;
    pids_job[i] = pid;
    return EAR_SUCCESS;
}

IO_F_LOAD(proc_file)
{
    scope = options & SCOPE_MASK;
    if (scope == SCOPE_NODE) {
        devs_count  = 1;
        granularity = GRANULARITY_NODE;
        if ((fd_node = fopen("/proc/diskstats", "r")) == NULL) {
            return;
        }
    } else if (scope == SCOPE_JOB) {
        devs_count  = tp->cpu_count * 3;
        scope       = SCOPE_JOB;
        granularity = GRANULARITY_PROCESS;
        fds_job     = (FILE **) calloc(devs_count, sizeof(FILE *));
        pids_job    = (pid_t *) calloc(devs_count, sizeof(pid_t));
    } else {
        devs_count  = 1;
        scope       = SCOPE_PROCESS;
        granularity = GRANULARITY_PROCESS;
        fds_job     = (FILE **) calloc(devs_count, sizeof(FILE *));
        pids_job    = (pid_t *) calloc(devs_count, sizeof(pid_t));
        pid_add(getpid());
    }
    // Filling each gap
    apis_put(ops->unload    , io_proc_file_unload);
    apis_put(ops->update   ,  io_proc_file_update);
    apis_put(ops->get_info  , io_proc_file_get_info);
    apis_put(ops->read      , io_proc_file_read);
}

static state_t pid_remove(int i)
{
    if (pids_job[i] > 0) {
        fclose(fds_job[i]);
        fds_job[i] = NULL;
        pids_job[i] = 0;
    }
    return EAR_SUCCESS;
}

static state_t pids_clean()
{
    int i;
    for(i = 0; i < devs_count; ++i) {
        pid_remove(i);
    }
    return EAR_SUCCESS;
}

IO_F_UNLOAD(proc_file)
{
    pids_clean();
    if (fd_node != NULL) {
        fclose(fd_node);
        fd_node = NULL;
    }
    if (pids_job != NULL) {
        free(pids_job);
        pids_job = NULL;
    }
    if (fds_job != NULL) {
        free(fds_job);
        fds_job = NULL;
    }
}

IO_F_UPDATE(proc_file)
{
    pid_t pid = (uint) ((ullong) value);
    int i;

    if (scope == SCOPE_JOB && option == UPD_PID_ADD) {
        return pid_add(pid);
    } else if (scope == SCOPE_JOB && option == UPD_PID_REMOVE) {
        for (i = 0; i < devs_count; ++i) {
            if (pids_job[i] == pid) {
                return pid_remove(i);
            }
        }
        return_msg(EAR_ERROR, "PID not found");
    } else if(scope == SCOPE_JOB && option == UPD_PIDS_CLEAN) {
        return pids_clean();
    }
    return_msg(EAR_ERROR, "option not available");
}

IO_F_GET_INFO(proc_file)
{
    info->api         = API_FILE;
    info->scope       = scope;
    info->granularity = (scope == SCOPE_NODE) ? GRANULARITY_NODE: GRANULARITY_PROCESS;
    info->devs_count  = devs_count;
}

static void read_diskstats(char *line, io_t *io)
{
    ulong reads_completed, writes_completed;
    ulong sectors_read, sectors_written;
    uint major, minor;
    char dev[32];
    char *p;
    char *d;

    int n = sscanf(line, "%u %u %31s %lu %*u %lu %*u %lu %*u %lu",
       &major, &minor, dev, &reads_completed, &sectors_read, &writes_completed, &sectors_written);
    if (n < 7) { return; }
    if (strstr(line, "loop")) { return; }
    if (strstr(line, "ram" )) { return; }
    if (strstr(line, "fd"  )) { return; }
    if (strstr(line, "sr"  )) { return; }
    if (strstr(line, "zram")) { return; }
    // If a 'p' is found (from Partition) and following the 'p' there is a number
    // until the end of device name, is a partition and does not count.
    for (p = dev; *p; ++p) {
        for (d = p+1; *p == 'p' && *d != '\0' && isdigit(*d); ++d) {}
        if (*p == 'p' && d != p+1 && *d == '\0') {
            return;
        }
    }
    #if SHOW_DEBUGS
    dprintf(debug_channel, "line: %s", line);
    #endif
    io->rstor = sectors_read    * 512; // SECTOR_SIZE
    io->wstor = sectors_written * 512;
}

IO_F_READ(proc_file)
{
    timestamp_t time = {0};
    char line[256];
    char *str;
    int i;

    timestamp_get(&time);
    memset(io, 0, devs_count * sizeof(io_t));
    // SCOPE is NODE
    if (scope == SCOPE_NODE) {
        io[0].pid  = 1; // PID of the system
        io[0].time = time;
        fseek(fd_node, 0L, SEEK_SET);
        while (fgets(line, sizeof(line), fd_node) != NULL) {
            read_diskstats(line, io);
        }
        return EAR_SUCCESS;
    }
    // SCOPE is PROCESS or JOB
    for (i = 0; i < devs_count; i++) {
        if (pids_job[i] == 0) {
            continue;
        }
        io[i].pid  = pids_job[i];
        io[i].time = time;
        fseek(fds_job[i], 0L, SEEK_SET);
        while (fgets(line, sizeof(line), fds_job[i]) != NULL) {
            if ((str = strstr(line, ": ")) != NULL) {
                #if SHOW_DEBUGS
                dprintf(debug_channel, "dev%d: line: %s", i, line);
                #endif
                if (!strncmp(line, "rchar"      ,  4)) { io[i].rchar = atoll(&str[2]); }
                if (!strncmp(line, "wchar"      ,  4)) { io[i].wchar = atoll(&str[2]); }
                if (!strncmp(line, "syscr"      ,  4)) { io[i].syscr = atoll(&str[2]); }
                if (!strncmp(line, "syscw"      ,  4)) { io[i].syscw = atoll(&str[2]); }
                if (!strncmp(line, "read_bytes" , 10)) { io[i].rstor = atoll(&str[2]); }
                if (!strncmp(line, "write_bytes", 11)) { io[i].wstor = atoll(&str[2]); }
                if (!strncmp(line, "cancelled_write_bytes", 21)) { io[i].cancelled = atoll(&str[2]); }
            }
        }
    }
    return EAR_SUCCESS;
}