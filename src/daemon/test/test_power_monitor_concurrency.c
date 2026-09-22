#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>
#include <daemon/power_monitor.h>
#include <daemon/power_monitor_lock.h>

/**

* @file test_power_monitor_concurrency.c
* @brief Concurrency and context lifetime tests for the PowerMon daemon.
*
* This test suite exercises production code from power_monitor.c to validate
* the synchronization rules used by the PowerMon and RemoteAPI threads.
*
* The tests focus on the lifecycle of entries stored in current_ear_app and
* the interaction between the global application lock and the per-context
* mutexes.
*
* The main scenarios covered are:
* * Safe detachment of an existing application context.
* * Correct cleanup of context counters and vector entries.
* * Failure to detach a context while another thread owns its mutex.
* * Release of the global application lock on all failure paths.
* * Concurrent access to a context during job finalization.
* * Prevention of stale context access after a context has been detached.
*
* These tests are intended to reproduce, in a deterministic way, the race
* conditions that can occur between PowerMon monitoring operations and
* RemoteAPI job finalization.
*
* Synchronization primitives such as pthread barriers are used where needed
* to force specific thread interleavings without relying on timing or sleeps.
  */

static void test_detach_existing_context(void **state)
{
    (void) state;

    powermon_app_t context           = {0};
    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    int curr_ctx;
    powermon_app_t *pmapp;

    context.app.job.id      = 10;
    context.app.job.step_id = 0;

    contexts[1] = &context;

    powermon_test_set_num_contexts(1);
    powermon_test_set_max_context_created(1);

    assert_int_equal(powermon_test_detach_context(10, 0, 0, &curr_ctx, &pmapp), EAR_SUCCESS);

    assert_int_equal(curr_ctx, 1);
    assert_ptr_equal(pmapp, &context);

    assert_null(contexts[1]);

    assert_int_equal(powermon_test_num_contexts(), 0);
    assert_int_equal(powermon_test_max_context_created(), 0);

    /* The global lock must have been released. */
    assert_int_equal(pthread_mutex_trylock(powermon_test_app_lock()), 0);
    pthread_mutex_unlock(powermon_test_app_lock());

    /* The context lock must remain owned by the caller. */
    assert_int_not_equal(pthread_mutex_trylock(&context_mutexes[1]), 0);

    pthread_mutex_unlock(&context_mutexes[1]);
}

static void test_detach_busy_context_releases_app_lock(void **state)
{
    (void) state;

    powermon_app_t context           = {0};
    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    int curr_ctx;
    powermon_app_t *pmapp;

    context.app.job.id      = 20;
    context.app.job.step_id = 0;

    contexts[1] = &context;

    powermon_test_set_num_contexts(1);
    powermon_test_set_max_context_created(1);

    /* Simulate another thread using the context. */
    assert_int_equal(pthread_mutex_lock(&context_mutexes[1]), 0);

    assert_int_equal(powermon_test_detach_context(20, 0, 0, &curr_ctx, &pmapp), EAR_ERROR);

    /* The context must remain registered. */
    assert_ptr_equal(contexts[1], &context);
    assert_int_equal(powermon_test_num_contexts(), 1);
    assert_int_equal(powermon_test_max_context_created(), 1);

    /* app_lock must not leak on the failure path. */
    assert_int_equal(pthread_mutex_trylock(powermon_test_app_lock()), 0);
    pthread_mutex_unlock(powermon_test_app_lock());

    pthread_mutex_unlock(&context_mutexes[1]);

    /* Test cleanup. */
    contexts[1] = NULL;
    powermon_test_set_num_contexts(0);
    powermon_test_set_max_context_created(0);
}

typedef struct {
    pthread_barrier_t acquired;
    pthread_barrier_t release;
    pthread_mutex_t *mutex;
} context_owner_args_t;

static void *context_owner(void *arg)
{
    context_owner_args_t *args = arg;

    pthread_mutex_lock(args->mutex);

    pthread_barrier_wait(&args->acquired);
    pthread_barrier_wait(&args->release);

    pthread_mutex_unlock(args->mutex);

    return NULL;
}

static void test_detach_while_context_is_used(void **state)
{
    (void) state;

    powermon_app_t context           = {0};
    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    pthread_t thread;
    context_owner_args_t args;

    int curr_ctx;
    powermon_app_t *pmapp;

    context.app.job.id      = 30;
    context.app.job.step_id = 0;

    contexts[1] = &context;
    powermon_test_set_num_contexts(1);
    powermon_test_set_max_context_created(1);

    args.mutex = &context_mutexes[1];

    assert_int_equal(pthread_barrier_init(&args.acquired, NULL, 2), 0);
    assert_int_equal(pthread_barrier_init(&args.release, NULL, 2), 0);

    assert_int_equal(pthread_create(&thread, NULL, context_owner, &args), 0);

    /* Wait until the worker owns the context mutex. */
    pthread_barrier_wait(&args.acquired);

    assert_int_equal(powermon_test_detach_context(30, 0, 0, &curr_ctx, &pmapp), EAR_ERROR);

    assert_ptr_equal(contexts[1], &context);

    /* app_lock must have been released. */
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

static void test_detached_context_cannot_be_reacquired(void **state)
{
    (void) state;

    powermon_app_t context           = {0};
    powermon_app_t **contexts        = powermon_test_contexts();
    pthread_mutex_t *context_mutexes = powermon_test_context_mutexes();

    int curr_ctx;
    powermon_app_t *pmapp        = NULL;
    powermon_app_t *reader_pmapp = NULL;

    context.app.job.id      = 40;
    context.app.job.step_id = 0;

    contexts[1] = &context;
    powermon_test_set_num_contexts(1);
    powermon_test_set_max_context_created(1);

    assert_int_equal(powermon_test_detach_context(40, 0, 0, &curr_ctx, &pmapp), EAR_SUCCESS);

    assert_ptr_equal(pmapp, &context);
    assert_null(contexts[1]);

    /*
     * The detached context mutex is still held by the detach caller.
     * Release it as powermon_end_job() eventually does.
     */
    pthread_mutex_unlock(&context_mutexes[1]);

    assert_int_not_equal(powermon_context_trylock_impl(powermon_test_app_lock(), context_mutexes, contexts, 1,
                                                       MAX_NESTED_LEVELS, &reader_pmapp),
                         EAR_SUCCESS);

    assert_null(reader_pmapp);

    /*
     * The failed lookup must not leak app_lock.
     */
    assert_int_equal(pthread_mutex_trylock(powermon_test_app_lock()), 0);
    pthread_mutex_unlock(powermon_test_app_lock());
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_detach_existing_context),
        cmocka_unit_test(test_detach_busy_context_releases_app_lock),
        cmocka_unit_test(test_detach_while_context_is_used),
        cmocka_unit_test(test_detached_context_cannot_be_reacquired),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
