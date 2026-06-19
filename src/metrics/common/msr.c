/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/
/* clang-format off */

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <metrics/common/msr.h>

#define MSR_MAX 4096

static pthread_mutex_t lock_gen = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_cpu[MSR_MAX];
static int             init_lock[MSR_MAX];
static int             fds_count[MSR_MAX];
static int             fds_mode[MSR_MAX];
static int             fds[MSR_MAX];

static struct error_s {
    char *lock;
    char *cpu_invalid;
    char *cpu_uninitialized;
    char *open;
} Error = {
    .lock              = "error during pthread_mutex",
    .cpu_invalid       = "cpu number is out of range",
    .cpu_uninitialized = "cpu MSR is not initialized",
    .open              = "can't open neither MSR nor MSR_SAFE files",
};

#define return_unlock_msg(s, message, lock)                                                                            \
    pthread_mutex_unlock(lock);                                                                                        \
    return_msg(s, message);

#define return_unlock(s, lock)                                                                                         \
    pthread_mutex_unlock(lock);                                                                                        \
    return s;

#if SHOW_DEBUGS
static char *strerrfd(int fd)
{
    if (fd >= 0) return strerror(0);
    return strerror(errno);
}
#endif

static int static_open_debug(char *file, mode_t mode, char *mode_str)
{
    int fd = open(file, mode);
    debug("Attempted to open MSR %s (%s): fd %d (%s)", file, mode_str, fd, strerrfd(fd));
    return fd;
}

static int static_open(uint cpu, mode_t mode)
{
    char *mode_str = (mode == MSR_RD) ? "MSR_RD": "MSR_WR";
    char file[64]; // Enough for MSR path
    int fd;

    sprintf(file, "/dev/cpu/%d/msr", cpu);
    if ((fd = static_open_debug(file, mode, mode_str)) < 0) {
        sprintf(file, "/dev/cpu/%d/msr_safe", cpu);
        fd = static_open_debug(file, mode, mode_str);
    }
    return fd;
}

state_t msr_test(topology_t *tp, mode_t mode)
{
    int fd = -1;
    int cpu;

    if (!tp->initialized) {
        return_msg(EAR_ERROR, Generr.input_uninitialized);
    }
    // Reducing overhead by testing in steps of 3 (odd)
    for (cpu = 0; cpu < tp->cpu_count; cpu += 3) {
        if ((fd = static_open(cpu, mode)) >= 0) {
            close(fd);
        } else {
            return_msg(EAR_ERROR, strerror(errno));
        }
    }
    return EAR_SUCCESS;
}

state_t msr_open(uint cpu, mode_t mode)
{
    int fd_aux = -1;

    if (cpu >= MSR_MAX) {
        return_msg(EAR_ERROR, Error.cpu_invalid);
    }
    // General exclusion
    while (pthread_mutex_trylock(&lock_gen));
    if (init_lock[cpu] == 0) {
        if (pthread_mutex_init(&lock_cpu[cpu], NULL) != 0) {
            return_unlock_msg(EAR_ERROR, Error.lock, &lock_gen);
        }
        init_lock[cpu] = 1;
    }
    pthread_mutex_unlock(&lock_gen);
    // CPU exclusion
    while (pthread_mutex_trylock(&lock_cpu[cpu]));
    if (fds_count[cpu] == 0) {
        if ((fds[cpu] = static_open(cpu, mode)) >= 0) {
            fds_mode[cpu] = mode;
        } else {
            return_unlock_msg(EAR_ERROR, Error.open, &lock_cpu[cpu]);
        }
    } else if (fds_count[cpu] > 0) {
        if (mode == MSR_WR && fds_mode[cpu] == MSR_RD) {
            if ((fd_aux = static_open(cpu, MSR_WR)) >= 0) {
                debug("CPU%u: Replacing FD%d in mode %d by FD%d in mode %d",
                    cpu, fds[cpu], fds_mode[cpu], fd_aux, mode);
                // Replacing MSR_RD file descriptor by MSR_WR
                close(fds[cpu]);
                fds[cpu] = fd_aux;
                fds_mode[cpu] = MSR_WR;
            } else {
                return_unlock_msg(EAR_ERROR, Error.open, &lock_cpu[cpu]);
            }
        }
    }
    fds_count[cpu] += 1;
    debug("CPU%u: opened FD%d MSR %d times in mode %d",
        cpu, fds[cpu], fds_count[cpu], fds_mode[cpu]);
    return_unlock(EAR_SUCCESS, &lock_cpu[cpu]);
}

state_t msr_close(uint cpu)
{
    if (cpu >= MSR_MAX) {
        return_msg(EAR_ERROR, Error.cpu_invalid);
    }
    if (fds_count[cpu] > 0) {
        if (fds_count[cpu] == 1) {
            debug("CPU%u: closed FD%d MSR, opened %d times, in mode %d",
                cpu, fds[cpu], fds_count[cpu], fds_mode[cpu]);
            close(fds[cpu]);
        } else {
            debug("CPU%u: tried to close FD%d MSR, opened %d times, in mode %d",
                cpu, fds[cpu], fds_count[cpu], fds_mode[cpu]);
        }
        fds_count[cpu] -= 1;
    }
    return EAR_SUCCESS;
}

state_t msr_read(uint cpu, void *buffer, size_t size, off_t offset)
{
    size_t psize;

    if (cpu >= MSR_MAX) {
        return_msg(EAR_ERROR, Error.cpu_invalid);
    }
    if (fds_count[cpu] == 0) {
        return_msg(EAR_ERROR, Error.cpu_uninitialized);
    }
    #ifdef MSR_LOCK
    while (pthread_mutex_trylock(&lock_cpu[cpu]));
    #endif
    psize = pread(fds[cpu], buffer, size, offset);
    debug("MSR read in CPU%d (fd %d, address %lx): %lu bytes of %lu expected",
          cpu, fds[cpu], offset, psize, size);
    if (psize != size) {
        #ifdef MSR_LOCK
        return_unlock_msg(EAR_ERROR, strerror(errno), &lock_cpu[cpu]);
        #else
        return_msg(EAR_ERROR, strerror(errno));
        #endif
    }
    #ifdef MSR_LOCK
    return_unlock(EAR_SUCCESS, &lock_cpu[cpu]);
    #else
    return EAR_SUCCESS;
    #endif
}

state_t msr_write(uint cpu, const void *buffer, size_t size, off_t offset)
{
    size_t psize;

    if (cpu >= MSR_MAX) {
        return_msg(EAR_ERROR, Error.cpu_invalid);
    }
    if (fds_count[cpu] == 0) {
        return_msg(EAR_ERROR, Error.cpu_uninitialized);
    }
    #ifdef MSR_LOCK
    while (pthread_mutex_trylock(&lock_cpu[cpu]));
    #endif
    psize = pwrite(fds[cpu], buffer, size, offset);
    debug("MSR written in CPU%d (fd %d, address %lx): %lu bytes of %lu expected",
          cpu, fds[cpu], offset, psize, size);
    if (psize != size) {
        #ifdef MSR_LOCK
        return_unlock_msg(EAR_ERROR, strerror(errno), &lock_cpu[cpu]);
        #else
        return_msg(EAR_ERROR, strerror(errno));
        #endif
    }
    #ifdef MSR_LOCK
    return_unlock(EAR_SUCCESS, &lock_cpu[cpu]);
    #else
    return EAR_SUCCESS;
    #endif
}

/*
 *
 * Development helpers
 *
 */

void msr_print(topology_t *tp, off_t offset)
{
    ulong value_cpu1;
    ulong value_cpu2;
    state_t s_cpu1;
    state_t s_cpu2;
    int id_cpu1;
    int id_cpu2;
    int cpu;

    for (cpu = 0; cpu < tp->cpu_count; ++cpu) {
        if (tp->cpus[cpu].is_thread) {
            continue;
        }
        // Print the MSR value of both threads of a core.
        id_cpu1    = tp->cpus[cpu].id;
        id_cpu2    = tp->cpus[cpu].sibling_id;
        value_cpu1 = 0LU;
        value_cpu2 = 0LU;
        // Open both core and thread
        s_cpu1 = msr_open(id_cpu1, MSR_RD);
        s_cpu2 = msr_open(id_cpu2, MSR_RD);
        // Reading both values
        if (state_ok(s_cpu1)) msr_read(id_cpu1, &value_cpu1, sizeof(ulong), offset);
        if (state_ok(s_cpu2)) msr_read(id_cpu2, &value_cpu2, sizeof(ulong), offset);
        // Printing
        verbose(0, "%d/%d: %lu %lu", id_cpu1, id_cpu2, value_cpu1, value_cpu2);
    }
}

void msr_inspect(topology_t *tp, int cpu_wanted, off_t *offsets, int fd)
{
    int max_cpu = 0;
    int min_cpu = 0;
    int cpu, off;
    ullong value;

    if (cpu_wanted == all_cpus) {
        max_cpu = tp->cpu_count;
    } else if (cpu_wanted == all_cores) {
        max_cpu = tp->core_count;
    } else {
        max_cpu = cpu_wanted + 1;
        min_cpu = cpu_wanted;
    }
    for (cpu = min_cpu; cpu < max_cpu; ++cpu) {
        if (fds_count[cpu] == 0) {
            if (state_fail(msr_open(cpu, MSR_RD))) {
                continue;
            }
        }
        // Through all the offsets
        off = 0;
        while (offsets[off] != 0LLU) {
            value = 0LLU;
            msr_read(cpu, &value, sizeof(ullong), offsets[off]);
            dprintf(fd, "%09llx ", value);
            ++off;
        }
        dprintf(fd, "\n");
    }
}

state_t msr_scan(topology_t *tp, off_t *regs, uint regs_count, ullong andval, off_t *offs)
{
    ullong content;
    uint cpu, reg;
    off_t zero;
    state_t s;

    for (cpu = 0; cpu < tp->cpu_count; ++cpu) {
        if (state_fail(s = msr_open(tp->cpus[cpu].id, MSR_RD))) {
            break;
        }
        // Initializing
        offs[cpu] = 0;
        zero      = 0;
        // Reading registers
        for (reg = 0; reg < regs_count; ++reg) {
            if (state_fail(s = msr_read(tp->cpus[cpu].id, &content, sizeof(content), regs[reg]))) {
                return s;
            }
            // If a register is using the same event, use that
            if (content & andval) {
                offs[cpu] = regs[reg];
                continue;
            }
            if (!content && !zero) {
                zero = regs[reg];
            }
        }
        // If free register is not found, use the first in the list
        if (!offs[cpu] && !zero) {
            offs[cpu] = regs[0];
        }
        // If a free register is found, use that
        if (!offs[cpu] && zero) {
            offs[cpu] = zero;
        }
    }
    return s;
}
