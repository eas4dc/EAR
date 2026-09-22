/***************************************************************************
 * Copyright (c) 2026 Energy Aware Solutions, S.L
 ***************************************************************************/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>
#include <pthread.h>
#include <string.h>

#include <daemon/power_monitor_lock.h>

/*
 * ### Power monitor context locking tests

These tests validate the locking contract used when accessing PowerMon application contexts. The main goal is to ensure
that the global `app_lock` is always released and that the per-context mutex is only retained when a valid context has
been successfully acquired.

1. **Empty context slot**

   Verifies the behavior when the requested entry in `current_ear_app` is `NULL`.

   The operation must fail without returning a context, and `app_lock` must be released before returning. This is a
regression test for the lock leak that caused the PowerMon thread to later fail when trying to reacquire `app_lock`.

2. **Context mutex already locked**

   Verifies the behavior when the requested context exists but its per-context mutex cannot be acquired.

   The operation must fail without returning the context, and the previously acquired `app_lock` must be released. This
ensures that contention on a context cannot leave the global application lock held.

3. **Successful context acquisition**

   Verifies the normal successful path when both the context and its mutex are available.

   The returned pointer must reference the requested context, `app_lock` must be released, and the per-context mutex
must remain locked. This guarantees that the caller can safely use the returned context while preventing its concurrent
destruction or modification.
*/

/**

* @file test_power_monitor_locks.c
* @brief Unit tests for PowerMon context locking primitives.
*
* This test suite validates the locking contract used to safely access
* entries in the PowerMon application context vector.
*
* The tests focus on:
* * Correct acquisition and release of the global application lock.
* * Correct handling of empty context slots.
* * Correct behavior when a per-context mutex is already owned.
* * Preservation of the per-context mutex on successful acquisition.
* * Deterministic multi-threaded contention scenarios.
* * Prevention of lock leaks on failure paths.
*
* The main goal is to guarantee that the global application lock is never
* left acquired after a failed lookup or lock attempt, while ensuring that
* a successfully acquired context remains protected by its own mutex.
*
* These tests exercise the production helper implemented in
* power_monitor_lock.c.
  */

typedef struct {
    pthread_mutex_t *app_lock;
    pthread_mutex_t *context_mutex;
    powermon_app_t **contexts;
    pthread_barrier_t *barrier;
} lock_test_args_t;

typedef struct {
    pthread_mutex_t *app_lock;
    pthread_mutex_t *context_mutex;
    powermon_app_t **contexts;
    pthread_barrier_t *barrier;
} remove_test_args_t;

typedef struct {
    pthread_mutex_t *app_lock;
    pthread_mutex_t *context_mutex;
    powermon_app_t **contexts;
    pthread_barrier_t *barrier;
} lifetime_test_args_t;

typedef struct {
    pthread_mutex_t *app_lock;
    pthread_barrier_t *barrier;
} app_lock_test_args_t;

static void *hold_app_lock(void *arg)
{
    app_lock_test_args_t *args = arg;

    pthread_mutex_lock(args->app_lock);

    /* Notify the test that app_lock is owned. */
    pthread_barrier_wait(args->barrier);

    /* Wait until the test has attempted the acquisition. */
    pthread_barrier_wait(args->barrier);

    pthread_mutex_unlock(args->app_lock);

    return NULL;
}

static void test_context_trylock_when_app_lock_is_busy(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    pthread_barrier_t barrier;
    pthread_t thread;

    int dummy_context;

    powermon_app_t *contexts[2] = {NULL, (powermon_app_t *) &dummy_context};

    powermon_app_t *pmapp = NULL;
    state_t ret;
    int lock_ret;

    app_lock_test_args_t args = {.app_lock = &app_lock, .barrier = &barrier};

    (void) state;

    assert_int_equal(pthread_barrier_init(&barrier, NULL, 2), 0);
    assert_int_equal(pthread_create(&thread, NULL, hold_app_lock, &args), 0);

    /*
     * Wait until the worker owns app_lock.
     */
    pthread_barrier_wait(&barrier);

    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_not_equal(ret, EAR_SUCCESS);
    assert_null(pmapp);

    /*
     * The context mutex must not have been touched.
     */
    lock_ret = pthread_mutex_trylock(&context_mutex[1]);
    assert_int_equal(lock_ret, 0);
    pthread_mutex_unlock(&context_mutex[1]);

    /*
     * Allow the worker to release app_lock.
     */
    pthread_barrier_wait(&barrier);

    pthread_join(thread, NULL);

    /*
     * app_lock must be available again after the owner releases it.
     */
    lock_ret = pthread_mutex_trylock(&app_lock);
    assert_int_equal(lock_ret, 0);
    pthread_mutex_unlock(&app_lock);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

static void *remove_context_waiting_for_reader(void *arg)
{
    lifetime_test_args_t *args = arg;

    pthread_mutex_lock(args->app_lock);

    /*
     * Notify the test that the remover is ready.
     */
    pthread_barrier_wait(args->barrier);

    /*
     * This must block while the reader owns context_mutex[1].
     */
    pthread_mutex_lock(&args->context_mutex[1]);

    args->contexts[1] = NULL;

    pthread_mutex_unlock(args->app_lock);
    pthread_mutex_unlock(&args->context_mutex[1]);

    return NULL;
}

static void test_context_cannot_be_removed_while_in_use(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    pthread_barrier_t barrier;
    pthread_t thread;

    int dummy_context;

    powermon_app_t *contexts[2] = {NULL, (powermon_app_t *) &dummy_context};

    powermon_app_t *pmapp = NULL;
    state_t ret;

    lifetime_test_args_t args = {
        .app_lock = &app_lock, .context_mutex = context_mutex, .contexts = contexts, .barrier = &barrier};

    (void) state;

    /*
     * Reader acquires the context.
     * On success, context_mutex[1] remains locked.
     */
    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_equal(ret, EAR_SUCCESS);
    assert_ptr_equal(pmapp, contexts[1]);

    assert_int_equal(pthread_barrier_init(&barrier, NULL, 2), 0);
    assert_int_equal(pthread_create(&thread, NULL, remove_context_waiting_for_reader, &args), 0);

    /*
     * The remover now owns app_lock and is about to wait for
     * context_mutex[1].
     */
    pthread_barrier_wait(&barrier);

    /*
     * The context must still be valid while the reader owns its mutex.
     */
    assert_non_null(contexts[1]);
    assert_ptr_equal(pmapp, contexts[1]);

    /*
     * Release the reader ownership. The remover may now continue.
     */
    pthread_mutex_unlock(&context_mutex[1]);

    pthread_join(thread, NULL);

    assert_null(contexts[1]);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

static void *remove_context_thread(void *arg)
{
    remove_test_args_t *args = arg;

    pthread_mutex_lock(args->app_lock);

    /*
     * Synchronize with the test while holding app_lock.
     */
    pthread_barrier_wait(args->barrier);

    args->contexts[1] = NULL;

    pthread_mutex_unlock(args->app_lock);

    return NULL;
}

static void test_context_removed_before_acquisition(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    pthread_barrier_t barrier;
    pthread_t thread;

    int dummy_context;

    powermon_app_t *contexts[2] = {NULL, (powermon_app_t *) &dummy_context};

    powermon_app_t *pmapp = NULL;
    state_t ret;
    int lock_ret;

    remove_test_args_t args = {
        .app_lock = &app_lock, .context_mutex = context_mutex, .contexts = contexts, .barrier = &barrier};

    (void) state;

    assert_int_equal(pthread_barrier_init(&barrier, NULL, 2), 0);
    assert_int_equal(pthread_create(&thread, NULL, remove_context_thread, &args), 0);

    /*
     * The remover owns app_lock at this point.
     */
    pthread_barrier_wait(&barrier);

    /*
     * Wait until the remover finishes and releases app_lock.
     */
    pthread_join(thread, NULL);

    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_not_equal(ret, EAR_SUCCESS);
    assert_null(pmapp);

    /*
     * Removal/failure must not leave the global lock held.
     */
    lock_ret = pthread_mutex_trylock(&app_lock);
    assert_int_equal(lock_ret, 0);
    pthread_mutex_unlock(&app_lock);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

static void *hold_context_lock(void *arg)
{
    lock_test_args_t *args = arg;

    pthread_mutex_lock(&args->context_mutex[1]);

    /*
     * Notify the test thread that the context mutex is already owned.
     */
    pthread_barrier_wait(args->barrier);

    /*
     * Wait until the test has attempted to acquire the context.
     */
    pthread_barrier_wait(args->barrier);

    pthread_mutex_unlock(&args->context_mutex[1]);

    return NULL;
}

static void test_context_trylock_concurrent_context_owner(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    pthread_barrier_t barrier;
    pthread_t thread;

    int dummy_context;

    powermon_app_t *contexts[2] = {NULL, (powermon_app_t *) &dummy_context};

    powermon_app_t *pmapp = NULL;
    state_t ret;
    int lock_ret;

    lock_test_args_t args = {
        .app_lock = &app_lock, .context_mutex = context_mutex, .contexts = contexts, .barrier = &barrier};

    (void) state;

    assert_int_equal(pthread_barrier_init(&barrier, NULL, 2), 0);
    assert_int_equal(pthread_create(&thread, NULL, hold_context_lock, &args), 0);

    /*
     * Wait until the worker owns context_mutex[1].
     */
    pthread_barrier_wait(&barrier);

    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_not_equal(ret, EAR_SUCCESS);
    assert_null(pmapp);

    /*
     * The failed context acquisition must not leave app_lock held.
     */
    lock_ret = pthread_mutex_trylock(&app_lock);
    assert_int_equal(lock_ret, 0);
    pthread_mutex_unlock(&app_lock);

    /*
     * Allow the worker to release the context mutex.
     */
    pthread_barrier_wait(&barrier);

    pthread_join(thread, NULL);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

static void test_context_trylock_releases_app_lock_on_empty_slot(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    int dummy_context;

    powermon_app_t *contexts[2] = {(powermon_app_t *) &dummy_context, NULL};

    powermon_app_t *pmapp = NULL;
    state_t ret;
    int lock_ret;

    (void) state;

    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_not_equal(ret, EAR_SUCCESS);
    assert_null(pmapp);

    lock_ret = pthread_mutex_trylock(&app_lock);
    assert_int_equal(lock_ret, 0);

    pthread_mutex_unlock(&app_lock);
    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

static void test_context_trylock_releases_app_lock_when_context_is_busy(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    int dummy_context;

    powermon_app_t *contexts[2] = {NULL, (powermon_app_t *) &dummy_context};

    powermon_app_t *pmapp = NULL;
    state_t ret;
    int lock_ret;

    (void) state;

    /*
     * Simulate another thread currently owning the context lock.
     */
    assert_int_equal(pthread_mutex_lock(&context_mutex[1]), 0);

    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_not_equal(ret, EAR_SUCCESS);
    assert_null(pmapp);

    /*
     * Even if the context lock cannot be acquired,
     * app_lock must always be released.
     */
    lock_ret = pthread_mutex_trylock(&app_lock);
    assert_int_equal(lock_ret, 0);

    pthread_mutex_unlock(&app_lock);
    pthread_mutex_unlock(&context_mutex[1]);

    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

static void test_context_trylock_success_holds_context_and_releases_app_lock(void **state)
{
    pthread_mutex_t app_lock         = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t context_mutex[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

    int dummy_context;

    powermon_app_t *contexts[2] = {NULL, (powermon_app_t *) &dummy_context};

    powermon_app_t *pmapp = NULL;
    state_t ret;
    int lock_ret;

    (void) state;

    ret = powermon_context_trylock_impl(&app_lock, context_mutex, contexts, 1, 2, &pmapp);

    assert_int_equal(ret, EAR_SUCCESS);
    assert_ptr_equal(pmapp, contexts[1]);

    /*
     * app_lock must already be released.
     */
    lock_ret = pthread_mutex_trylock(&app_lock);
    assert_int_equal(lock_ret, 0);
    pthread_mutex_unlock(&app_lock);

    /*
     * The context mutex must remain locked for the caller.
     */
    lock_ret = pthread_mutex_trylock(&context_mutex[1]);
    assert_int_not_equal(lock_ret, 0);

    pthread_mutex_unlock(&context_mutex[1]);

    pthread_mutex_destroy(&context_mutex[0]);
    pthread_mutex_destroy(&context_mutex[1]);
    pthread_mutex_destroy(&app_lock);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_context_trylock_releases_app_lock_on_empty_slot),
        cmocka_unit_test(test_context_trylock_releases_app_lock_when_context_is_busy),
        cmocka_unit_test(test_context_trylock_success_holds_context_and_releases_app_lock),
        cmocka_unit_test(test_context_trylock_concurrent_context_owner),
        cmocka_unit_test(test_context_removed_before_acquisition),
        cmocka_unit_test(test_context_cannot_be_removed_while_in_use),
        cmocka_unit_test(test_context_trylock_when_app_lock_is_busy),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
