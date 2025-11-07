/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
// #define SHOW_DEBUGS 1
#include <common/colors.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/execute.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <management/cpufreq/frequency.h>

/* From Grace Hopper Performance Tuning guide, section 4.4 (GPU and Module Power Management) */
#define GH_SET_POWERCAP     "nvidia-smi -pl %u -sc 1"
#define GH_DISABLE_POWERCAP "nvidia-smi -pl 0 -sc 1"

/* Additional commands */
#define GH_ENABLE_PERSISTENT_MODE "nvidia-smi -pm 1"

static uint32_t c_limit;
static uint32_t policy_enabled = 0;
static uint32_t pc_on          = 0;
static ulong c_req_f;
static suscription_t *sus_inm;

static domain_settings_t settings = {.node_ratio = 0.0, .security_range = 0.01}; //

int do_cmd(char *cmd)
{
    if (strcmp(cmd, "NO_CMD") == 0)
        return 0;
    return 1;
}

state_t disable()
{
    return EAR_SUCCESS;
}

state_t enable(suscription_t *sus)
{
    sus_inm = sus;
    return EAR_SUCCESS;
}

state_t plugin_set_burst()
{
    return monitor_burst(sus_inm, 0);
}

state_t plugin_set_relax()
{
    return monitor_relax(sus_inm);
}

void plugin_get_settings(domain_settings_t *s)
{
    memcpy(s, &settings, sizeof(domain_settings_t));
}

state_t disable_powercap()
{
    char cmd[1024] = GH_DISABLE_POWERCAP;
    return execute(cmd);
}

state_t set_powercap_value(uint32_t pid, uint32_t domain, uint32_t *limit, uint32_t *cpu_util)
{
    char cmd[1024];
    if (!policy_enabled)
        return EAR_SUCCESS;
    if (domain == LEVEL_DEVICE) {
        warning("Per device powercap not implemented for Grace Hopper, returning before applying it.");
        return EAR_ERROR;
    }
    debug("GH:inm_set_powercap_value policy %u limit %u", pid, *limit);
    c_limit = *limit;
    pc_on   = 1;
    if (c_limit == POWER_CAP_UNLIMITED) {
        return disable_powercap();
    }
    sprintf(cmd, GH_SET_POWERCAP, *limit);
    debug(cmd);
    return execute(cmd);
}

state_t get_powercap_value(uint32_t pid, uint32_t *powercap)
{
    debug("GH. get_powercap_value");
    *powercap = c_limit;
    return EAR_SUCCESS;
}

uint32_t is_powercap_policy_enabled(uint32_t pid)
{
    return policy_enabled;
}

void set_status(uint32_t status)
{
    debug("GH. pending");
}

uint32_t get_powercap_stragetgy()
{
    return PC_POWER;
}

void set_app_req_freq(ulong *f)
{
    debug("GH:Requested application freq set to %lu", *f);
    c_req_f = *f;
}

uint32_t get_powercap_status(uint32_t *in_target, uint32_t *tbr)
{
    return 0;
#warning Grace Hopper powercap status not implemented
}
