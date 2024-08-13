/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_EARDBD_SYNC_H
#define EAR_EARDBD_SYNC_H

#include <database_cache/eardbd.h>

void time_substract_timeouts();

void time_reset_timeout_insr(time_t offset_insr);

void time_reset_timeout_aggr();

void time_reset_timeout_slct();

int sync_fd_is_new(int fd);

int sync_fd_is_mirror(int fd_lst);

int sync_fd_exists(long ip, int *fd_old);

void sync_fd_add(int fd, long ip);

void sync_fd_get_ip(int fd, long *ip);

void sync_fd_disconnect(int fd);

int sync_question(uint sync_option, int veteran, sync_answer_t *answer);

int sync_answer(int fd, int veteran);

#endif //EAR_EARDBD_SYNC_H
