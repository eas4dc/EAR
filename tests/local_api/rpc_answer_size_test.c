/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* eard_recv() is the one place that copies a length the daemon chose into a buffer
 * the caller sized. Every RPC in the tree goes through it, so a daemon that answers
 * more bytes than the client asked for is a heap overflow in the job process - and
 * the client is the side that cannot check the length any other way.
 *
 * No energy plug-in here on purpose: this is about the transport, so the tests
 * drive eard_rpc()/eard_rpc_buffered() straight against a mock daemon on a
 * socketpair and use a service number nothing serves. See
 * energy_shim_contract_test.c for the layer above.
 *
 * Worth running under valgrind or ASan: case 2 is written so that a regression
 * shows up as a detected overflow there, and as a canary mismatch without them. */

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <common/states.h>
#include <daemon/local_api/eard_api.h>
#include <daemon/local_api/eard_api_rpc.h>

/* Client-side connection state the RPC layer reads. Setting these by hand is what
 * lets the mock stand in for a real daemon without the FIFO handshake. */
extern int ear_fd_req;
extern int ear_fd_ack;
extern uint8_t app_connected;

/* Any number will do - the mock answers whatever it is asked. Kept outside the
 * ranges in eard_api_rpc.h so a reader does not take it for a real service. */
#define TEST_RPC 9001

#define PAYLOAD    64  /* what the client asks for */
#define CANARY     0x5A
#define CANARY_PAD 64 /* red zone either side of the client's buffer */

struct mock {
    int fd;
    size_t answer_size; /* bytes the daemon claims, and sends */
    int answers;        /* how many requests it served */
};

static int write_all(int fd, const void *buf, size_t size)
{
    const char *p = (const char *) buf;
    size_t done   = 0;

    while (done < size) {
        ssize_t w = write(fd, p + done, size - done);
        if (w <= 0) {
            return -1;
        }
        done += (size_t) w;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t size)
{
    char *p     = (char *) buf;
    size_t done = 0;

    while (done < size) {
        ssize_t r = read(fd, p + done, size - done);
        if (r <= 0) {
            return -1;
        }
        done += (size_t) r;
    }
    return 0;
}

/* Mirrors eard_send(): a header, then the payload. The payload is filled with the
 * request count so a test can tell one answer from the next. */
static void *mock_server(void *arg)
{
    struct mock *m = (struct mock *) arg;
    eard_head_t head;
    char *body;

    for (;;) {
        if (read_all(m->fd, &head, sizeof(head)) != 0) {
            break;
        }
        if (head.size > 0) { // request body, nothing here sends one
            body = malloc(head.size);
            if (body == NULL || read_all(m->fd, body, head.size) != 0) {
                free(body);
                break;
            }
            free(body);
        }

        body = malloc(m->answer_size);
        if (body == NULL) {
            break;
        }
        memset(body, m->answers + 1, m->answer_size);
        m->answers++;

        head.state = EAR_SUCCESS;
        head.size  = m->answer_size;
        if (write_all(m->fd, &head, sizeof(head)) != 0 || write_all(m->fd, body, m->answer_size) != 0) {
            free(body);
            break;
        }
        free(body);
    }

    close(m->fd);
    m->fd = -1;
    return NULL;
}

static int mock_start(struct mock *m, size_t answer_size, pthread_t *thread)
{
    int sv[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        return -1;
    }
    memset(m, 0, sizeof(*m));
    m->fd          = sv[1];
    m->answer_size = answer_size;

    ear_fd_req    = sv[0];
    ear_fd_ack    = sv[0];
    app_connected = 1;

    if (pthread_create(thread, NULL, mock_server, m) != 0) {
        close(sv[0]);
        close(sv[1]);
        return -1;
    }
    return 0;
}

static void mock_stop(struct mock *m, pthread_t thread)
{
    close(ear_fd_req);
    ear_fd_req    = -1;
    ear_fd_ack    = -1;
    app_connected = 0;
    pthread_join(thread, NULL);
    if (m->fd >= 0) {
        close(m->fd);
    }
}

/* A buffer with a red zone either side, so an overflow is visible even where the
 * allocator would have absorbed it. */
struct guarded {
    char *block;
    char *data;
};

static int guarded_alloc(struct guarded *g, size_t size)
{
    g->block = malloc(size + 2 * CANARY_PAD);
    if (g->block == NULL) {
        return -1;
    }
    memset(g->block, CANARY, size + 2 * CANARY_PAD);
    g->data = g->block + CANARY_PAD;
    return 0;
}

static int guarded_intact(struct guarded *g, size_t size)
{
    for (size_t i = 0; i < CANARY_PAD; i++) {
        if ((unsigned char) g->block[i] != CANARY) {
            return 0;
        }
        if ((unsigned char) g->data[size + i] != CANARY) {
            return 0;
        }
    }
    return 1;
}

/* The daemon answering exactly what was asked for: the baseline the rejection
 * cases are only meaningful against. */
static int case_exact_size(void)
{
    struct guarded g;
    struct mock m;
    pthread_t thread;
    int failures = 0;

    if (mock_start(&m, PAYLOAD, &thread) != 0 || guarded_alloc(&g, PAYLOAD) != 0) {
        printf("  SKIP: cannot start the mock daemon\n");
        return 0;
    }

    if (state_fail(eard_rpc(TEST_RPC, NULL, 0, g.data, PAYLOAD))) {
        printf("  FAIL: an exactly sized answer was rejected (%s)\n", state_msg);
        failures++;
    } else if (g.data[0] != 1 || g.data[PAYLOAD - 1] != 1) {
        printf("  FAIL: the answer did not land in the caller's buffer\n");
        failures++;
    } else {
        printf("  exact-size answer                        : accepted, ok\n");
    }
    if (!guarded_intact(&g, PAYLOAD)) {
        printf("  FAIL: the exact-size answer wrote outside the buffer\n");
        failures++;
    }

    free(g.block);
    mock_stop(&m, thread);
    return failures;
}

/* The case that matters: more bytes than the caller's buffer holds must be
 * rejected before the copy, not after it. */
static int case_over_long_answer(void)
{
    struct guarded g;
    struct mock m;
    pthread_t thread;
    int failures = 0;

    if (mock_start(&m, PAYLOAD + sizeof(ulong), &thread) != 0 || guarded_alloc(&g, PAYLOAD) != 0) {
        printf("  SKIP: cannot start the mock daemon\n");
        return 0;
    }

    if (state_ok(eard_rpc(TEST_RPC, NULL, 0, g.data, PAYLOAD))) {
        printf("  FAIL: an over-long answer was accepted\n");
        failures++;
    } else {
        printf("  over-long answer                         : rejected, ok\n");
    }
    if (!guarded_intact(&g, PAYLOAD)) {
        printf("  FAIL: an over-long answer wrote outside the caller's buffer\n");
        failures++;
    }

    free(g.block);
    mock_stop(&m, thread);
    return failures;
}

/* Rejecting is only half of it. The body is on the wire either way, so leaving it
 * there would misalign every later RPC on the connection - the next call would
 * read this answer's tail as its own header. */
static int case_connection_survives_rejection(void)
{
    struct guarded small, big;
    struct mock m;
    pthread_t thread;
    int failures = 0;

    if (mock_start(&m, PAYLOAD, &thread) != 0 || guarded_alloc(&small, PAYLOAD / 2) != 0 ||
        guarded_alloc(&big, PAYLOAD) != 0) {
        printf("  SKIP: cannot start the mock daemon\n");
        return 0;
    }

    // Ask for half of what the daemon will answer: rejected, and drained.
    if (state_ok(eard_rpc(TEST_RPC, NULL, 0, small.data, PAYLOAD / 2))) {
        printf("  FAIL: an over-long answer was accepted\n");
        failures++;
    }

    // Same connection, correctly sized this time. The mock fills answer N with N.
    if (state_fail(eard_rpc(TEST_RPC, NULL, 0, big.data, PAYLOAD))) {
        printf("  FAIL: the connection did not survive a rejected answer (%s)\n", state_msg);
        failures++;
    } else if (big.data[0] != 2) {
        printf("  FAIL: the next RPC read the rejected answer's bytes (got %d, wanted 2)\n", big.data[0]);
        failures++;
    } else {
        printf("  RPC after a rejected answer               : in sync, ok\n");
    }
    if (!guarded_intact(&small, PAYLOAD / 2) || !guarded_intact(&big, PAYLOAD)) {
        printf("  FAIL: a rejected answer wrote outside a caller's buffer\n");
        failures++;
    }

    free(small.block);
    free(big.block);
    mock_stop(&m, thread);
    return failures;
}

/* eard_rpc_buffered() answers out of the scratch buffer, which grows to fit. The
 * pointer it hands back has to be the grown one: realloc() moves the block, so a
 * pointer published before the call can be freed memory by the time it is read. */
static int case_buffered_answer_grows(void)
{
    const size_t big_answer = 32768; // over the 8K the scratch buffer starts at
    struct mock m;
    pthread_t thread;
    char *buffer = NULL;
    size_t got   = 0;
    int failures = 0;

    if (mock_start(&m, big_answer, &thread) != 0) {
        printf("  SKIP: cannot start the mock daemon\n");
        return 0;
    }

    if (state_fail(eard_rpc_buffered(TEST_RPC, NULL, 0, &buffer, &got))) {
        printf("  FAIL: a buffered answer larger than the scratch buffer was rejected (%s)\n", state_msg);
        failures++;
    } else if (buffer == NULL || got != big_answer) {
        printf("  FAIL: buffered answer came back as %lu bytes, wanted %lu\n", (ulong) got, (ulong) big_answer);
        failures++;
    } else if (buffer[0] != 1 || buffer[big_answer - 1] != 1) {
        printf("  FAIL: the buffered answer is not the one the daemon sent\n");
        failures++;
    } else {
        printf("  buffered answer over the scratch size    : grown and published, ok\n");
    }

    mock_stop(&m, thread);
    return failures;
}

/* One process per case: a rejection can disconnect the RPC layer, and
 * rpc_disconnected is a static with no way back. fork() also keeps an overflow in
 * one case from being blamed on the next. */
static int run_in_child(int (*fn)(void))
{
    pid_t pid;
    int status;

    fflush(stdout);
    fflush(stderr);

    pid = fork();
    if (pid < 0) {
        printf("  SKIP: fork failed\n");
        return 0;
    }
    if (pid == 0) {
        int rc = fn();
        fflush(stdout);
        _exit((rc != 0) ? 1 : 0);
    }

    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("  FAIL: case died on signal %d\n", WTERMSIG(status));
        return 1;
    }
    if (!WIFEXITED(status)) {
        printf("  FAIL: case did not exit normally\n");
        return 1;
    }
    return WEXITSTATUS(status);
}

int main(void)
{
    int failed = 0;

    printf("=== RPC answer size bounds ===\n");
    failed += (run_in_child(case_exact_size) != 0);
    failed += (run_in_child(case_over_long_answer) != 0);
    failed += (run_in_child(case_connection_survives_rejection) != 0);
    failed += (run_in_child(case_buffered_answer_grows) != 0);
    printf("\n");

    if (failed != 0) {
        printf("%d case(s) failed.\n", failed);
        return EXIT_FAILURE;
    }
    printf("All cases behaved as intended.\n");
    return EXIT_SUCCESS;
}
