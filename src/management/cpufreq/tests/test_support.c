/***************************************************************************
 * Copyright (c) 2026 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE
#include "cpufreq_fixture.h"
#include <cmocka.h>
#include <errno.h>
#include <management/cpufreq/drivers/acpi_cpufreq.h>
#include <management/cpufreq/drivers/intel_pstate.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static _Thread_local test_context_t *worker_context;
static _Thread_local int reader_role;

int __real_pthread_mutex_lock(pthread_mutex_t *mutex);
off_t __real_lseek(int fd, off_t offset, int whence);

static struct timespec deadline(void)
{
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    end.tv_sec += 2;
    return end;
}

/* Caller holds this context's gate. No CMocka calls from worker threads. */
static int wait_for(test_context_t *context, int *flag)
{
    struct timespec end = deadline();
    int error           = 0;
    while (!*flag && !error) {
        error = pthread_cond_timedwait(&context->changed, &context->gate, &end);
    }
    if (error) {
        context->gate_error = error;
    }
    return error;
}

int test_context_init(test_context_t *context)
{
    *context  = (test_context_t) {.watched_fd = -1, .frequency_fds = {-1, -1}, .governor_fds = {-1, -1}};
    int error = pthread_mutex_init(&context->gate, NULL);
    if (!error) {
        error = pthread_cond_init(&context->changed, NULL);
        if (error) {
            pthread_mutex_destroy(&context->gate);
        }
    }
    return error;
}

int test_context_destroy(test_context_t *context)
{
    int error       = pthread_cond_destroy(&context->changed);
    int mutex_error = pthread_mutex_destroy(&context->gate);
    return error ? error : mutex_error;
}

int test_bind_files(test_context_t *context, const char *frequency_attribute)
{
    for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
        context->frequency_fds[cpu] = test_hardware_fd(frequency_attribute, cpu);
        context->governor_fds[cpu]  = test_hardware_fd("scaling_governor", cpu);
        if (context->frequency_fds[cpu] < 0 || context->governor_fds[cpu] < 0) {
            return EBADF;
        }
    }
    return 0;
}

int test_write_file(int fd, const char *contents)
{
    size_t length = strlen(contents);
    return ftruncate(fd, 0) != 0 || pwrite(fd, contents, length, 0) != (ssize_t) length ? EIO : 0;
}

int test_set_frequencies(test_context_t *context, ullong cpu0, ullong cpu1)
{
    ullong values[] = {cpu0, cpu1};
    for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%llu\n", values[cpu]);
        int error = test_write_file(context->frequency_fds[cpu], buffer);
        if (error) {
            return error;
        }
    }
    return 0;
}

void test_expect_file(int fd, const char *expected)
{
    char actual[64] = {0};
    assert_true(pread(fd, actual, sizeof(actual) - 1, 0) > 0);
    assert_string_equal(actual, expected);
}

int __wrap_pthread_mutex_lock(pthread_mutex_t *mutex)
{
    test_context_t *context = worker_context;
    if (!context || mutex == &context->gate) {
        return __real_pthread_mutex_lock(mutex);
    }
    __real_pthread_mutex_lock(&context->gate);
    if (reader_role == READER_A && context->pause_seek) {
        context->driver_mutex = mutex;
    } else if (reader_role == READER_B && mutex == context->driver_mutex && context->pause_seek) {
        context->lock_probe = pthread_mutex_trylock(mutex);
        if (!context->lock_probe) {
            pthread_mutex_unlock(mutex);
        }
        context->b_lock_attempt = 1;
        pthread_cond_broadcast(&context->changed);
    }
    pthread_mutex_unlock(&context->gate);
    return __real_pthread_mutex_lock(mutex);
}

off_t __wrap_lseek(int fd, off_t offset, int whence)
{
    off_t result            = __real_lseek(fd, offset, whence);
    test_context_t *context = worker_context;
    if (context && reader_role == READER_A && fd == context->watched_fd && result == 0 && whence == SEEK_SET) {
        __real_pthread_mutex_lock(&context->gate);
        if (context->pause_seek) {
            context->a_waiting = 1;
            pthread_cond_broadcast(&context->changed);
            wait_for(context, &context->release_a);
        }
        pthread_mutex_unlock(&context->gate);
    }
    return result;
}

static void pause_after_driver_read(void)
{
    test_context_t *context = worker_context;
    if (context && reader_role == READER_A && context->pause_default) {
        __real_pthread_mutex_lock(&context->gate);
        context->a_waiting = 1;
        pthread_cond_broadcast(&context->changed);
        wait_for(context, &context->release_a);
        pthread_mutex_unlock(&context->gate);
    }
}

/* Scheduling only: preserve the real getter's arguments, output and status. */
state_t test_scheduled_intel_read(test_driver_output_t out)
{
    state_t status = mgt_intel_pstate_get_current_list(out);
    pause_after_driver_read();
    return status;
}

state_t test_scheduled_acpi_read(test_driver_output_t out)
{
    state_t status = mgt_acpi_cpufreq_get_current_list(out);
    pause_after_driver_read();
    return status;
}

void *test_read_worker(void *argument)
{
    test_reader_t *reader   = argument;
    test_context_t *context = reader->context;
    worker_context          = context;
    reader_role             = reader->role;
    if (reader->get_frequencies) {
        reader->status = test_read(reader->get_frequencies, &reader->snapshot);
    } else if (reader->get_governors) {
        reader->status = reader->get_governors(reader->governors);
    } else if (reader->get_pstates) {
        reader->status = reader->get_pstates(NULL, reader->pstates);
    } else {
        reader->status = EAR_BAD_ARGUMENT;
    }
    __real_pthread_mutex_lock(&context->gate);
    if (reader_role == READER_B) {
        context->b_done = 1;
        pthread_cond_broadcast(&context->changed);
    }
    pthread_mutex_unlock(&context->gate);
    worker_context = NULL;
    reader_role    = 0;
    return NULL;
}

int test_wait_for_a(test_context_t *context)
{
    __real_pthread_mutex_lock(&context->gate);
    int error = wait_for(context, &context->a_waiting);
    pthread_mutex_unlock(&context->gate);
    return error;
}

int test_wait_for_b(test_context_t *context)
{
    __real_pthread_mutex_lock(&context->gate);
    struct timespec end = deadline();
    int error           = 0;
    /* Unlocked readers finish; serialized readers attempt the driver lock.
     * Never require B's I/O to proceed while paused A owns the driver lock. */
    while (!context->b_done && !context->b_lock_attempt && !error) {
        error = pthread_cond_timedwait(&context->changed, &context->gate, &end);
    }
    if (error) {
        context->gate_error = error;
    }
    pthread_mutex_unlock(&context->gate);
    return error;
}

void test_release_a(test_context_t *context)
{
    __real_pthread_mutex_lock(&context->gate);
    context->release_a = 1;
    pthread_cond_broadcast(&context->changed);
    pthread_mutex_unlock(&context->gate);
}

/* Keep the subprocess watchdog, but expose only one CMocka report. Child
 * diagnostics go to a file (not a pipe, which could fill while parent waits). */
void test_run_isolated(void **state)
{
    const test_case_t *test   = *state;
    const char *name          = test->name;
    void (*scenario)(void **) = test->scenario;
    FILE *output              = tmpfile();
    assert_non_null(output);
    int hardware_status = test_hardware_setup();
    if (hardware_status) {
        test_hardware_cleanup();
        fclose(output);
        fail_msg("Cannot create fake sysfs files");
    }
    fflush(NULL);
    pid_t child = fork();
    if (child < 0) {
        test_hardware_cleanup();
        fclose(output);
        fail_msg("Cannot fork CPUFreq case");
    }
    if (child == 0) {
        if (dup2(fileno(output), STDOUT_FILENO) < 0 || dup2(fileno(output), STDERR_FILENO) < 0) {
            _exit(EXIT_FAILURE);
        }
        signal(SIGALRM, SIG_DFL);
        alarm(10);
        unsetenv("CMOCKA_MESSAGE_OUTPUT");
        cmocka_set_message_output(CM_OUTPUT_STDOUT);
        const struct CMUnitTest tests[] = {{.name = name, .test_func = scenario}};
        int result                      = cmocka_run_group_tests_name("isolated CPUFreq case", tests, NULL, NULL);
        fflush(NULL);
        _exit(result ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    /* Parent owns fake files, so cleanup also runs after a watchdog kill. */
    test_hardware_cleanup();
    char line[1024], detail[1024] = "CPUFreq child failed without an assertion";
    char source[512] = __FILE__;
    int source_line  = __LINE__;
    rewind(output);
    while (fgets(line, sizeof(line), output)) {
        char *error = strstr(line, "[  ERROR   ] --- ");
        if (error) {
            snprintf(detail, sizeof(detail), "%s", error + strlen("[  ERROR   ] --- "));
            detail[strcspn(detail, "\r\n")] = '\0';
            if (fgets(line, sizeof(line), output)) {
                char *location = strstr(line, "[   LINE   ] --- ");
                if (location) {
                    sscanf(location + strlen("[   LINE   ] --- "), "%511[^:]:%d", source, &source_line);
                }
            }
            break;
        }
    }
    fclose(output);
    assert_int_equal(waited, child);
    if (WIFSIGNALED(status)) {
        fail_msg("CPUFreq case terminated by signal %d (SIGALRM means watchdog timeout)", WTERMSIG(status));
    }
    assert_true(WIFEXITED(status));
    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
        /* Preserve original assertion text and location, without reporting the
         * child suite or a redundant exit-status assertion. */
        cmocka_print_error("%s\n", detail);
        _fail(source, source_line);
    }
}
