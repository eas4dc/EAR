/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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

#define MSR_MAX	4096

static pthread_mutex_t lock_gen = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_cpu[MSR_MAX];
static int init_lock[MSR_MAX];
static int init_msr[MSR_MAX];
static int fds_wr[MSR_MAX];
static int fds_rd[MSR_MAX];

static struct error_s {
	char *lock;
	char *cpu_invalid;
	char *cpu_uninitialized;
    char *open;
} Error = {
	.lock        = "error during pthread_mutex",
	.cpu_invalid = "cpu number is out of range",
	.cpu_uninitialized = "cpu MSR is not initialized",
    .open        = "can't open neither MSR nor MSR_SAFE files",

};

#define return_unlock_msg(s, message, lock) \
	pthread_mutex_unlock(lock); \
	return_msg(s, message);

#define return_unlock(s, lock) \
	pthread_mutex_unlock(lock); \
	return s;

state_t msr_test(topology_t *tp, mode_t mode)
{
	state_t s;
	int cpu;

    if (!tp->initialized) {
		return_msg(EAR_ERROR, Generr.input_uninitialized);
	}
	for (cpu = 0; cpu < tp->cpu_count; ++cpu) {
        if (state_fail(s = msr_open(cpu, mode))) {
            return s;
        }
	}
	return EAR_SUCCESS;
}

#if SHOW_DEBUGS
static char *strerrfd(int fd)
{
    if (fd >= 0) return strerror(0);
    return strerror(errno);
}
#endif

static int static_open_debug(char *file, mode_t mode, char *cmode)
{
    int fd = open(file, mode);
    debug("Attempted to open MSR %s (%s): fd %d (%s)", file, cmode, fd, strerrfd(fd));
    return fd;
}

static int static_open(uint cpu, mode_t mode, char *cmode)
{
    char file[SZ_PATH_KERNEL];
    int fd;
        sprintf(file, "/dev/cpu/%d/msr", cpu);
    if ((fd = static_open_debug(file, mode, cmode)) < 0) {
        sprintf(file, "/dev/cpu/%d/msr_safe", cpu);
        fd = static_open_debug(file, mode, cmode);
    }
    return fd;
}

state_t msr_open(uint cpu, mode_t mode)
{
	if (cpu >= MSR_MAX) {
		return_msg(EAR_BAD_ARGUMENT, Error.cpu_invalid);
	}
	// General exclusion
	while (pthread_mutex_trylock(&lock_gen));
	if (init_lock[cpu] == 0) {
		if (pthread_mutex_init(&lock_cpu[cpu], NULL) != 0) {
			return_unlock_msg(EAR_ERROR, Error.lock, &lock_gen);
		}
	}
	init_lock[cpu] = 1;
	pthread_mutex_unlock(&lock_gen);
	// CPU exclusion
	while (pthread_mutex_trylock(&lock_cpu[cpu]));
	if (init_msr[cpu] == 0) {
        fds_wr[cpu] = static_open(cpu, MSR_WR, "MSR_WR");
        fds_rd[cpu] = static_open(cpu, MSR_RD, "MSR_RD");
        //
        init_msr[cpu] = 1;
	}
    if ((mode == MSR_WR) && (fds_wr[cpu] < 0)) {
        return_unlock_msg(EAR_ERROR, Error.open, &lock_cpu[cpu]);
    }
    if ((mode == MSR_RD) && (fds_rd[cpu] < 0)) {
        return_unlock_msg(EAR_ERROR, Error.open, &lock_cpu[cpu]);
    }
	return_unlock(EAR_SUCCESS, &lock_cpu[cpu]);
}

state_t msr_close(uint cpu)
{
	if (cpu >= MSR_MAX) {
		return_msg(EAR_BAD_ARGUMENT, Error.cpu_invalid);
	}
	return EAR_SUCCESS;
}

state_t msr_read(uint cpu, void *buffer, size_t size, off_t offset)
{
    size_t psize;
	if (cpu >= MSR_MAX) {
		return_msg(EAR_BAD_ARGUMENT, Error.cpu_invalid);
	}
	if (init_msr[cpu] == 0) {
		return_msg(EAR_NOT_INITIALIZED, Error.cpu_uninitialized);
	}
    if (fds_rd[cpu] < 0) {
        return_msg(EAR_ERROR, Error.open);
    }
	#ifdef MSR_LOCK
	while (pthread_mutex_trylock(&lock_cpu[cpu]));
    #endif
    psize = pread(fds_rd[cpu], buffer, size, offset);
    debug("MSR read in CPU%d (fd %d, address %lx): %lu bytes of %lu expected",
        cpu, fds_wr[cpu], offset, psize, size);
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
		return_msg(EAR_BAD_ARGUMENT, Error.cpu_invalid);
	}
	if (init_msr[cpu] == 0) {
		return_msg(EAR_NOT_INITIALIZED, Error.cpu_uninitialized);
	}
    if (fds_wr[cpu] < 0) {
        return_msg(EAR_ERROR, Error.open);
    }
    #ifdef MSR_LOCK
	while (pthread_mutex_trylock(&lock_cpu[cpu]));
	#endif
	psize = pwrite(fds_wr[cpu], buffer, size, offset);
    debug("MSR written in CPU%d (fd %d, address %lx): %lu bytes of %lu expected",
        cpu, fds_wr[cpu], offset, psize, size);
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
		id_cpu1 = tp->cpus[cpu].id;
		id_cpu2 = tp->cpus[cpu].sibling_id;
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
		max_cpu = cpu_wanted+1;
		min_cpu = cpu_wanted;
	}
	for (cpu = min_cpu; cpu < max_cpu; ++cpu) {
		if (!init_msr[cpu]) {
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
