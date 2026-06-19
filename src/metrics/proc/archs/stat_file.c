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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <common/system/time.h>
#include <metrics/proc/archs/stat_file.h>

static uint      devs_count;
static pid_t    *pids_job;
static FILE    **fds_job;
static FILE     *fd_node;
static uint      scope;
static uint      granularity;
static long      clk_sec;
static uint      kernel;

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
    sprintf(buffer, "/proc/%d/stat", pid);
    if ((fd = fopen(buffer, "r")) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    fds_job[i]  = fd;
    pids_job[i] = pid;
    // This forces the fscanf re-reads the special proc-file 'stat'.
    setvbuf(fds_job[i], NULL, _IONBF, 0);
    return EAR_SUCCESS;
}

static int kernel_version(uint *kernel, uint *major, uint *minor, uint *patch)
{
    struct utsname buffer;
    uint ver[4];
    char *p;
    int i;
    if (uname(&buffer) != 0) {
        return_msg(0, strerror(errno));
    }
    p = buffer.release;
    i = 0;
    //
    while (*p && i < 4) {
        if (isdigit(*p)) {
            ver[i] = (uint) strtol(p, &p, 10);
            i++;
        } else {
            p++;
        }
    }
    *kernel = ver[0], *major  = ver[1];
    *minor  = ver[2], *patch  = ver[3];
    return 1;
}

PROC_F_LOAD(stat_file)
{
    uint kernel_no;
    uint major;
    uint minor;
    uint patch;

    // Already loaded
    if (kernel > 0) {
        return;
    }
    if (!kernel_version(&kernel_no, &major, &minor, &patch)) {
        return;
    }
    if ((clk_sec = sysconf(_SC_CLK_TCK)) < 0) {
        clk_sec = 100; // Using default value
    }
    debug("Detected kernel %u.%u.%u at @%ld", kernel_no, major, minor, clk_sec);
    if ((kernel_no  < 3) || (kernel_no == 3 && major < 3)) kernel = 1; // 2.6.24
    if ((kernel_no == 3  && major > 2)) kernel = 2; // 3.3
    if ((kernel_no  > 3  && major > 3)) kernel = 3; // > 3.5
    if ((kernel_no  > 4              )) kernel = 3; // > 4.x
    if (kernel == 0) {
        return_msg(, "Incompatble kernel version");
    }
    scope = options & SCOPE_MASK;
    if (scope == SCOPE_NODE) {
        devs_count   = tp->cpu_count;
        granularity  = GRANULARITY_CPU;
        if ((fd_node = fopen("/proc/stat", "r")) == NULL) {
            return;
        }
        setvbuf(fd_node, NULL, _IONBF, 0);
    } else if (scope == SCOPE_JOB) {
        devs_count  = tp->cpu_count * 3;
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
    apis_put(ops->unload    , proc_stat_file_unload  );
    apis_put(ops->update    , proc_stat_file_update  );
    apis_put(ops->get_info  , proc_stat_file_get_info);
    apis_put(ops->read      , proc_stat_file_read    );
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

PROC_F_UNLOAD(stat_file)
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

PROC_F_UPDATE(stat_file)
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

PROC_F_GET_INFO(stat_file)
{
    info->api         = API_FILE;
    info->scope       = scope;
    info->granularity = granularity;
    info->devs_count  = devs_count;
}

static state_t read_procstat(proc_t *pr, timestamp_t *time)
{
    struct {
        char name[16];
        int64_t user;
        int64_t nice;
        int64_t system;
        int64_t idle;
        int64_t iowait;
        int64_t irq;
        int64_t softirq;
        int64_t steal;
        int64_t guest;
        int64_t guest_nice;
    } stat;
    char line[256];
    int i = 0;

    fseek(fd_node, 0L, SEEK_SET);
    while (fgets(line, sizeof(line), fd_node) != NULL) {
        // The CPU lines are always the first
        if (strncmp(line, "cpu ", 4) == 0) { continue; }
        if (strncmp(line, "cpu" , 3) != 0) { break; }
        int parsed = sscanf(line,
            "%15s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
            stat.name, &stat.user   , &stat.nice , &stat.system, &stat.idle, &stat.iowait,
            &stat.irq, &stat.softirq, &stat.steal, &stat.guest , &stat.guest_nice);
        pr[i].pid    = 1;
        pr[i].time   = *time;
        pr[i].utime  = (double) (stat.user + stat.nice + stat.guest + stat.guest_nice);
        pr[i].stime  = (double) (stat.system + stat.irq + stat.softirq + stat.steal);
        pr[i].utime /= (double) clk_sec;
        pr[i].stime /= (double) clk_sec;
        debug("%s %0.2lf %0.2lf", stat.name, pr[i].utime, pr[i].stime);
        ++i;
        if (parsed < 5) {
            break;
        }
    }
    return EAR_SUCCESS;
}

PROC_F_READ(stat_file)
{
    char comm[256] = {0};
    ulong utime;
    ulong stime;
    char state;
    int pid;
    int ret;
    int i;

    // Cleaning
    memset(pr, 0, sizeof(proc_t)*devs_count);
    // To compute CPU usage
    timestamp_getfast(&pr[0].time);
    //
    if (scope == SCOPE_NODE) {
        return read_procstat(pr, &pr[0].time);
    }
    // Reading
    for (i = 0; i < devs_count; i++) {
        if (pids_job[i] == 0) {
            continue;
        }
        utime = 0LU;
        stime = 0LU;
        pr[i].pid  = pids_job[i];
        pr[i].time = pr[0].time;
        fseek(fds_job[i], 0L, SEEK_SET);
        ret = fscanf(fds_job[i], "%d (%[^)]) %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
               &pid, comm, &state, &utime, &stime);
        (void) ret;
        pr[i].utime = (double) utime / (double) clk_sec;
        pr[i].stime = (double) stime / (double) clk_sec;
        debug("pid: %d, utime: %lf %lf (clk %lu)",
            pid, pr[i].utime, pr[i].stime, clk_sec);
    }
    return EAR_SUCCESS;
}
