/***************************************************************************
 * Copyright (c) 2026 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE
#include "cpufreq_fixture.h"
#include <common/hardware/cpuid.h>
#include <common/utils/keeper.h>
#include <fcntl.h>
#include <management/cpufreq/cpufreq_base.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* External inputs only. Production load/init creates its own descriptor arrays,
 * frequency tables, history, and result storage from these files. */
static struct {
    const char *name, *contents;
    int fd[TEST_CPUS];
} attributes[] = {
    {"cpuinfo_max_freq", "3000000\n", {-1, -1}},
    {"cpuinfo_min_freq", "1800000\n", {-1, -1}},
    {"scaling_min_freq", "1800000\n", {-1, -1}},
    {"scaling_max_freq", "3000000\n", {-1, -1}},
    {"scaling_setspeed", "3000000\n", {-1, -1}},
    {"scaling_governor", "performance\n", {-1, -1}},
    {"scaling_available_governors", "userspace performance powersave\n", {-1, -1}},
    {"scaling_available_frequencies", "3000000 2400000 1800000\n", {-1, -1}},
};

static char directory[] = "/tmp/ear-cpufreq-XXXXXX";
static int opened[64], opened_count;

int __real_open(const char *path, int flags, ...);
int __real_access(const char *path, int mode);
int __real_close(int fd);
ssize_t __real_pwrite(int fd, const void *buffer, size_t count, off_t offset);

ssize_t __wrap_pwrite(int fd, const void *buffer, size_t count, off_t offset)
{
    ssize_t written = __real_pwrite(fd, buffer, count, offset);
    for (int i = 0; i < opened_count; ++i) {
        if (opened[i] == fd) {
            /* A sysfs write replaces the attribute, including shorter values. */
            if (written > 0 && ftruncate(fd, offset + written) != 0) {
                return -1;
            }
            break;
        }
    }
    return written;
}

static void attribute_path(char *path, size_t size, int attribute, int cpu)
{
    snprintf(path, size, "%s/cpu%d_%s", directory, cpu, attributes[attribute].name);
}

/* Unlisted sysfs attributes fail instead of falling through to real hardware. */
static int attribute_index(const char *path, int *cpu)
{
    char name[128];
    if (sscanf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/%127s", cpu, name) != 2 || *cpu < 0 || *cpu >= TEST_CPUS) {
        return -1;
    }
    for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i) {
        if (!strcmp(name, attributes[i].name)) {
            return (int) i;
        }
    }
    return -1;
}

int __wrap_open(const char *path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, mode_t);
        va_end(arguments);
    }
    if (strncmp(path, "/sys/", 5)) {
        return __real_open(path, flags, mode);
    }
    int cpu, attribute = attribute_index(path, &cpu);
    if (attribute < 0 || opened_count == (int) (sizeof(opened) / sizeof(opened[0]))) {
        errno = ENOENT;
        return -1;
    }
    char backing[256];
    attribute_path(backing, sizeof(backing), attribute, cpu);
    int fd = __real_open(backing, flags, mode);
    if (fd >= 0) {
        attributes[attribute].fd[cpu] = fd;
        opened[opened_count++]        = fd;
    }
    return fd;
}

int __wrap_access(const char *path, int mode)
{
    if (strncmp(path, "/sys/", 5)) {
        return __real_access(path, mode);
    }
    int cpu, attribute = attribute_index(path, &cpu);
    if (attribute < 0) {
        errno = ENOENT;
        return -1;
    }
    char backing[256];
    attribute_path(backing, sizeof(backing), attribute, cpu);
    return __real_access(backing, mode);
}

int __wrap_close(int fd)
{
    for (int i = 0; i < opened_count; ++i) {
        if (opened[i] == fd) {
            opened[i] = -1;
        }
    }
    return __real_close(fd);
}

int test_hardware_setup(void)
{
    strcpy(directory, "/tmp/ear-cpufreq-XXXXXX");
    opened_count = 0;
    if (!mkdtemp(directory)) {
        return -1;
    }
    for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i) {
        for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
            char path[256];
            attribute_path(path, sizeof(path), (int) i, cpu);
            int fd = __real_open(path, O_CREAT | O_TRUNC | O_RDWR, 0600);
            if (fd < 0) {
                return -1;
            }
            size_t length   = strlen(attributes[i].contents);
            ssize_t written = write(fd, attributes[i].contents, length);
            __real_close(fd);
            if (written != (ssize_t) length) {
                return -1;
            }
        }
    }
    return 0;
}

void test_hardware_cleanup(void)
{
    for (int i = 0; i < opened_count; ++i) {
        if (opened[i] >= 0) {
            __real_close(opened[i]);
        }
    }
    for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i) {
        for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
            char path[256];
            attribute_path(path, sizeof(path), (int) i, cpu);
            unlink(path);
        }
    }
    rmdir(directory);
}

int test_hardware_fd(const char *attribute, int cpu)
{
    for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i) {
        if (!strcmp(attribute, attributes[i].name)) {
            return attributes[i].fd[cpu];
        }
    }
    return -1;
}

/* Replace machine discovery and persistent keeper I/O with fixed inputs. */
void cpufreq_base_init(topology_t *tp, cpufreq_base_t *base)
{
    (void) tp;
    *base = (cpufreq_base_t) {.frequency = 3000000, .boost_enabled = 0};
}

void cpuid_native(uint *eax, uint *ebx, uint *ecx, uint *edx)
{
    *eax = *ebx = *ecx = *edx = 0;
}

int keeper_load_uint32(const char *id, uint *value)
{
    (void) id;
    (void) value;
    return 0;
}

int keeper_load_uint64(const char *id, ullong *value)
{
    (void) id;
    (void) value;
    return 0;
}

void keeper_save_uint32(const char *id, uint value)
{
    (void) id;
    (void) value;
}

void keeper_save_uint64(const char *id, ullong value)
{
    (void) id;
    (void) value;
}
