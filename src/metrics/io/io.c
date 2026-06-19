/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <common/math_operations.h>
#include <metrics/io/io.h>
#include <metrics/io/archs/dummy.h>
#include <metrics/io/archs/proc_file.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static apinfo_t        info;
static io_ops_t        ops;

void io_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        goto done;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    io_proc_file_load(tp, &ops, options);
dummy:
    io_dummy_load(tp, &ops, options);
    io_get_info(&info);
done:
    pthread_mutex_unlock(&lock);
}

void io_unload()
{
    while (pthread_mutex_trylock(&lock));
    if (ops.unload != NULL) {
        ops.unload();
        memset(&ops, 0, sizeof(io_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

state_t io_update(uint option, void *value)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    s = ops.update(option, value);
    pthread_mutex_unlock(&lock);
    return s;
}

void io_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "IO";
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t io_read(io_t *io)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    memset(io, 0, sizeof(io_t)*info.devs_count);
    s = ops.read(io);
    pthread_mutex_unlock(&lock);
    return s;
}

state_t io_read_diff(io_t *io2, io_t *io1, io_t *io_diff, double *mbs)
{
    state_t s;
    if (state_fail(s = io_read(io2))) {
        return s;
    }
    io_data_diff(io2, io1, io_diff, mbs);
    return s;
}

state_t io_read_copy(io_t *io2, io_t *io1, io_t *io_diff, double *mbs)
{
    state_t s;
    if (state_fail(s = io_read_diff(io2, io1, io_diff, mbs))) {
        return s;
    }
    io_data_copy(io1, io2);
    return s;
}

void io_data_diff(io_t *io2, io_t *io1, io_t *io_diff, double *mbs)
{
    double mbs_own = 0.0;
    double secs = 0.0;
    int i;

    secs = timestamp_fdiff(&io2[0].time, &io1[0].time, TIME_SECS, TIME_MSECS);
    for (i = 0; i < info.devs_count; ++i) {
        if (io2[i].pid == 0 || io1[i].pid == 0) {
            continue;
        }
        io_diff[i].secs  = secs;
        io_diff[i].pid   = io2[i].pid;
        io_diff[i].rchar = overflow_zeros_u64(io2[i].rchar, io1[i].rchar);
        io_diff[i].wchar = overflow_zeros_u64(io2[i].wchar, io1[i].wchar);
        io_diff[i].syscr = overflow_zeros_u64(io2[i].syscr, io1[i].syscr);
        io_diff[i].syscw = overflow_zeros_u64(io2[i].syscw, io1[i].syscw);
        io_diff[i].rstor = overflow_zeros_u64(io2[i].rstor, io1[i].rstor);
        io_diff[i].wstor = overflow_zeros_u64(io2[i].wstor, io1[i].wstor);
        io_diff[i].cancelled = overflow_zeros_u64(io2[i].cancelled, io1[i].cancelled);
        mbs_own += (double) (io_diff[i].rstor + io_diff[i].wstor);
    }
    if (mbs != NULL) {
        if (secs == 0.0) {
            secs = 1.0;
        }
        *mbs = mbs_own / secs;
    }
}

void io_data_alloc(io_t **io)
{
    if (io == NULL) {
        return;
    }
    *io = (io_t *) calloc(info.devs_count, sizeof(io_t));
}

void io_data_free(io_t **io)
{
    if (io != NULL || *io != NULL) {
        return;
    }
    free(*io);
    *io = NULL;
}

void io_data_copy(io_t *io_dst, io_t *io_src)
{
    memcpy(io_dst, io_src, sizeof(io_t) * info.devs_count);
}

void io_data_print(io_t *io_diff, double mbs, int fd)
{
    char buffer[1024] = "";
    io_data_tostr(io_diff, mbs, buffer, sizeof(buffer));
    dprintf(fd, "%s", buffer);
}

char *io_data_tostr(io_t *io_diff, double mbs, char *buffer, size_t length)
{
    int i, b, w;
    buffer[0] = '\0';
    for (i = b = 0; i < info.devs_count && length > 0; ++i) {
        if (io_diff[i].pid == 0) {
            continue;
        }
        w = snprintf(&buffer[b], length-1,
            "d%d "
            #if PRINT_ALOT
            "rchar %011llu wchar %011llu, "
            "syscr %011llu syscw %011llu, "
            #endif
            "rwstor: %llu bytes\n", i,
            #if PRINT_ALOT
            io_diff[i].rchar, io_diff[i].wchar,
            io_diff[i].syscr, io_diff[i].syscw,
            #endif
            io_diff[i].rstor + io_diff[i].wstor);
        w = (w < (length-1))? w: length;
        b += w, length -= w;
    }
    return buffer;
}

#if TEST
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static topology_t tp;
static apinfo_t   info;
static io_t   *t1;
static io_t   *t2;
static io_t   *tD;
static double     tA;
static pid_t      pid;
static int        forked;

static int count_fds()
{
    int dummy_fd = open("/dev/null", O_RDONLY);
    close(dummy_fd);
    return dummy_fd;
}

int main(int argc, char *argv[])
{
    int i, j, k = 0;

    topology_init(&tp);
reload:
    dprintf(STDOUT_FILENO, "%d: Loading... (%d fds)\n", getpid(), count_fds());
    io_load(&tp, API_FREE);
    io_get_info(&info);
    apinfo_tostr(&info);
    dprintf(STDOUT_FILENO, "%d: Loaded %s (%s:%s), with %u devices and %d fds\n",
        getpid(), info.api_str, info.scope_str, info.granularity_str, info.devs_count, count_fds());
    io_data_alloc(&t1);
    io_data_alloc(&t2);
    io_data_alloc(&tD);
reread:
    io_read(t1);
    sleep(2);
    for (i = j = 0; i < 1000000; ++i) {
        j += 1;
    }
    io_data_print(t1, 0.0, STDOUT_FILENO);
    io_read_diff(t2, t1, tD, &tA);
    io_data_print(t2, 0.0, STDOUT_FILENO);
    io_data_print(tD, 0.0, STDOUT_FILENO);
    io_data_copy(t1, t2);
    dprintf(STDOUT_FILENO, "%d: Printing...\n", getpid());
    #if 1
    if (k++ == 2 || k == 10) {
        io_update(UPD_PID_ADD, (void *) 1383557);
    } else if (k == 6 || k == 14){
        io_update(UPD_PID_REMOVE, (void *) 1383557);
        //cache_update(UPD_PIDS_CLEAN, NULL);
    }
    #endif
    goto reread;
    return 0;
}
#endif