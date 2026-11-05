/*********************************************************************
 * Copyright (c) 2026 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

/*
 * fake_powermon.c -- see fake_powermon.h
 */
#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include "fake_powermon.h"

uint fake_pc_def     = 1000;
uint fake_pc_max     = 1500;
uint fake_node_power = 0;
fake_ctx_t fake_ctx[FAKE_MAX_CTX];
fake_event_t fake_events[FAKE_MAX_EVENTS];
int fake_num_events = 0;

/* Globals the powercap code expects from eard.c / power_monitor.c */
int ips_storage[1]          = {0x0a000001};
int *ips                    = ips_storage;
int self_id                 = 0;
volatile int init_ips_ready = 1;
int eard_must_exit          = 0;
int max_context_created     = 0;
int num_contexts            = 1;
powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];

void fake_powermon_reset(void)
{
    memset(fake_ctx, 0, sizeof(fake_ctx));
    memset(fake_events, 0, sizeof(fake_events));
    fake_num_events = 0;
    fake_node_power = 0;
    num_contexts    = 1;
}

void fake_ctx_start(int cc, ulong cpu_khz, ulong gpu_mhz, int num_gpus)
{
    fake_ctx_t *c = &fake_ctx[cc];
    memset(c, 0, sizeof(*c));
    c->active           = 1;
    c->app.settings     = &c->settings;
    c->app.resched      = &c->resched;
    c->app.app_info     = &c->app_info;
    c->app.pc_app_info  = &c->pc_app_info;
    c->app.app.is_mpi   = 1;
    c->app.is_job       = 1;
    c->app.app.job.id   = 1000 + cc;
    CPU_ZERO(&c->app_info.node_mask);
    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++) {
        CPU_SET(i, &c->app_info.node_mask);
        c->pc_app_info.req_f[i] = cpu_khz;
    }
#if USE_GPUS
    for (int i = 0; i < num_gpus && i < MAX_GPUS_SUPPORTED; i++)
        c->pc_app_info.req_gpu_f[i] = gpu_mhz;
#endif
    num_contexts = 0;
    for (int i = 0; i < FAKE_MAX_CTX; i++)
        num_contexts += fake_ctx[i].active;
}

void fake_ctx_stop(int cc)
{
    fake_ctx[cc].active = 0;
}

fake_event_t *fake_last_event(uint type)
{
    for (int i = fake_num_events - 1; i >= 0; i--)
        if (fake_events[i].type == type)
            return &fake_events[i];
    return NULL;
}

int fake_count_events(uint type)
{
    int n = 0;
    for (int i = 0; i < fake_num_events; i++)
        n += (fake_events[i].type == type);
    return n;
}

/* ------------------------------------------------ power_monitor.h API */

state_t powermon_context_trylock(uint cc, powermon_app_t **pmapp)
{
    if (cc >= FAKE_MAX_CTX || !fake_ctx[cc].active)
        return EAR_ERROR;
    *pmapp = &fake_ctx[cc].app;
    return EAR_SUCCESS;
}

void powermon_context_unlock(uint cc)
{
}

uint32_t powermon_current_power()
{
    return fake_node_power;
}

uint32_t powermon_get_base_powercap()
{
    return fake_pc_def;
}

uint fake_min_working = 0;

uint32_t powermon_get_min_working_power()
{
    return fake_min_working;
}

uint32_t powermon_get_max_powercap_def()
{
    return fake_pc_max;
}

void powermon_report_event(uint event_type, llong value)
{
    if (fake_num_events < FAKE_MAX_EVENTS) {
        fake_events[fake_num_events].type  = event_type;
        fake_events[fake_num_events].value = value;
        fake_num_events++;
    }
}

uint is_job_in_node(job_id id, job_context_t **jc)
{
    static job_context_t j = {.num_ctx = 1};
    *jc                    = &j;
    return 1;
}
