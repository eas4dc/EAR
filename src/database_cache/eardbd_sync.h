/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
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

int sync_question(uint sync_option, int veteran, sync_ans_t *answer);

int sync_answer(int fd, int veteran);

#endif //EAR_EARDBD_SYNC_H
