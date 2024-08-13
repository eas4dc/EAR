/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_MSR_H
#define EAR_MSR_H

#include <fcntl.h>
#include <unistd.h>
#include <common/types.h>
#include <common/states.h>
#include <common/hardware/topology.h>
#include <metrics/common/apis.h>

#define MSR_RD O_RDONLY
#define MSR_WR O_RDWR

#define msr_clean(fd) \
	*fd = -1

/** Tests if the MSR register is available and readable. */
state_t msr_test(topology_t *tp, mode_t mode);

/** Opens an MSR register for a specific CPU */
state_t msr_open(uint cpu, mode_t mode);

/** It does nothing in reality. */
state_t msr_close(uint cpu);

/** Reads data (buffer) in specific CPU and memory offset MSR register. */
state_t msr_read(uint cpu, void *buffer, size_t count, off_t offset);

/** Writes data (buffer) in a MSR for specific CPU and memory offset. */
state_t msr_write(uint cpu, const void *buffer, size_t count, off_t offset);

/** Prints a register of all threads in the node. */
void msr_print(topology_t *tp, off_t offset);

/** Develoment function to print the content of a list of MSR registers. */
void msr_inspect(topology_t *tp, int cpu, off_t *offsets, int fd);

/** Looks for already used registers. */
state_t msr_scan(topology_t *tp, off_t *regs, uint regs_count, ullong andval, off_t *offs);

#endif //EAR_MSR_H
