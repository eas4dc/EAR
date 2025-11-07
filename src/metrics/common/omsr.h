/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_OMSR_H
#define EAR_OMSR_H
#include <common/states.h>
#include <common/types/generic.h>
#include <unistd.h>

/* */

#define MAX_PACKAGES 16
#define NUM_SOCKETS  2

state_t omsr_open(uint cpu, int *fd);

/* */
state_t omsr_close(int *fd);

/* */
state_t omsr_read(int *fd, void *buffer, size_t count, off_t offset);

/* */
state_t omsr_write(int *fd, const void *buffer, size_t count, off_t offset);

int get_msr_ids(int *dest_fd_map);
int get_total_packages();
int is_msr_initialized();
int init_msr(int *dest_fd_map);

#endif // EAR_MSR_H
