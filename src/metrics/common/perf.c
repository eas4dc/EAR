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

#include <asm/unistd.h>
#include <common/output/debug.h>
#include <common/string_enhanced.h>
#include <common/system/file.h>
#include <common/system/folder.h>
#include <common/utils/string.h>
#include <errno.h>
#include <metrics/common/file.h>
#include <metrics/common/perf.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

// Future TO-DO:
// - Add a control and static manager for already open events (duplicates are
//   bad because there are limited counters in each CPU).
// - Convert to thead safe class (what happens when reading the same FD?).

static int test_paranoid()
{
    static int paranoid_checked = 0;
    static int paranoid_invalid = 0;
    char buffer[32];

    if (!paranoid_checked) {
        // Checking if the paranoid is valid
        if (filemagic_once_read("/proc/sys/kernel/perf_event_paranoid", buffer, sizeof(buffer))) {
            paranoid_invalid = (atoi(buffer) > 2 && getuid() > 0);
        }
        paranoid_checked = 1;
    }
    if (paranoid_invalid) {
        return 0;
    }
    return 1;
}

state_t perf_open(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event)
{
    return perf_open_cpu(perf, group, pid, type, event, 0, -1);
}

state_t perf_open_cpu(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event, uint options, int cpu)
{
    int gp_flag = 0;
    int gp_fd   = -1;

    // Cleaning
    memset(perf, 0, sizeof(perf_t));
    perf->fd = -1;
    if (!test_paranoid()) {
        return_msg(EAR_ERROR, "perf_event_paranoid value is not valid");
    }
    if (group != NULL) {
        if (perf != group) {
            gp_fd = group->fd;
        } else {
        }
        perf->group = group;
        gp_flag     = PERF_FORMAT_GROUP;
    }
    perf->attr.type   = type;
    perf->attr.size   = sizeof(struct perf_event_attr);
    perf->attr.config = event;
    if (cpu < 0) {
        perf->attr.exclusive = options;
        perf->attr.disabled  = 1;
#if ESTIMATE_PERF == 0
        perf->attr.inherit = 1;
#if PERF_ATTR_SIZE_VER6
        perf->attr.inherit_thread = 1;
#endif
#endif
        perf->attr.exclude_kernel = 1;
        perf->attr.exclude_hv     = 1;
    }
    perf->attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | gp_flag;
    perf->fd               = syscall(__NR_perf_event_open, &perf->attr, pid, cpu, gp_fd, 0);
    // Used in some cases
    perf->scale = 1.0;

#ifdef SHOW_DEBUGS
    debug("input: event 0x%lx, group %p and fd %d type %u size %u cpu %d group fd %d", event, group, perf->fd,
          perf->attr.type, perf->attr.size, cpu, gp_fd);
    if (perf->fd == -1) {
        debug("error (no: %d, str: %s)", errno, strerror(errno));
    } else {
        debug("ok");
    }
#endif
    if (perf->fd == -1) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t perf_close(perf_t *perf)
{
    if (perf->fd >= 0) {
        close(perf->fd);
    }
    memset(perf, 0, sizeof(perf_t));
    return EAR_SUCCESS;
}

state_t perf_reset(perf_t *perf)
{
    int gp_flag = 0;
    int ret;
    if (perf->group != NULL) {
        gp_flag = PERF_IOC_FLAG_GROUP;
    }
    ret = ioctl(perf->fd, PERF_EVENT_IOC_RESET, gp_flag);
#ifdef SHOW_DEBUGS
    if (ret == -1) {
        debug("ioctl of fd %d returned %d (no: %d, s: %s)", perf->fd, ret, errno, strerror(errno));
    } else {
        debug("ok fd %d and group %p", perf->fd, perf->group);
    }
#else
    (void) (ret);
#endif
    if (perf->fd == -1) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t perf_start(perf_t *perf)
{
    int gp_flag = 0;
    int ret;
    if (perf->group != NULL) {
        gp_flag = PERF_IOC_FLAG_GROUP;
    }
    ret = ioctl(perf->fd, PERF_EVENT_IOC_ENABLE, gp_flag);
#ifdef SHOW_DEBUGS
    if (ret == -1) {
        debug("ioctl of fd %d returned %d (no: %d, s: %s)", perf->fd, ret, errno, strerror(errno));
    } else {
        debug("ok fd %d and group %p", perf->fd, perf->group);
    }
#else
    (void) (ret);
#endif
    if (perf->fd == -1) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t perf_stop(perf_t *perf)
{
    int gp_flag = 0;
    int ret;
    if (perf->group != NULL) {
        gp_flag = PERF_IOC_FLAG_GROUP;
    }
    ret = ioctl(perf->fd, PERF_EVENT_IOC_DISABLE, gp_flag);
#ifdef SHOW_DEBUGS
    if (ret == -1) {
        debug("ioctl of fd %d returned %d (no: %d, s: %s)", perf->fd, ret, errno, strerror(errno));
    } else {
        debug("ok fd %d and group %p", perf->fd, perf->group);
    }
#else
    (void) (ret);
#endif
    if (perf->fd == -1) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t perf_read(perf_t *perf, llong *value)
{
    struct read_format_s {
        ullong nrval;
        ullong time_enabled;
        ullong time_running;
        ullong values[8];
    } value_s;

    double time_act = 0.0;
    double time_run = 0.0;
    double time_mul = 0.0;
    double value_d;
    double total_d;
    int ret;
    int i;

    memset(&value_s, 0, sizeof(struct read_format_s));
    ret = read(perf->fd, &value_s, sizeof(struct read_format_s));
    debug("PERF read val/ret/err: %llu %d %d", value_s.nrval, ret, errno);
    //
    if (ret == -1) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    if (value_s.time_running > 0) {
        time_act = (double) value_s.time_enabled;
        time_run = (double) value_s.time_running;
        time_mul = time_act / time_run;
    }
    debug("PERF time act/run/mul: %0.2lf %0.2lf %0.2lf (group %p)", time_act, time_run, time_mul, perf->group);

    if (perf->group == NULL) {
        value_d = (double) value_s.nrval;
        total_d = value_d * time_mul;
        *value  = (llong) total_d;
        debug("PERF final value     : %lld", *value);
    } else
        for (i = 0; i < value_s.nrval && i < 8; ++i) {
            value_d  = (double) value_s.values[i];
            total_d  = value_d * time_mul;
            value[i] = (llong) total_d;
        }
    return EAR_SUCCESS;
}

static char *point_value(char *string, char *key)
{
    char *p1 = NULL;
    if ((p1 = strstr(string, key)) != NULL) {
        p1 += strlen(key);
    }
    return p1;
}

static char *parse_value(char *buffer, char *key)
{
    static char buffer_ret[512];
    char *p1, *p2;
    //  i.e: buffer: event=0xff,umask=0x10, key: event=
    if ((p1 = point_value(buffer, key)) == NULL) {
        buffer_ret[0] = '0';
        buffer_ret[1] = '\0';
        return buffer_ret;
    }
    // Pointer to value correctly, i.e: p1: event=[0]xff,umask=0x10, p2: 0xff[?]
    p2 = buffer_ret;
    while (*p1 != ',' && *p1 != '-' && *p1 != '\n' && *p1 != '\0' && *p1 != EOF) {
        *p2 = *p1;
        ++p1;
        ++p2;
    }
    *p2 = '\0';
    // debug("buffer_ret: %s", buffer_ret);
    return buffer_ret;
}

static ulong parse_config(char *buffer, ulong value)
{
    uint l1_count;
    uint i1;
    uint l2_count;
    char **l1;
    uint *l2;
    ulong final = 0LU;
    ulong mask;
    char *p1;

    if ((p1 = point_value(buffer, "config:")) == NULL) {
        return 0;
    }
    if (strtoa(p1, ',', &l1, &l1_count) == NULL) {
        return 0;
    }
    for (i1 = 0; i1 < l1_count; ++i1) {
        ulong b_left  = 0LU;
        ulong b_right = 0LU;
        if (strtoat(l1[i1], '-', (void **) &l2, &l2_count, ID_UINT) == NULL) {
            continue;
        }
        if (l2_count > 0) {
            b_right = l2[0];
            b_left  = b_right;
        }
        if (l2_count > 1) {
            b_left = l2[1];
        }
        if (b_left >= b_right) {
            // Getting value
            mask  = (((0x01LU << (b_left - b_right + 1LU)) - 1LU) << b_right);
            final = final | ((value << b_right) & mask);
            debug("rank[%d]: %s, mask 0x%lx, value 0x%lx (%lu), final 0x%lx", i1, l1[i1], mask, value, value, final);
        }
        free(l2);
    }
    strtoa_free(l1);
    return final;
}

static uint parse_cpumask(char *buffer, uint **cpus)
{
    char **l1       = NULL;
    uint *l2        = NULL;
    uint cpus_count = 0;
    uint l1_count   = 0;
    uint l2_count   = 0;
    uint iG         = 0; // iGlobal
    uint iC         = 0; // iCPU
    uint i1         = 0;

    *cpus = NULL;
    if (strtoa(buffer, ',', &l1, &l1_count) == NULL) {
        return 0;
    }
    for (; iG < 2; ++iG) {
        for (i1 = 0; i1 < l1_count; ++i1) {
            strtoat(l1[i1], '-', (void **) &l2, &l2_count, ID_UINT);
            if (l2_count == 2) {
                uint min = l2[0];
                uint max = l2[1];
                if (min < max) {
                    if (*cpus != NULL) {
                        while (min <= max) {
                            debug("CPU%u: %u", iC, min);
                            (*cpus)[iC] = min;
                            ++min;
                            ++iC;
                        }
                    } else {
                        cpus_count += max - min + 1;
                    }
                }
            }
            if (l2_count == 1) {
                uint val = l2[0];
                if (*cpus != NULL) {
                    debug("CPU[%u]: %u", iC, val);
                    (*cpus)[iC] = val;
                    ++iC;
                } else {
                    cpus_count += 1;
                }
            }
            free(l2);
        }
        if (*cpus == NULL) {
            *cpus = calloc(cpus_count, sizeof(uint));
        }
    } // for iG
    debug("#CPUS: %u", cpus_count);
    strtoa_free(l1);
    return cpus_count;
}

static state_t _perf_open_files(perf_t **perfs, char *pmu_name, char *ev_name, uint *perfs_count)
{
    char buffer[512];
    char path[512];
    char unit[16];
    int no_cpu      = -1; // No CPU
    int *cpus       = &no_cpu;
    uint cpus_count = 1;
    int cpu         = 0;
    pid_t pid       = 0; // Our process
    ulong config    = 0;
    ulong event     = 0;
    ulong umask     = 0;
    double scale    = 1.0;
    uint type       = 0;
    DIR *dir        = NULL;

    *perfs = NULL;
    //
    if (pmu_name == NULL || ev_name == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    // If folder exists
    sprintf(path, "/sys/bus/event_source/devices/%s", pmu_name);
    if ((dir = opendir(path)) == NULL) {
        return_msg(EAR_ERROR, Generr.not_found);
    }
    closedir(dir);
    // Getting cpumask (i.e: 0-79(80) or 0,40(2)
    sprintf(path, "/sys/bus/event_source/devices/%s/cpumask", pmu_name);
    // Reading cpumask format file (this field is used to know the number of perfs to open)
    if (state_ok(ear_file_read(path, memset(buffer, 0, sizeof(buffer)), sizeof(buffer), 0))) {
        if ((cpus_count = parse_cpumask(buffer, (uint **) &cpus)) > 0) {
            // If ok, we are going per CPU, not process
            pid = -1;
        }
    }
    debug("allocating %d CPUs", cpus_count);
    *perfs = calloc(cpus_count, sizeof(perf_t));
    //
    sprintf(unit, "?");
    // If the event is not a number
    if (strncmp(ev_name, "0x", 2) != 0) {
        // Getting event (i.e: 'event=0xff,umask=0x10')
        sprintf(path, "/sys/bus/event_source/devices/%s/events/%s", pmu_name, ev_name);
        // Reading event file
        debug("%s reading event file", path);
        if (state_fail(ear_file_read(path, memset(buffer, 0, sizeof(buffer)), sizeof(buffer), 0))) {
            return EAR_ERROR;
        }
        event = (ulong) strtol(parse_value(buffer, "event="), NULL, 16);
        umask = (ulong) strtol(parse_value(buffer, "umask="), NULL, 16);
        // Getting event scale (i.e: 'event=0xff,umask=0x10')
        sprintf(path, "/sys/bus/event_source/devices/%s/events/%s.scale", pmu_name, ev_name);
        // Reading event file
        if (state_ok(ear_file_read(path, memset(buffer, 0, sizeof(buffer)), sizeof(buffer), 0))) {
            scale = atof(buffer);
        }
        // Getting event unit (i.e: 'event=0xff,umask=0x10')
        sprintf(path, "/sys/bus/event_source/devices/%s/events/%s.unit", pmu_name, ev_name);
        // Reading event file
        if (state_ok(ear_file_read(path, memset(unit, 0, sizeof(unit)), sizeof(unit), 0))) {
            strclean(unit, '\n');
        }
        // Getting event format (i.e: 'config:0-7')
        sprintf(path, "/sys/bus/event_source/devices/%s/format/event", pmu_name);
        // Reading event format file
        if (state_ok(ear_file_read(path, memset(buffer, 0, sizeof(buffer)), sizeof(buffer), 0))) {
            config = config | parse_config(buffer, event);
        }
        // Getting umask format (i.e: 'config:8-15,32-55')
        sprintf(path, "/sys/bus/event_source/devices/%s/format/umask", pmu_name);
        // Reading umask format file
        if (state_ok(ear_file_read(path, memset(buffer, 0, sizeof(buffer)), sizeof(buffer), 0))) {
            config = config | parse_config(buffer, umask);
        }
    } else {
        config = (ulong) atoull(ev_name);
    }
    // Getting type (i.e: '17')
    sprintf(path, "/sys/bus/event_source/devices/%s/type", pmu_name);
    // Reading umask format file
    if (state_ok(ear_file_read(path, memset(buffer, 0, sizeof(buffer)), sizeof(buffer), 0))) {
        type = (uint) atoi(buffer);
    }
    debug("event  : '0x%02lx'", event);
    debug("umask  : '0x%02lx'", umask);
    debug("config : '0x%02lx'", config);
    debug("type   : '0x%02x' ", type);
    debug("pid    : '%d'     ", pid);
    debug("scale  : '%lf'    ", scale);
    debug("unit   : '%s'     ", unit);
    debug("#cpus  : '%d'     ", cpus_count);
    debug("cpu0   : '%d'     ", cpus[0]);
    for (cpu = 0; cpu < cpus_count; ++cpu) {
        if (state_fail(perf_open_cpu(&((*perfs)[cpu]), NULL, pid, type, config, 0, cpus[cpu]))) {
            debug("perf_open_cpu returned: %s", state_msg);
            cpus_count -= 1;
            cpu -= 1;
        } else {
            strcpy((*perfs)[cpu].event_name, ev_name);
            strcpy((*perfs)[cpu].pmu_name, pmu_name);
            strcpy((*perfs)[cpu].unit, unit);
            (*perfs)[cpu].scale = scale;
        }
    }
    if (perfs_count != NULL) {
        *perfs_count = cpus_count;
    }
    if (pid == -1 && cpus != NULL && cpus[0] != no_cpu) {
        free(cpus);
    }
    if (cpus_count == 0) {
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

state_t perf_open_files(perf_t **_perfs, char *_pmu_name, char *_ev_name, uint *_perfs_count)
{
    char pmu_name[128];
    char *ev_list_fake[2];
    perf_t *perfs     = NULL; // Perfs accumulation
    uint perfs_count  = 0;
    perf_t *allocs    = NULL;
    uint allocs_count = 0;
    int is_multi_pmu  = 0;
    int is_multi_ev   = 0;
    int i             = 0; // pmu iterator
    int j             = 0; // ev  iterator
    char **ev_list    = NULL;
    char *ev_name     = NULL;
    state_t s         = EAR_SUCCESS;

    *_perfs_count = 0;
    // Is multi depends if a '%d' is found
    is_multi_pmu = strstr(_pmu_name, "%d") != NULL;
    is_multi_ev  = strchr(_ev_name, ',') != NULL;
    //
    ev_list_fake[0] = _ev_name;
    ev_list_fake[1] = NULL;
    //
    if (strtoa(_ev_name, ',', &ev_list, NULL) == NULL) {
        ev_list = ev_list_fake;
    }
    do {
        // Converting pmu_name_%d into pmu_name_0, pmu_name_1...
        sprintf(pmu_name, _pmu_name, i++);
        // Setting event pointer to 0
        j = 0;
        do {
            ev_name = ev_list[j++];
            // Calling perf section
            perfs       = NULL;
            perfs_count = 0;
            debug("%s:%s trying to open", pmu_name, ev_name);
            // Calling original perf_open_files
            if (state_ok(s = _perf_open_files(&perfs, pmu_name, ev_name, &perfs_count))) {
                // Counting the accumulation of perfs
                allocs_count += perfs_count;
                debug("%s returned %d/%d perfs (%p)", pmu_name, perfs_count, allocs_count, perfs);
                // Allocating more space if required
                allocs = realloc(allocs, allocs_count * sizeof(perf_t));
                // Copying the newly received perfs into the accumulation
                memcpy(&allocs[allocs_count - perfs_count], perfs, perfs_count * sizeof(perf_t));
                // Freeing the memory because we already have it copied.
                free(perfs);
            }
        } while (is_multi_ev && state_ok(s));
    } while (is_multi_pmu && (state_ok(s) || (state_fail(s) && j > 1)));
    //
    if (allocs_count == 0) {
        return EAR_ERROR;
    }
    *_perfs       = allocs;
    *_perfs_count = allocs_count;
    return EAR_SUCCESS;
}

void perf_set_scale(perf_t *perfs, uint perfs_count, char *ev_name, double scale)
{
    int i;
    for (i = 0; i < perfs_count; ++i) {
        if (strcmp(perfs[i].event_name, ev_name) == 0) {
            perfs[i].scale = scale;
        }
    }
}
