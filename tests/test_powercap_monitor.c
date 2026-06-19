#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Enable powercap monitor for testing
#define POWERCAP_MON 1

// Include headers for types
#include "common/config.h"
#include "common/states.h"
#include "common/system/monitor.h"
#include "common/types.h"
#include "daemon/power_monitor.h"
#include "daemon/powercap/powercap_mgt.h"
#include "daemon/powercap/powercap_status.h"
#include "daemon/shared_configuration.h"

// Mock verbose macros to silence output
#undef verbose
#undef debug
#undef error
#define verbose(...)
#define debug(...)
#define error(...)

// Mock globals
int *ips                    = NULL;
int ips_storage[]           = {0};
int self_id                 = 0;
volatile int init_ips_ready = 1;
int eard_must_exit          = 0;
int max_context_created     = 0;
int num_contexts            = 0;
powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];

// Mock monitor functions
suscription_t mock_sus;
int suscribe_called           = 0;
int monitor_register_called   = 0;
int monitor_unregister_called = 0;

state_t dummy_suscribe(void *s)
{
    suscribe_called++;
    return EAR_SUCCESS;
}

suscription_t *suscription()
{
    memset(&mock_sus, 0, sizeof(suscription_t));
    mock_sus.suscribe = dummy_suscribe;
    return &mock_sus;
}

state_t monitor_register(suscription_t *s)
{
    monitor_register_called++;
    return EAR_SUCCESS;
}

state_t monitor_unregister(suscription_t *s)
{
    monitor_unregister_called++;
    return EAR_SUCCESS;
}

int monitor_is_initialized()
{
    return 1;
}

// Mock pmgt functions
uint pmgt_get_cpu_devices()
{
    return 1;
}

uint pmgt_get_dram_devices()
{
    return 1;
}

uint pmgt_get_gpu_devices()
{
    return 0;
}

state_t pmgt_init()
{
    return EAR_SUCCESS;
}

state_t pmgt_handler_alloc(pwr_mgt_t **p)
{
    return EAR_SUCCESS;
}

state_t pmgt_enable(pwr_mgt_t *p)
{
    return EAR_SUCCESS;
}

state_t pmgt_disable(pwr_mgt_t *p)
{
    return EAR_SUCCESS;
}

void pmgt_process_message(char *level, int32_t num_values, int32_t values[num_values])
{
}

state_t pmgt_set_powercap_value(pwr_mgt_t *p, uint pid, uint domain, uint32_t limit)
{
    return EAR_SUCCESS;
}

uint pmgt_get_powercap_cpu_strategy(pwr_mgt_t *p)
{
    return 0;
}

uint pmgt_get_powercap_gpu_strategy(pwr_mgt_t *p)
{
    return 0;
}

void pmgt_set_pc_mode(pwr_mgt_t *p, uint mode)
{
}

void pmgt_set_status(pwr_mgt_t *p, uint status)
{
}

void pmgt_idle_to_run(pwr_mgt_t *p)
{
}

void pmgt_run_to_idle(pwr_mgt_t *p)
{
}

state_t pmgt_get_powercap_value_per_device(uint domain, uint32_t *powercaps)
{
    return EAR_SUCCESS;
}

void pmgt_set_app_req_freq(pwr_mgt_t *p)
{
}

void pmgt_set_power_per_domain(pwr_mgt_t *p, dom_power_t *cp, uint st)
{
}

void pmgt_get_status(pmgt_status_t *status)
{
}

// Mock powermon functions
uint powermon_get_max_powercap_def()
{
    return 1000;
}

uint powermon_get_powercap_def()
{
    return 500;
}

uint powermon_current_power()
{
    return 100;
}

void powermon_report_event(uint event, llong value)
{
}

// Mock timestamp functions
ullong timestamp_diffnow(timestamp *start, ullong units)
{
    return 0;
}

void timestamp_get(timestamp *t)
{
}

// Additional pmgt mocks
uint pmgt_is_powercap_enabled(pwr_mgt_t *p, uint pid)
{
    return 1;
}

void pmgt_new_job(pwr_mgt_t *p)
{
}

void pmgt_end_job(pwr_mgt_t *p)
{
}

// Mock powercap status functions
int is_powercap_on(node_powercap_opt_t *opt)
{
    return 1;
}

uint get_powercap_allocated(node_powercap_opt_t *opt)
{
    return 0;
}

// Include the source file under test
#include "../src/daemon/powercap/powercap.c"

// Test functions
void test_monitor_enable()
{
    int initial_calls = suscribe_called;
    powercap_process_message("enable", "monitor", NULL, 0, NULL);
    assert(suscribe_called > initial_calls);
    printf("✓ Monitor enable test passed\n");
}

void test_monitor_disable()
{
    int initial_calls = monitor_unregister_called;
    powercap_process_message("disable", "monitor", NULL, 0, NULL);
    assert(monitor_unregister_called > initial_calls);
    printf("✓ Monitor disable test passed\n");
}

void test_monitor_set_time()
{
    int times[] = {8000, 9000};
    powercap_process_message("set", "monitor", NULL, 2, times);
    assert(sus_powercap_monitor->time_relax == 8000);
    assert(sus_powercap_monitor->time_burst == 9000);
    printf("✓ Monitor set time test passed\n");
}

int main()
{
    printf("Starting test_powercap_monitor...\n");

    // Initialize global state
    ips = ips_storage;

    // Initialize powercap
    assert(powercap_init() == EAR_SUCCESS);

    // Run tests
    test_monitor_enable();
    test_monitor_disable();
    test_monitor_set_time();

    printf("All tests passed!\n");
    return 0;
}
