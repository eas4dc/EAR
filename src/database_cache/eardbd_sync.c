/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE
#include <database_cache/eardbd.h>
#include <database_cache/eardbd_body.h>
#include <database_cache/eardbd_signals.h>
#include <database_cache/eardbd_storage.h>
#include <database_cache/eardbd_sync.h>

// Buffers
extern char input_buffer[SZ_BUFFER];
extern char extra_buffer[SZ_BUFFER];

//
extern int master_iam; // Master is who speaks
extern int server_iam;
extern int mirror_iam;

// Sockets
extern socket_t *socket_server;
extern socket_t *socket_mirror;
extern socket_t *socket_sync01;
extern socket_t *socket_sync02;

// Synchronization
extern sync_question_t sync_question;
extern sync_answer_t sync_answer;

// Descriptors
extern struct sockaddr_storage addr_new;
extern afd_set_t fds_active;
// Descriptors storage
extern long fd_hosts[EDB_MAX_CONNECTIONS + 48];
//
extern struct timeval timeout_insr;
extern struct timeval timeout_aggr;
extern struct timeval timeout_slct;
extern time_t time_insr;
extern time_t time_aggr;
extern time_t time_slct;
//
extern time_t time_recv1[EDB_NTYPES];
extern time_t time_recv2[EDB_NTYPES];
extern time_t time_insert1[EDB_NTYPES];
extern time_t time_insert2[EDB_NTYPES];
extern size_t type_sizeof[EDB_NTYPES];
extern uint samples_index[EDB_NTYPES];
extern char *type_name[EDB_NTYPES];
extern ulong type_alloc_len[EDB_NTYPES];
extern uint samples_count[EDB_NTYPES];
//
extern uint sockets_accepted;
extern uint sockets_online;
extern uint sockets_err_disconnected;
extern uint sockets_err_timeout;
extern uint sockets_err_other;
// Strings
extern char *str_who[2];
extern int verbosity;

/*
 *
 * Time
 *
 */

void time_substract_timeouts()
{
    timeout_insr.tv_sec -= time_slct;
    timeout_aggr.tv_sec -= time_slct;
}

void time_reset_timeout_insr(time_t offset_insr)
{
    // Refresh insert time
    timeout_insr.tv_sec  = time_insr + offset_insr;
    timeout_insr.tv_usec = 0L;
}

void time_reset_timeout_aggr()
{
    // Refresh aggregation timeout
    timeout_aggr.tv_sec  = time_aggr;
    timeout_aggr.tv_usec = 0L;
}

void time_reset_timeout_slct()
{
    // Refresh select time
    time_slct = timeout_aggr.tv_sec;

    if (timeout_insr.tv_sec < timeout_aggr.tv_sec) {
        time_slct = timeout_insr.tv_sec;
    }

    timeout_slct.tv_sec  = time_slct;
    timeout_slct.tv_usec = 0L;
}

/*
 *
 * Net
 *
 */

int sync_fd_is_new(int fd)
{
    int nc;
    nc = !mirror_iam && (fd == socket_server->fd || fd == socket_sync01->fd);
    nc |= mirror_iam && (fd == socket_mirror->fd);
    return nc;
}

int sync_fd_is_mirror(int fd_lst)
{
    return !mirror_iam && (fd_lst == socket_sync01->fd);
}

int sync_fd_exists(long ip, int *fd_old)
{
    int i;

    if (ip == 0) {
        return 0;
    }
    for (i = fds_active.fd_min; i <= fds_active.fd_max; ++i) {
        if (fd_hosts[i] == ip && AFD_ISSET(i, &fds_active)) {
            *fd_old = i;
            return 1;
        }
    }

    return 0;
}

void sync_fd_add(int fd, long ip)
{
    if (ip == 0) {
        verb1("Warning, the IP of the new connection is %ld", ip);
        // Fake IP (255.0.0.0)
        ip = 4278190080;
    }
    // Saving IP
    fd_hosts[fd] = ip;
    // Enabling FD in select
    AFD_SET(fd, &fds_active);
    // Metrics
    sockets_accepted += 1;
    sockets_online += 1;
}

void sync_fd_get_ip(int fd, long *ip)
{
    *ip = fd_hosts[fd];
}

void sync_fd_disconnect(int fd)
{
    sockets_close_fd(fd);
    // Disabling FD in select
    AFD_CLR(fd, &fds_active);
    // If is 0 maybe was cleaned already
    if (fd_hosts[fd] > 0) {
        sockets_online -= 1;
        fd_hosts[fd] = 0;
    }
}

/*
 *
 * Synchronization main/mirror
 *
 */

int sync_send_question(uint sync_option, int veteran, sync_answer_t *answer)
{
    size_t data_size = 0;
    ullong extra     = 0LLU;
    time_t timeout_old;
    state_t s;

    verb1("synchronization started: asking the question to %s", socket_sync02->host_dst);
    //
    sync_question.sync_option = sync_option;
    sync_question.veteran     = veteran;
    // Synchronization pipeline
    if (state_fail(s = sockets_socket(socket_sync02))) {
        verb1("failed to create client socket (%d, %s)", s, state_msg);
        return EAR_ERROR;
    }
    if (state_fail(s = sockets_connect(socket_sync02))) {
        verb1("failed to connect (%d, %s)", s, state_msg);
        return EAR_ERROR;
    }
    if (state_fail(s = sockets_send(socket_sync02->fd, EDB_TYPE_SYNC_QUESTION, (char *) &sync_question,
                                    sizeof(sync_question_t), key_add(extra)))) {
        verb1("failed to send (%d, %s)", s, state_msg);
        return EAR_ERROR;
    }

    // Setting new timeout
    sockets_timeout_get(socket_sync02->fd, &timeout_old);
    s = sockets_timeout_set(socket_sync02->fd, 10);
    // Waiting
    if (state_ok(s = sockets_recv_header(socket_sync02->fd, NULL, &data_size, &extra, 1))) {
        if (state_ok(s = sockets_recv(socket_sync02->fd, (char *) &sync_answer, data_size, 1))) {
            verb0("synchronization finished");
        }
    }
    // Recovering old timeout
    sockets_timeout_set(socket_sync02->fd, timeout_old);

    if (state_fail(s)) {
        verb1("synchronization failed: %s", state_msg);
        return s;
    }
    // Closing anyway
    sockets_close(socket_sync02);

    if (key_get(extra) != key_add(0LLU)) {
        return_msg(EAR_ERROR, "Key does not match");
    }
    if (answer != NULL) {
        memcpy(answer, &sync_answer, sizeof(sync_answer_t));
    }
    return EAR_SUCCESS;
}

int sync_send_answer(int fd, int veteran)
{
    ullong extra = 0LLU;
    state_t s;

    if (verbosity) {
        verb0("synchronization started: answering the question");
    }
    sync_answer.answer  = 0;
    sync_answer.veteran = veteran;

    if (state_fail(
            s = sockets_send(fd, EDB_TYPE_SYNC_ANSWER, (char *) &sync_answer, sizeof(sync_answer_t), key_add(extra)))) {
        verb1("Failed to send to MIRROR (%d, %s)", s, state_msg);
        return EAR_IGNORE;
    }
    if (verbosity) {
        verb0("synchronization finished");
    }
    return EAR_SUCCESS;
}