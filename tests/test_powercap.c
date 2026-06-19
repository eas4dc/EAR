#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Enable powercap monitor
#define POWERCAP_MON 1

// Include headers for types
#include "common/config.h"
#include "common/states.h"
#include "common/system/monitor.h"
#include "common/types.h"
#include "daemon/power_monitor.h"
#include "daemon/powercap/powercap.h"
#include "daemon/powercap/powercap_mgt.h"

// Mock verbose macros
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

// Mock constants
uint32_t mock_power_unlimited = POWER_CAP_UNLIMITED;
uint mock_cpu_devices         = 2;
uint mock_dram_devices        = 1;
uint mock_gpu_devices         = 0;

// Mock monitor functions
suscription_t mock_sus;

suscription_t *suscription()
{
    memset(&mock_sus, 0, sizeof(suscription_t));
    return &mock_sus;
}

state_t monitor_register(suscription_t *s)
{
    return EAR_SUCCESS;
}

state_t monitor_unregister(suscription_t *s)
{
    return EAR_SUCCESS;
}

int monitor_is_initialized()
{
    return 1;
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

// Mock pmgt functions
uint pmgt_get_cpu_devices()
{
    return mock_cpu_devices;
}

uint pmgt_get_dram_devices()
{
    return mock_dram_devices;
}

uint pmgt_get_gpu_devices()
{
    return mock_gpu_devices;
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

void pmgt_set_app_req_freq(pwr_mgt_t *p)
{
}

void pmgt_set_power_per_domain(pwr_mgt_t *p, dom_power_t *cp, uint st)
{
}

void pmgt_get_status(pmgt_status_t *status)
{
}

void pmgt_new_job(pwr_mgt_t *p)
{
}

void pmgt_end_job(pwr_mgt_t *p)
{
}

void pmgt_process_message(char *level, int32_t num_values, int32_t values[num_values])
{
    printf("pmgt_process_message: level=%s, num_values=%d\n", level, num_values);
}

state_t pmgt_get_powercap_value_per_device(uint domain, uint32_t *powercaps)
{
    uint count = (domain == DOMAIN_CPU)    ? mock_cpu_devices
                 : (domain == DOMAIN_DRAM) ? mock_dram_devices
                                           : mock_gpu_devices;
    for (uint i = 0; i < count; i++) {
        powercaps[i] = mock_power_unlimited;
    }
    return EAR_SUCCESS;
}

// Include source file under test
#include "daemon/powercap/powercap.c"

// Test functions
void test_powercap_storage()
{
    printf("Testing powercap storage...\n");

    // Initialize storage
    powercap_init_device_storage();

    // Check initial values
    assert(powercap_get_stored_device_value(DOMAIN_CPU, 0) == mock_power_unlimited);
    assert(powercap_get_stored_device_value(DOMAIN_CPU, 1) == mock_power_unlimited);
    assert(powercap_get_stored_device_value(DOMAIN_DRAM, 0) == mock_power_unlimited);

    // Test setting values
    assert(powercap_set_stored_device_value(DOMAIN_CPU, 0, 100) == EAR_SUCCESS);
    assert(powercap_get_stored_device_value(DOMAIN_CPU, 0) == 100);

    assert(powercap_set_stored_device_value(DOMAIN_DRAM, 0, 50) == EAR_SUCCESS);
    assert(powercap_get_stored_device_value(DOMAIN_DRAM, 0) == 50);

    // Test updating all devices in domain
    powercap_update_all_device_storage(DOMAIN_CPU, 120);
    assert(powercap_get_stored_device_value(DOMAIN_CPU, 0) == 120);
    assert(powercap_get_stored_device_value(DOMAIN_CPU, 1) == 120);

    // Cleanup
    powercap_cleanup_device_storage();
    assert(powercap_get_stored_device_value(DOMAIN_CPU, 0) == 0);
    assert(powercap_get_stored_device_value(DOMAIN_DRAM, 0) == 0);

    printf("✓ Storage test passed\n");
}

void test_powercap_verification()
{
    printf("Testing powercap verification...\n");

    powercap_init_device_storage();
    my_pc_opt.current_pc = 100; // Enable powercap

    // Set stored value
    powercap_set_stored_device_value(DOMAIN_CPU, 0, mock_power_unlimited);

    // Mock hardware matches stored
    assert(powercap_verify_device_values(DOMAIN_CPU) == EAR_SUCCESS);

    // Create mismatch
    powercap_set_stored_device_value(DOMAIN_CPU, 0, 100);
    assert(powercap_verify_device_values(DOMAIN_CPU) == EAR_WARNING);

    powercap_cleanup_device_storage();

    printf("✓ Verification test passed\n");
}

void test_powercap_process_message()
{
    printf("Testing powercap process message...\n");

    int32_t values[] = {200};

    // Test manual mode
    powercap_process_message(NULL, "manual", NULL, 0, NULL);
    assert(current_mode == PC_MODE_MANUAL);

    // Test deactivate manual
    powercap_process_message("deactivate", "manual", NULL, 0, NULL);
    assert(current_mode == PC_MODE_AUTO);

    // Test node level
    powercap_process_message(NULL, NULL, "node", 0, NULL);
    assert(current_mode == PC_MODE_AUTO);

    // Test cpu level
    powercap_process_message(NULL, NULL, "cpu", 0, NULL);
    assert(current_mode == PC_MODE_MANUAL);

    // Test set action
    powercap_process_message("set", NULL, "cpu", 1, values);

    printf("✓ Process message test passed\n");
}

int main()
{
    printf("Running powercap tests...\n");

    test_powercap_storage();
    test_powercap_verification();
    test_powercap_process_message();

    printf("All tests passed!\n");
    return 0;
}
