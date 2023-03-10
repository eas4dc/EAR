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

// #define SHOW_DEBUGS 1
#include <common/system/poll.h>
#include <common/output/debug.h>

static int ids_count;

#define adebug(...) \
    cdebug(AFD_DEBUG, __VA_ARGS__);

void afd_init(afd_set_t *set)
{
    int i;
    // Full clean
    memset(set, 0, sizeof(afd_set_t));
    // Tags
    #if AFD_DEBUG
    char *p = calloc(sizeof(char), AFD_MAX * AFD_TAG);
    #endif
    //
    for (i = 0; i < AFD_MAX; ++i) {
        set->fds[i].fd      =  -1;
        set->fds[i].events  =   0;
        set->fds[i].revents =   0;
        #if !AFD_DEBUG
        set->tags[i]        = NULL;
        #else
        set->tags[i]        = &p[i*AFD_TAG];
        sprintf(set->tags[i], "-");
        #endif
    }
    set->id     = ++ids_count;
    set->fd_min = AFD_MAX;
    set->fd_max = 0;
    set->init   = 1;
}

int afd_set(int fd, afd_set_t *set, int flags)
{
    if (set->init == 0) {
        afd_init(set);
    }
    if (fd >= AFD_MAX || fd < 0 || set->fds[fd].fd != -1) {
        return 0;
    }

    set->fds[fd].fd      = fd;
    set->fds[fd].events  = flags;
    set->fds[fd].revents = 0;
    set->fds_count      += 1;

    if (fd > set->fd_max) {
        set->fd_max = fd;
    }
    if (fd < set->fd_min) {
        set->fd_min = fd;
    }
    // Counting
    set->fds_rank = set->fd_max - set->fd_min + 1;
    return 1;
}

int afd_ishup(int fd, afd_set_t *set)
{
    return (set->fds[fd].revents & (POLLHUP | POLLERR | POLLNVAL));
}

int afd_isset(int fd, afd_set_t *set, int flag)
{
    int retset = (set->fds[fd].revents & flag);
    int rethup = afd_ishup(fd, set); 
    adebug("AFD_ISSET(%d, set%d): ret %d, hup %d (revents %d), tag %s",
        fd, set->id, retset, rethup, set->fds[fd].revents, set->tags[fd]);
    if (!retset && rethup) {
        retset = rethup;
    }
    return retset;
}

void afd_clear(int fd, afd_set_t *set)
{
    int i;

    if (fd < 0) {
        return;
    }
    if (set->fds[fd].fd == -1) {
        adebug("AFD_CLR is trying to clear invalid fd %d", fd);
        return;
    }
    set->fds[fd].fd      = -1;
    set->fds[fd].events  =  0;
    set->fds[fd].revents =  0;
    set->fds_count      -=  1;

    for (i = set->fd_min; i < AFD_MAX; ++i) {
        if (set->fds[i].fd != -1) {
            set->fd_min = set->fds[i].fd;
            break;
        }
    }
    for (i = set->fd_max; i >= 0; --i) {
        if (set->fds[i].fd != -1) {
            set->fd_max = set->fds[i].fd;
            break;
        }
    }
    // Counting
    set->fds_rank = set->fd_max - set->fd_min + 1;
    #if AFD_DEBUG
    sprintf(set->tags[i], "-");
    #endif
    afd_debug(fd, set, "AFD_CLR");
}

int afd_stat(int fd, struct stat *s)
{
    struct stat *p = s;
    struct stat aux;

    if (s == NULL) {
        p = &aux;
    }
    if (fstat(fd, p) < 0) {
        return 0;
    }
    return 1;
}

void afd_debug(int fd, afd_set_t *set, const char *prefix)
{
    adebug("%s(%d, set%d) [%d,%d], fds_count %u, fds_rank %d, tag %s", prefix, fd,
        set->id, set->fd_min, set->fd_max, set->fds_count, (int) set->fds_rank, set->tags[fd]);
}

int aselect(afd_set_t *set, ullong timeout, ullong *time_left)
{
    // select()
    // On success, return the number of file descriptors contained in the three returned descriptor
    // sets (that is, the total number of bits that are set in readfds, writefds, exceptfds) which
    // may be zero if the timeout expires before anything interesting happens. On error, -1 is
    // returned, and errno is set to indicate the error; the file descriptor sets  are  unmodified,
    // and timeout becomes undefined.
    //
    // poll()
    // On success, a positive number is returned; this is the number of structures which have
    // nonzero revents fields (in other words, those descriptors with events or errors reported). A
    // value of 0 indicates that the call timed out and no file descriptors were ready. On error, -1
    // is returned, and errno is set appropriately.
    ullong time_passed;
    timestamp_t ts;
    int r;
    // Polling and calculating time
    timestamp_get(&ts);
    r = poll(&set->fds[set->fd_min], set->fds_rank, (int) timeout);
    time_passed = timestamp_diffnow(&ts, TIME_MSECS);
    adebug("%d = poll(set%d, fds_count: %u, fds_rank: %lu, timeout: %d, time_passed: %llu)",
        r, set->id, set->fds_count, set->fds_rank, (int) timeout, time_passed);
    // Calculating the lefting time
    if (time_left != NULL) {
        *time_left = 0;
        if (time_passed < timeout) {
            *time_left = timeout - time_passed;
        }
    }
    return r;
}

int aselectv(afd_set_t *set, struct timeval *_timeout)
{
    ullong timeout = AFD_INFINITE;
    ullong time_left;
    int r;
    // If timeout is NULL, then the timeout is infinite.
    if (_timeout != NULL) {
        timeout = timeval_convert(_timeout, TIME_MSECS);
    }
    // Converting timeval to single time value.
    r = aselect(set, timeout, &time_left);
    // Converting lefting time to timeval.
    if (_timeout != NULL) {
        *_timeout = timeval_create(time_left, TIME_MSECS);
    }
    return r;
}
