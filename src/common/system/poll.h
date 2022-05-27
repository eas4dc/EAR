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

#ifndef COMMON_SYSTEM_POLL
#define COMMON_SYSTEM_POLL

#include <poll.h>
#include <common/states.h>

typedef struct afd_set_s {
    struct pollfd fds[8192];
    nfds_t fds_count;
    int timeout;
} afd_set_t;

void afd_set(int fd, afd_set_t *set);

int afd_isset(int fd, afd_set_t *set);

void afd_clear(int fd, afd_set_t *set);

#define AFD_SET(fd, set) \
    afd_set(fd, set);

#define AFD_ISSET(fd, set) \
    afd_isset(fd, set);

#define AFD_CLEAR(fd, set) \
    afd_clear(fd, set);

int aselect(afd_set_t *set);

#endif //COMMON_SYSTEM_POLL
