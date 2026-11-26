/***************************************************************************
 * Copyright (c) 2026 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef CPUFREQ_TEST_FIXTURE_H
#define CPUFREQ_TEST_FIXTURE_H

#include <management/cpufreq/cpufreq.h>
#include <pthread.h>

#define TEST_CPUS 2

typedef struct test_snapshot {
    ullong storage[TEST_CPUS];
    const ullong *values;
} test_snapshot_t;

/* Keep the old pointer alive: copying here would hide the ownership bug. */
static inline state_t test_read_shared(state_t (*get)(const ullong **), test_snapshot_t *snapshot)
{
    return get(&snapshot->values);
}

static inline state_t test_read_owned(state_t (*get)(ullong *), test_snapshot_t *snapshot)
{
    snapshot->values = snapshot->storage;
    return get(snapshot->storage);
}

/* Compile unchanged before and after the caller-owned driver API change. */
#define test_read(get, snapshot)                                                                                       \
    _Generic((get), state_t(*)(ullong *): test_read_owned, default: test_read_shared)((get), (snapshot))

/* Type of the public driver's output parameter, for pass-through wrappers. */
typedef __typeof__(_Generic(((mgt_ps_driver_ops_t *) 0)->get_current_list,
                       state_t (*)(ullong *): (ullong *) 0,
                       default: (const ullong **) 0)) test_driver_output_t;

enum {
    READER_A = 1,
    READER_B = 2
};

/* Each scenario owns this context on its stack. Wrappers access only the
 * context attached to their worker, never another test's operations or data. */
typedef struct test_context {
    mgt_ps_driver_ops_t driver;
    mgt_ps_ops_t arch;
    int frequency_fds[TEST_CPUS], governor_fds[TEST_CPUS];
    pthread_mutex_t gate;
    pthread_cond_t changed;
    pthread_mutex_t *driver_mutex;
    int watched_fd, pause_seek, pause_default, a_waiting, release_a;
    int b_lock_attempt, b_done, lock_probe, gate_error;
} test_context_t;

typedef struct test_reader {
    test_context_t *context;
    int role;
    state_t (*get_frequencies)(test_driver_output_t out);
    state_t (*get_governors)(uint *out);
    state_t (*get_pstates)(ctx_t *ctx, pstate_t *out);
    state_t status;
    test_snapshot_t snapshot;
    uint governors[TEST_CPUS];
    pstate_t pstates[TEST_CPUS];
} test_reader_t;

int test_context_init(test_context_t *context);
int test_context_destroy(test_context_t *context);
int test_bind_files(test_context_t *context, const char *frequency_attribute);
int test_write_file(int fd, const char *contents);
int test_set_frequencies(test_context_t *context, ullong cpu0, ullong cpu1);
int test_wait_for_a(test_context_t *context);
int test_wait_for_b(test_context_t *context);
void test_release_a(test_context_t *context);
void *test_read_worker(void *argument);
state_t test_scheduled_intel_read(test_driver_output_t out);
state_t test_scheduled_acpi_read(test_driver_output_t out);
void test_expect_file(int fd, const char *expected);

typedef struct test_case {
    const char *name;
    void (*scenario)(void **);
} test_case_t;

void test_run_isolated(void **state);

/* Registration only: the named function below contains the entire scenario. */
#define ISOLATED_TEST(function)                                                                                        \
    {                                                                                                                  \
        .name = #function, .test_func = test_run_isolated, .initial_state = &(test_case_t)                             \
        {                                                                                                              \
            #function, function                                                                                        \
        }                                                                                                              \
    }

int test_hardware_setup(void);
void test_hardware_cleanup(void);
int test_hardware_fd(const char *attribute, int cpu);

#endif
