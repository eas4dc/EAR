#define _GNU_SOURCE
#define SHOW_DEBUGS 1
#define SYN_TEST    1

#include <assert.h>
#include <common/config.h>
#include <common/system/monitor.h>
#include <common/types/powercap.h>
#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Mocking globals from powercap_mgt.c
cluster_conf_t      my_cluster_conf;
node_powercap_opt_t my_pc_opt;
my_node_conf_t     *my_node_conf;
uint32_t            current_mode;
powermon_app_t     *current_ear_app[MAX_NESTED_LEVELS];
int                 max_context_created;
int                 num_contexts;

// Mocking functions
state_t suscribe_f_dummy(void *p)
{
    return EAR_SUCCESS;
}

state_t suscall_f_dummy(void *p)
{
    return EAR_SUCCESS;
}

// Mocking low-level powercap management functions
state_t mock_enable(suscription_t *sus)
{
    sus->suscribe  = suscribe_f_dummy;
    sus->call_init = suscall_f_dummy;
    sus->call_main = suscall_f_dummy;
    return EAR_SUCCESS;
}

state_t mock_disable()
{
    return EAR_SUCCESS;
}

state_t mock_set_powercap_value(uint pid, uint domain, uint *limits, ulong *util)
{
    return EAR_SUCCESS;
}

state_t mock_get_powercap_value(uint pid, uint *powercap)
{
    return EAR_SUCCESS;
}

state_t mock_get_powercap_value_per_device(ulong *powercaps)
{
    return EAR_SUCCESS;
}

uint mock_is_powercap_policy_enabled(uint pid)
{
    return 1;
}

void mock_set_status(uint status)
{
}

void mock_set_pc_mode(uint mode)
{
}

uint mock_get_powercap_strategy()
{
    return 0;
}

void mock_powercap_to_str(char *b)
{
    strcpy(b, "mocked powercap");
}

void mock_print_powercap_value(int fd)
{
}

void mock_set_app_req_freq(ulong *f)
{
}

void mock_set_verb_channel(int fd)
{
}

void mock_set_new_utilization(ulong *util)
{
}

uint mock_get_powercap_status(domain_status_t *status)
{
    return EAR_SUCCESS;
}

state_t mock_reset_powercap_value()
{
    return EAR_SUCCESS;
}

state_t mock_release_powercap_allocation(uint decrease)
{
    return EAR_SUCCESS;
}

state_t mock_increase_powercap_allocation(uint increase)
{
    return EAR_SUCCESS;
}

state_t mock_plugin_set_burst()
{
    return EAR_SUCCESS;
}

state_t mock_plugin_set_relax()
{
    return EAR_SUCCESS;
}

void mock_plugin_get_settings(domain_settings_t *settings)
{
}

void setup_mock_functions()
{
    // This function will be used to replace the function pointers in pcsyms_fun
    // with our mock functions. This is a bit tricky since pcsyms_fun is static.
    // We will use a trick to get access to it.
}

void test_powercap_c_functions()
{
    printf("Testing powercap.c functions...\n");
    assert(get_device_enum("DRAM_0") == DRAM0);
    assert(get_device_enum("CPU_0") == CPU0);
    assert(get_device_enum("GPU_0") == GPUSTART);
    assert(get_device_enum("GPU_1") == GPUSTART + 1);
    assert(get_device_enum("INVALID") == NO_TYPE);
    printf("powercap.c functions tested successfully.\n");
}

void test_powercap_mgt_c_functions()
{
    printf("Testing powercap_mgt.c functions...\n");

    // We need to initialize the power management
    assert(pmgt_init() == EAR_SUCCESS);

    pwr_mgt_t *p_mgt;
    assert(pmgt_handler_alloc(&p_mgt) == EAR_SUCCESS);
    assert(pmgt_enable(p_mgt) == EAR_SUCCESS);

    // Test set/get powercap value
    assert(pmgt_set_powercap_value(p_mgt, 0, 0, 100) == EAR_SUCCESS);
    ulong powercaps[NUM_DOMAINS];
    assert(pmgt_get_powercap_value(p_mgt, 0, powercaps) == EAR_SUCCESS);

    // Test other functions
    assert(pmgt_is_powercap_enabled(p_mgt, 0) > 0);

    char buffer[256];
    pmgt_powercap_to_str(p_mgt, buffer);
    assert(strlen(buffer) > 0);

    pmgt_set_status(p_mgt, 0);
    pmgt_set_pc_mode(p_mgt, 0);

    assert(pmgt_disable(p_mgt) == EAR_SUCCESS);

    printf("powercap_mgt.c functions tested successfully.\n");
}

int main()
{
    test_powercap_c_functions();
    test_powercap_mgt_c_functions(); // This will be enabled later

    return 0;
}