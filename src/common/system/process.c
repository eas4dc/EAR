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
#include <dirent.h>
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

static state_t read_proc_file(char *path, char *buffer, size_t size)
{
    int fd;
    ssize_t n;

    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        state_return(EAR_ERROR);
    }

    n = read(fd, buffer, size - 1);
    close(fd);

    if (n < 0) {
        state_return(EAR_ERROR);
    }

    buffer[n] = '\0';
    state_return(EAR_SUCCESS);
}

static state_t parse_task_stat(char *buffer, ulong *utime, ulong *stime)
{
    char *p, *endp;
    int field = 3;
    unsigned long long value;

    p = strrchr(buffer, ')');
    if (p == NULL) {
        state_return(EAR_ERROR);
    }

    p++;

    while (*p != '\0') {
        while (*p == ' ') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (field == 3) {
            p++;
        } else {
            value = strtoull(p, &endp, 10);
            if (endp == p) {
                state_return(EAR_ERROR);
            }

            if (field == 14) {
                *utime = (ulong) value;
            } else if (field == 15) {
                *stime = (ulong) value;
                state_return(EAR_SUCCESS);
            }

            p = endp;
        }

        field++;
    }

    state_return(EAR_ERROR);
}

static state_t read_task_cpu(pid_t pid, char *tid, ulong *utime, ulong *stime)
{
    char path[SZ_PATH];
    char buffer[4096];

    snprintf(path, sizeof(path), "/proc/%d/task/%s/stat", pid, tid);

    if (state_fail(read_proc_file(path, buffer, sizeof(buffer)))) {
        state_return(EAR_ERROR);
    }

    state_return(parse_task_stat(buffer, utime, stime));
}

static state_t read_process_cpu(pid_t pid, ulong *user_ticks, ulong *system_ticks, uint *threads)
{
    char path[SZ_PATH];
    DIR *dir;
    struct dirent *entry;
    ulong utime, stime;

    *user_ticks   = 0;
    *system_ticks = 0;
    *threads      = 0;

    snprintf(path, sizeof(path), "/proc/%d/task", pid);

    dir = opendir(path);
    if (dir == NULL) {
        state_return(EAR_ERROR);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        utime = 0;
        stime = 0;

        if (state_ok(read_task_cpu(pid, entry->d_name, &utime, &stime))) {
            *user_ticks += utime;
            *system_ticks += stime;
            (*threads)++;
        }
    }

    closedir(dir);

    if (*threads == 0) {
        state_return(EAR_ERROR);
    }

    state_return(EAR_SUCCESS);
}

static state_t read_process_memory(pid_t pid, ulong *vm_rss_kb, ulong *vm_size_kb)
{
    char path[SZ_PATH];
    char buffer[8192];
    char *line, *saveptr = NULL;
    unsigned long long value;

    *vm_rss_kb  = 0;
    *vm_size_kb = 0;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    if (state_fail(read_proc_file(path, buffer, sizeof(buffer)))) {
        state_return(EAR_ERROR);
    }

    line = strtok_r(buffer, "\n", &saveptr);
    while (line != NULL) {
        if (sscanf(line, "VmRSS: %llu kB", &value) == 1) {
            *vm_rss_kb = (ulong) value;
        } else if (sscanf(line, "VmSize: %llu kB", &value) == 1) {
            *vm_size_kb = (ulong) value;
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }

    state_return(EAR_SUCCESS);
}

static state_t count_process_fds(pid_t pid, uint *open_fds)
{
    char path[SZ_PATH];
    DIR *dir;
    struct dirent *entry;

    *open_fds = 0;

    snprintf(path, sizeof(path), "/proc/%d/fd", pid);

    dir = opendir(path);
    if (dir == NULL) {
        state_return(EAR_ERROR);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            (*open_fds)++;
        }
    }

    closedir(dir);
    state_return(EAR_SUCCESS);
}

state_t process_health_get(pid_t pid, process_health_t *health)
{
    long ticks_per_sec;
    ulong user_ticks, system_ticks;
    ulong ns_per_tick;

    if ((pid <= 0) || (health == NULL)) {
        state_return(EAR_BAD_ARGUMENT);
    }

    memset(health, 0, sizeof(process_health_t));
    health->pid = pid;

    ticks_per_sec = sysconf(_SC_CLK_TCK);
    if (ticks_per_sec <= 0) {
        state_return(EAR_ERROR);
    }

    if (state_fail(read_process_cpu(pid, &user_ticks, &system_ticks, &health->num_threads))) {
        state_return(EAR_ERROR);
    }

    ns_per_tick = 1000000000UL / (ulong) ticks_per_sec;

    health->cpu_user_ns   = user_ticks * ns_per_tick;
    health->cpu_system_ns = system_ticks * ns_per_tick;
    health->cpu_total_ns  = health->cpu_user_ns + health->cpu_system_ns;

    if (state_fail(read_process_memory(pid, &health->vm_rss_kb, &health->vm_size_kb))) {
        state_return(EAR_ERROR);
    }

    if (state_fail(count_process_fds(pid, &health->open_fds))) {
        state_return(EAR_ERROR);
    }

    state_return(EAR_SUCCESS);
}

/**
 *  typedef struct process_health {
    pid_t pid;
    ulong cpu_user_ns;
    ulong cpu_system_ns;
    ulong cpu_total_ns;
    uint num_threads;
    ulong vm_rss_kb;
    ulong vm_size_kb;
    uint open_fds;
} process_health_t;
*/

void process_health_print_fd(process_health_t *health, int fd)
{
    dprintf(fd,
            " PID %d CPU_user(ns) %lu CPU_sys(ns) %lu CPU_total(ns) %lu Th %u VM_RSS(KB) %lu VM_size(KB) %lu FDs %u\n",
            health->pid, health->cpu_user_ns, health->cpu_system_ns, health->cpu_total_ns, health->num_threads,
            health->vm_rss_kb, health->vm_size_kb, health->open_fds);
}
