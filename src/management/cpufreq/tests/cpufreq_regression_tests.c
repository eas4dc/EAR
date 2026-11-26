/***************************************************************************
 * Copyright (c) 2026 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include "cpufreq_fixture.h"
#include <cmocka.h>
#include <errno.h>
#include <management/cpufreq/archs/default.h>
#include <management/cpufreq/drivers/acpi_cpufreq.h>
#include <management/cpufreq/drivers/intel_pstate.h>

/* Every named test owns its context, inputs, target calls and expectations.
 * test_support.c supplies scheduling and isolation, not scenario policy. */

static void test_intel_independent_results(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_intel_pstate_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    assert_int_equal(context.driver.init(), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_max_freq"), 0);

    test_snapshot_t first, second;
    assert_int_equal(test_set_frequencies(&context, 2400000, 1800000), 0);
    assert_int_equal(test_read(mgt_intel_pstate_get_current_list, &first), EAR_SUCCESS);

    /* A second call completes before the first caller consumes its result. */
    assert_int_equal(test_set_frequencies(&context, 1800000, 2400000), 0);
    assert_int_equal(test_read(mgt_intel_pstate_get_current_list, &second), EAR_SUCCESS);

    assert_non_null(first.values);
    assert_non_null(second.values);
    assert_int_equal(first.values[0], 2400000);
    assert_int_equal(first.values[1], 1800000);
    assert_int_equal(second.values[0], 1800000);
    assert_int_equal(second.values[1], 2400000);
    assert_int_equal(context.driver.dispose(), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_acpi_independent_results(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_acpi_cpufreq_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    assert_int_equal(context.driver.init(), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_setspeed"), 0);

    test_snapshot_t first, second;
    assert_int_equal(test_set_frequencies(&context, 2400000, 1800000), 0);
    assert_int_equal(test_read(mgt_acpi_cpufreq_get_current_list, &first), EAR_SUCCESS);

    /* A second call completes before the first caller consumes its result. */
    assert_int_equal(test_set_frequencies(&context, 1800000, 2400000), 0);
    assert_int_equal(test_read(mgt_acpi_cpufreq_get_current_list, &second), EAR_SUCCESS);

    assert_non_null(first.values);
    assert_non_null(second.values);
    assert_int_equal(first.values[0], 2400000);
    assert_int_equal(first.values[1], 1800000);
    assert_int_equal(second.values[0], 1800000);
    assert_int_equal(second.values[1], 2400000);
    assert_int_equal(context.driver.dispose(), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_intel_concurrent_frequency_readers(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_intel_pstate_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    assert_int_equal(context.driver.init(), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_max_freq"), 0);
    assert_int_equal(test_set_frequencies(&context, 2400000, 1800000), 0);

    pthread_t a_thread, b_thread;
    test_reader_t a    = {.context = &context, .role = READER_A, .get_frequencies = mgt_intel_pstate_get_current_list};
    test_reader_t b    = {.context = &context, .role = READER_B, .get_frequencies = mgt_intel_pstate_get_current_list};
    context.pause_seek = 1;
    context.watched_fd = context.frequency_fds[0];

    /* A stops between seek and read. B either consumes the shared descriptor
     * or waits for the driver lock. Release A in either case, then join both. */
    assert_int_equal(pthread_create(&a_thread, NULL, test_read_worker, &a), 0);
    int error     = test_wait_for_a(&context);
    int created_b = error ? error : pthread_create(&b_thread, NULL, test_read_worker, &b);
    if (!created_b) {
        error = test_wait_for_b(&context);
    }
    test_release_a(&context);
    int joined_a = pthread_join(a_thread, NULL);
    int joined_b = created_b ? 0 : pthread_join(b_thread, NULL);

    /* Assert only after releasing and joining all workers, including errors. */
    assert_int_equal(error, 0);
    assert_int_equal(created_b, 0);
    assert_int_equal(joined_a, 0);
    assert_int_equal(joined_b, 0);
    assert_int_equal(context.gate_error, 0);
    if (context.b_lock_attempt) {
        assert_int_equal(context.lock_probe, EBUSY);
    }
    assert_int_equal(a.status, EAR_SUCCESS);
    assert_int_equal(b.status, EAR_SUCCESS);
    assert_non_null(a.snapshot.values);
    assert_non_null(b.snapshot.values);
    assert_int_equal(a.snapshot.values[0], 2400000);
    assert_int_equal(a.snapshot.values[1], 1800000);
    assert_int_equal(b.snapshot.values[0], 2400000);
    assert_int_equal(b.snapshot.values[1], 1800000);
    assert_int_equal(context.driver.dispose(), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_intel_concurrent_governor_readers(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_intel_pstate_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    assert_int_equal(context.driver.init(), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_max_freq"), 0);
    for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
        assert_int_equal(test_write_file(context.governor_fds[cpu], "performance\n"), 0);
    }

    pthread_t a_thread, b_thread;
    test_reader_t a    = {.context = &context, .role = READER_A, .get_governors = mgt_intel_pstate_governor_get};
    test_reader_t b    = {.context = &context, .role = READER_B, .get_governors = mgt_intel_pstate_governor_get_list};
    context.pause_seek = 1;
    context.watched_fd = context.governor_fds[0];

    /* A stops between seek and read. B either consumes the shared descriptor
     * or waits for the driver lock. Release A in either case, then join both. */
    assert_int_equal(pthread_create(&a_thread, NULL, test_read_worker, &a), 0);
    int error     = test_wait_for_a(&context);
    int created_b = error ? error : pthread_create(&b_thread, NULL, test_read_worker, &b);
    if (!created_b) {
        error = test_wait_for_b(&context);
    }
    test_release_a(&context);
    int joined_a = pthread_join(a_thread, NULL);
    int joined_b = created_b ? 0 : pthread_join(b_thread, NULL);

    /* Assert only after releasing and joining all workers, including errors. */
    assert_int_equal(error, 0);
    assert_int_equal(created_b, 0);
    assert_int_equal(joined_a, 0);
    assert_int_equal(joined_b, 0);
    assert_int_equal(context.gate_error, 0);
    if (context.b_lock_attempt) {
        assert_int_equal(context.lock_probe, EBUSY);
    }
    assert_int_equal(a.status, EAR_SUCCESS);
    assert_int_equal(b.status, EAR_SUCCESS);
    assert_int_equal(a.governors[0], Governor.performance);
    assert_int_equal(b.governors[0], Governor.performance);
    assert_int_equal(b.governors[1], Governor.performance);
    assert_int_equal(context.driver.dispose(), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_intel_default_snapshots(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_intel_pstate_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    /* Pause only after the real driver getter returns; do not copy results. */
    context.driver.get_current_list = test_scheduled_intel_read;
    assert_int_equal(mgt_cpufreq_default_load(&topology, &context.arch, &context.driver), EAR_SUCCESS);
    assert_non_null(context.arch.init);
    assert_int_equal(context.arch.init(NULL), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_max_freq"), 0);
    uint high_index, low_index;
    assert_int_equal(context.arch.get_index(NULL, 2400000, &high_index, 0), EAR_SUCCESS);
    assert_int_equal(context.arch.get_index(NULL, 1800000, &low_index, 0), EAR_SUCCESS);
    assert_int_equal(test_set_frequencies(&context, 2400000, 1800000), 0);

    pthread_t thread;
    test_reader_t first = {.context = &context, .role = READER_A, .get_pstates = mgt_cpufreq_default_get_current_list};
    pstate_t second[TEST_CPUS];
    context.pause_default = 1;

    /* Pause A after its driver read, before default.c converts the result.
     * B reads changed frequencies and completes before A resumes conversion. */
    assert_int_equal(pthread_create(&thread, NULL, test_read_worker, &first), 0);
    int error             = test_wait_for_a(&context);
    state_t second_status = EAR_ERROR;
    if (!error) {
        error = test_set_frequencies(&context, 1800000, 2400000);
    }
    if (!error) {
        second_status = mgt_cpufreq_default_get_current_list(NULL, second);
    }
    test_release_a(&context);
    int joined = pthread_join(thread, NULL);

    assert_int_equal(joined, 0);
    assert_int_equal(error, 0);
    assert_int_equal(context.gate_error, 0);
    assert_int_equal(first.status, EAR_SUCCESS);
    assert_int_equal(second_status, EAR_SUCCESS);
    /* Success alone permits fallback P-State 0: inspect every converted value. */
    assert_int_equal(first.pstates[0].idx, high_index);
    assert_int_equal(first.pstates[0].khz, 2400000);
    assert_int_equal(first.pstates[1].idx, low_index);
    assert_int_equal(first.pstates[1].khz, 1800000);
    assert_int_equal(second[0].idx, low_index);
    assert_int_equal(second[0].khz, 1800000);
    assert_int_equal(second[1].idx, high_index);
    assert_int_equal(second[1].khz, 2400000);
    assert_int_equal(context.arch.dispose(NULL), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_acpi_default_snapshots(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_acpi_cpufreq_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    /* Pause only after the real driver getter returns; do not copy results. */
    context.driver.get_current_list = test_scheduled_acpi_read;
    assert_int_equal(mgt_cpufreq_default_load(&topology, &context.arch, &context.driver), EAR_SUCCESS);
    assert_non_null(context.arch.init);
    assert_int_equal(context.arch.init(NULL), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_setspeed"), 0);
    uint high_index, low_index;
    assert_int_equal(context.arch.get_index(NULL, 2400000, &high_index, 0), EAR_SUCCESS);
    assert_int_equal(context.arch.get_index(NULL, 1800000, &low_index, 0), EAR_SUCCESS);
    assert_int_equal(test_set_frequencies(&context, 2400000, 1800000), 0);

    pthread_t thread;
    test_reader_t first = {.context = &context, .role = READER_A, .get_pstates = mgt_cpufreq_default_get_current_list};
    pstate_t second[TEST_CPUS];
    context.pause_default = 1;

    /* Pause A after its driver read, before default.c converts the result.
     * B reads changed frequencies and completes before A resumes conversion. */
    assert_int_equal(pthread_create(&thread, NULL, test_read_worker, &first), 0);
    int error             = test_wait_for_a(&context);
    state_t second_status = EAR_ERROR;
    if (!error) {
        error = test_set_frequencies(&context, 1800000, 2400000);
    }
    if (!error) {
        second_status = mgt_cpufreq_default_get_current_list(NULL, second);
    }
    test_release_a(&context);
    int joined = pthread_join(thread, NULL);

    assert_int_equal(joined, 0);
    assert_int_equal(error, 0);
    assert_int_equal(context.gate_error, 0);
    assert_int_equal(first.status, EAR_SUCCESS);
    assert_int_equal(second_status, EAR_SUCCESS);
    /* Success alone permits fallback P-State 0: inspect every converted value. */
    assert_int_equal(first.pstates[0].idx, high_index);
    assert_int_equal(first.pstates[0].khz, 2400000);
    assert_int_equal(first.pstates[1].idx, low_index);
    assert_int_equal(first.pstates[1].khz, 1800000);
    assert_int_equal(second[0].idx, low_index);
    assert_int_equal(second[0].khz, 1800000);
    assert_int_equal(second[1].idx, high_index);
    assert_int_equal(second[1].khz, 2400000);
    assert_int_equal(context.arch.dispose(NULL), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_intel_governor_setters_complete(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_intel_pstate_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    assert_int_equal(context.driver.init(), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_max_freq"), 0);
    for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
        assert_int_equal(test_write_file(context.governor_fds[cpu], "performance\n"), 0);
    }
    assert_int_equal(test_set_frequencies(&context, 2400000, 1800000), 0);

    /* Changing governor restores max frequency; last restores performance. */
    assert_int_equal(mgt_intel_pstate_governor_set(Governor.powersave), EAR_SUCCESS);
    for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
        test_expect_file(context.governor_fds[cpu], "powersave\n");
        test_expect_file(context.frequency_fds[cpu], "3000000\n");
    }
    assert_int_equal(mgt_intel_pstate_governor_set(Governor.last), EAR_SUCCESS);
    uint governor;
    assert_int_equal(mgt_intel_pstate_governor_get(&governor), EAR_SUCCESS);
    assert_int_equal(governor, Governor.performance);
    for (int cpu = 0; cpu < TEST_CPUS; ++cpu) {
        test_expect_file(context.governor_fds[cpu], "performance\n");
    }
    assert_int_equal(context.driver.dispose(), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

static void test_intel_frequency_setters_complete(void **state)
{
    (void) state;
    test_context_t context;
    cpu_t cpus[TEST_CPUS] = {{.id = 0}, {.id = 1}};
    topology_t topology   = {.vendor = VENDOR_INTEL, .cpu_count = TEST_CPUS, .cpus = cpus};
    assert_int_equal(test_context_init(&context), 0);
    mgt_intel_pstate_load(&topology, &context.driver);
    assert_non_null(context.driver.init);
    assert_int_equal(mgt_cpufreq_default_load(&topology, &context.arch, &context.driver), EAR_SUCCESS);
    assert_non_null(context.arch.init);
    assert_int_equal(context.arch.init(NULL), EAR_SUCCESS);
    assert_int_equal(test_bind_files(&context, "scaling_max_freq"), 0);
    uint indices[TEST_CPUS];
    assert_int_equal(context.arch.get_index(NULL, 2400000, &indices[0], 0), EAR_SUCCESS);
    assert_int_equal(context.arch.get_index(NULL, 1800000, &indices[1], 0), EAR_SUCCESS);
    assert_int_equal(test_set_frequencies(&context, 1800000, 2400000), 0);

    /* The list setter and following getter must both complete without a
     * recursive lock, and must apply the requested per-CPU frequencies. */
    assert_int_equal(mgt_intel_pstate_set_current_list(indices), EAR_SUCCESS);
    test_expect_file(context.frequency_fds[0], "2400000\n");
    test_expect_file(context.frequency_fds[1], "1800000\n");
    test_snapshot_t result;
    assert_int_equal(test_read(mgt_intel_pstate_get_current_list, &result), EAR_SUCCESS);
    assert_non_null(result.values);
    assert_int_equal(result.values[0], 2400000);
    assert_int_equal(result.values[1], 1800000);
    assert_int_equal(context.arch.dispose(NULL), EAR_SUCCESS);
    assert_int_equal(test_context_destroy(&context), 0);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        ISOLATED_TEST(test_intel_independent_results),
        ISOLATED_TEST(test_acpi_independent_results),
        ISOLATED_TEST(test_intel_concurrent_frequency_readers),
        ISOLATED_TEST(test_intel_concurrent_governor_readers),
        ISOLATED_TEST(test_intel_default_snapshots),
        ISOLATED_TEST(test_acpi_default_snapshots),
        ISOLATED_TEST(test_intel_governor_setters_complete),
        ISOLATED_TEST(test_intel_frequency_setters_complete),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
