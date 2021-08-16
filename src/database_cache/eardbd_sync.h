/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

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
