/***************************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L.
 *
 **************************************************************************/

/**
 * @file test_powercap_context_locks.c
 * @brief Unit tests for PowerCap access to PowerMon contexts.
 *
 * This test suite validates that PowerCap code accesses PowerMon application
 * contexts using the expected locking contract.
 *
 * The tests focus on:
 * - Safe access to PowerMon contexts from PowerCap code.
 * - Correct handling of busy or unavailable contexts.
 * - Prevention of lock leaks on failure and continue paths.
 * - Protection against stale context access during concurrent job teardown.
 *
 * The tests use the real PowerMon locking helpers where possible.
 */

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include <pthread.h>

#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_mgt.h>

void update_node_powercap_opt_shared_info(void);

typedef struct {
    pthread_barrier_t acquired;
    pthread_barrier_t release;
    pthread_mutex_t *mutex;
} context_owner_args_t;

uint frequency_get_num_pstates(void)
{
    return 4;
}

ulong frequency_pstate_to_freq(uint pstate)
{
    static const ulong freqs[] = {3000000, 2500000, 2000000, 1500000};

    return freqs[pstate];
}

static void *context_owner(void *arg)
{
    context_owner_args_t *args = arg;

    pthread_mutex_lock(args->mutex);

    pthread_barrier_wait(&args->acquired);
    pthread_barrier_wait(&args->release);

    pthread_mutex_unlock(args->mutex);

    return NULL;
}

static void test_pmgt_get_app_req_freq_continue_releases_context(void **state)
{
    (void) state;

    powermon_app_t context           = {0};
    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    ulong freqs[MAX_CPUS_SUPPORTED] = {0};

    /*
     * Force the "sbatch with other steps inside" path:
     *
     *   app.is_mpi == 0
     *   is_job     == 1
     *   job is not present in jobs_in_node_list
     */
    context.app.job.id      = 12345;
    context.app.job.step_id = 0;
    context.app.is_mpi      = 0;
    context.is_job          = 1;

    contexts[1] = &context;

    powermon_test_set_num_contexts(1);
    powermon_test_set_max_context_created(1);

    pmgt_get_app_req_freq(DOMAIN_CPU, freqs, MAX_CPUS_SUPPORTED);

    /*
     * The continue path must have released the context mutex.
     */
    assert_int_equal(pthread_mutex_trylock(&context_mutexes[1]), 0);

    pthread_mutex_unlock(&context_mutexes[1]);

    /*
     * app_lock must also be available.
     */
    assert_int_equal(pthread_mutex_trylock(powermon_test_app_lock()), 0);

    pthread_mutex_unlock(powermon_test_app_lock());

    contexts[1] = NULL;
    powermon_test_set_num_contexts(0);
    powermon_test_set_max_context_created(0);
}

static void test_pmgt_get_app_req_freq_busy_context(void **state)
{
    (void) state;

    powermon_app_t context           = {0};
    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    pthread_t thread;
    context_owner_args_t args;

    ulong freqs[MAX_CPUS_SUPPORTED] = {0};

    contexts[1] = &context;

    powermon_test_set_num_contexts(1);
    powermon_test_set_max_context_created(1);

    args.mutex = &context_mutexes[1];

    assert_int_equal(pthread_barrier_init(&args.acquired, NULL, 2), 0);
    assert_int_equal(pthread_barrier_init(&args.release, NULL, 2), 0);

    assert_int_equal(pthread_create(&thread, NULL, context_owner, &args), 0);

    pthread_barrier_wait(&args.acquired);

    /*
     * Context 1 is busy. The function must skip it without
     * dereferencing the context or leaking app_lock.
     */
    pmgt_get_app_req_freq(DOMAIN_CPU, freqs, MAX_CPUS_SUPPORTED);

    assert_int_equal(pthread_mutex_trylock(powermon_test_app_lock()), 0);
    pthread_mutex_unlock(powermon_test_app_lock());

    pthread_barrier_wait(&args.release);
    pthread_join(thread, NULL);

    pthread_barrier_destroy(&args.acquired);
    pthread_barrier_destroy(&args.release);

    contexts[1] = NULL;
    powermon_test_set_num_contexts(0);
    powermon_test_set_max_context_created(0);
}

static void test_powercap_shared_info_busy_context(void **state)
{
    (void) state;

    powermon_app_t context_busy = {0};
    powermon_app_t context_free = {0};

    settings_conf_t settings_busy = {0};
    settings_conf_t settings_free = {0};

    resched_t resched_busy = {0};
    resched_t resched_free = {0};

    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    pthread_t thread;
    context_owner_args_t args;

    context_busy.settings = &settings_busy;
    context_busy.resched  = &resched_busy;

    context_free.settings = &settings_free;
    context_free.resched  = &resched_free;

    contexts[1] = &context_busy;
    contexts[2] = &context_free;

    powermon_test_set_num_contexts(2);
    powermon_test_set_max_context_created(2);

    args.mutex = &context_mutexes[1];

    assert_int_equal(pthread_barrier_init(&args.acquired, NULL, 2), 0);
    assert_int_equal(pthread_barrier_init(&args.release, NULL, 2), 0);

    assert_int_equal(pthread_create(&thread, NULL, context_owner, &args), 0);

    pthread_barrier_wait(&args.acquired);

    /*
     * Context 1 is busy, while context 2 is available.
     */
    update_node_powercap_opt_shared_info();

    /*
     * Busy context must not have been modified.
     */
    assert_int_equal(resched_busy.force_rescheduling, 0);

    /*
     * Free context must have been updated.
     */
    assert_int_equal(resched_free.force_rescheduling, 1);

    /*
     * The global lock must not have been leaked.
     */
    assert_int_equal(pthread_mutex_trylock(powermon_test_app_lock()), 0);
    pthread_mutex_unlock(powermon_test_app_lock());

    /*
     * Context 2 mutex must also have been released.
     */
    assert_int_equal(pthread_mutex_trylock(&context_mutexes[2]), 0);
    pthread_mutex_unlock(&context_mutexes[2]);

    pthread_barrier_wait(&args.release);
    pthread_join(thread, NULL);

    pthread_barrier_destroy(&args.acquired);
    pthread_barrier_destroy(&args.release);

    contexts[1] = NULL;
    contexts[2] = NULL;

    powermon_test_set_num_contexts(0);
    powermon_test_set_max_context_created(0);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pmgt_get_app_req_freq_busy_context),
        cmocka_unit_test(test_pmgt_get_app_req_freq_continue_releases_context),
        cmocka_unit_test(test_powercap_shared_info_busy_context),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
