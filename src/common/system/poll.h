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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef COMMON_SYSTEM_POLL
#define COMMON_SYSTEM_POLL

#include <poll.h>
#include <sys/stat.h>
#include <common/states.h>
#include <common/system/time.h>
//#include <common/output/verbose.h>

#define AFD_MAX        4096
#define AFD_TAG        64
#define AFD_INFINITE  -1 // Infinite timeout
#define AFD_DEBUG      0

typedef struct afd_set_s {
    struct pollfd fds[AFD_MAX];
    char *tags[AFD_MAX];
    nfds_t fds_rank;
    uint fds_count;
    int fd_max;
    int fd_min;
    int init;
    int id;
} afd_set_t;

/* Used to clean and initialize an afd_set. */
void afd_init(afd_set_t *set);

/* Set a file descriptor, and init the afd_set if not done yet. */
int afd_set(int fd, afd_set_t *set, int flags);

int afd_isset(int fd, afd_set_t *set, int flags);

int afd_ishup(int fd, afd_set_t *set);

void afd_clear(int fd, afd_set_t *set);

int afd_stat(int fd, struct stat *st);

void afd_debug(int fd, afd_set_t *set, const char *prefix);

#if AFD_DEBUG
#define AFD_SETT(fd, set, ...) \
    if (afd_set(fd, set, POLLIN)) { \
        snprintf((set)->tags[fd], AFD_TAG, __VA_ARGS__); \
        afd_debug(fd, set, "AFD_SET"); \
    }
#else
#define AFD_SETT(fd, set, ...) \
    afd_set(fd, set, POLLIN)
#endif

#define AFD_SET(fd, set) \
    afd_set(fd, set, POLLIN)

#define AFD_SETW(fd, set) \
    afd_set(fd, set, POLLOUT)

#define AFD_ISSET(fd, set) \
    afd_isset(fd, set, POLLIN | POLLOUT)

#define AFD_ISSETW(fd, set) \
    afd_isset(fd, set, POLLOUT)

#define AFD_ISHUP(fd, set) \
    afd_ishup(fd, set)

#define AFD_CLR(fd, set) \
    afd_clear(fd, set)

#define AFD_STAT(fd, stat) \
    afd_stat(fd, stat)

#define AFD_ZERO(set) \
    afd_init(set)

/* New select based in poll. Time values are in milliseconds. */
int aselect(afd_set_t *set, ullong timeout, ullong *time_left);

/* Version using timevals to ease the compatibility. The lefting time is saved in timeout like select(). */
int aselectv(afd_set_t *set, struct timeval *timeout);

#endif //COMMON_SYSTEM_POLL
