/*
 * fake_pmgt.c -- see fake_pmgt.h
 */
#include <stdlib.h>
#include <string.h>

#include <common/config.h>
#include <daemon/powercap/powercap_status_conf.h>

#include "fake_pmgt.h"

pmgt_status_t fake_pmgt_status;
state_t fake_pmgt_init_ret = EAR_SUCCESS;
uint fake_pmgt_devices[4]  = {0, 2, 2, 4};
uint32_t fake_pmgt_hw_caps[4][16];

fake_pmgt_call_t fake_pmgt_calls[FAKE_PMGT_MAX_CALLS];
int fake_pmgt_num_calls = 0;
uint fake_pmgt_last_status;
int fake_pmgt_idle_to_run, fake_pmgt_run_to_idle, fake_pmgt_new_job, fake_pmgt_end_job;
int fake_pmgt_per_domain_calls;
uint fake_pmgt_per_domain_last_st;
int fake_pmgt_msg_calls;
char fake_pmgt_msg_level[32];
int32_t fake_pmgt_msg_values[16];
int32_t fake_pmgt_msg_num;
int fake_pmgt_disabled;

void fake_pmgt_reset(void)
{
    memset(&fake_pmgt_status, 0, sizeof(fake_pmgt_status));
    fake_pmgt_init_ret  = EAR_SUCCESS;
    fake_pmgt_num_calls = 0;
    fake_pmgt_idle_to_run = fake_pmgt_run_to_idle = fake_pmgt_new_job = fake_pmgt_end_job = 0;
    fake_pmgt_per_domain_calls = 0;
    fake_pmgt_msg_calls        = 0;
    fake_pmgt_disabled         = 0;
    memset(fake_pmgt_hw_caps, 0, sizeof(fake_pmgt_hw_caps));
}

long long fake_pmgt_last_limit(void)
{
    if (fake_pmgt_num_calls == 0)
        return -1;
    return fake_pmgt_calls[fake_pmgt_num_calls - 1].limit;
}

state_t pmgt_init()
{
    return fake_pmgt_init_ret;
}

state_t pmgt_handler_alloc(pwr_mgt_t **phandler)
{
    *phandler = calloc(1, sizeof(pwr_mgt_t));
    return EAR_SUCCESS;
}

state_t pmgt_enable(pwr_mgt_t *phandler)
{
    return EAR_SUCCESS;
}

state_t pmgt_disable(pwr_mgt_t *phandler)
{
    fake_pmgt_disabled++;
    return EAR_SUCCESS;
}

state_t pmgt_set_powercap_value(pwr_mgt_t *phandler, uint pid, uint domain, uint32_t limit)
{
    if (fake_pmgt_num_calls < FAKE_PMGT_MAX_CALLS) {
        fake_pmgt_calls[fake_pmgt_num_calls].domain = domain;
        fake_pmgt_calls[fake_pmgt_num_calls].limit  = limit;
        fake_pmgt_num_calls++;
    }
    return EAR_SUCCESS;
}

uint pmgt_get_powercap_cpu_strategy(pwr_mgt_t *phandler)
{
    return PC_POWER;
}

uint pmgt_get_powercap_gpu_strategy(pwr_mgt_t *phandler)
{
    return PC_POWER;
}

void pmgt_set_pc_mode(pwr_mgt_t *phandler, uint mode)
{
}

void pmgt_set_power_per_domain(pwr_mgt_t *phandler, dom_power_t *pdomain, uint st)
{
    fake_pmgt_per_domain_calls++;
    fake_pmgt_per_domain_last_st = st;
}

void pmgt_set_app_req_freq(pwr_mgt_t *phandler)
{
}

void pmgt_set_status(pwr_mgt_t *phandler, uint status)
{
    fake_pmgt_last_status = status;
}

void pmgt_idle_to_run(pwr_mgt_t *phandler)
{
    fake_pmgt_idle_to_run++;
}

void pmgt_run_to_idle(pwr_mgt_t *phandler)
{
    fake_pmgt_run_to_idle++;
}

void pmgt_new_job(pwr_mgt_t *phandler)
{
    fake_pmgt_new_job++;
}

void pmgt_end_job(pwr_mgt_t *phandler)
{
    fake_pmgt_end_job++;
}

void pmgt_get_status(pmgt_status_t *status)
{
    memcpy(status, &fake_pmgt_status, sizeof(pmgt_status_t));
}

state_t pmgt_process_message(char *level, int32_t num_values, int32_t values[num_values])
{
    fake_pmgt_msg_calls++;
    strncpy(fake_pmgt_msg_level, level ? level : "(null)", sizeof(fake_pmgt_msg_level) - 1);
    fake_pmgt_msg_num = num_values;
    for (int i = 0; i < num_values && i < 16; i++)
        fake_pmgt_msg_values[i] = values[i];
    return EAR_SUCCESS;
}

uint pmgt_get_cpu_devices()
{
    return fake_pmgt_devices[DOMAIN_CPU];
}

uint pmgt_get_dram_devices()
{
    return fake_pmgt_devices[DOMAIN_DRAM];
}

uint pmgt_get_gpu_devices()
{
    return fake_pmgt_devices[DOMAIN_GPU];
}

state_t pmgt_get_powercap_value_per_device(uint domain, uint32_t *powercaps)
{
    if (domain >= 4)
        return EAR_ERROR;
    for (uint i = 0; i < fake_pmgt_devices[domain]; i++)
        powercaps[i] = fake_pmgt_hw_caps[domain][i];
    return EAR_SUCCESS;
}

uint32_t fake_hw_min = 0, fake_hw_max = 0;
void pmgt_get_node_hw_limits(uint32_t *node_min, uint32_t *node_max)
{
    *node_min = fake_hw_min;
    *node_max = fake_hw_max;
}

uint pmgt_domain_readable(uint domain)
{
    return domain < NUM_DOMAINS;
}

uint32_t pmgt_get_device_tdp(uint domain, uint device)
{
    return UINT32_MAX;
}
