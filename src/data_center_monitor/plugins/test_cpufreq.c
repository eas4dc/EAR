/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <string.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <data_center_monitor/plugins/keyboard.h>
#include <data_center_monitor/plugins/management.h>

static manages_info_t *m;
static char buffer[4096];

declr_up_get_tag()
{
    *tag = "test_cpufreq";
    *tags_deps = "<!management,!keyboard";
}

declr_up_action_init(_management)
{
    m = (manages_info_t *) data;
    debug("Received management data");
    return NULL;
}

static void governors_print(char **params)
{
    static uint govs[1024];
    int g;

    if (state_fail(mgt_cpufreq_governor_get_list(no_ctx, govs))) {
        debug("mgt_cpufreq_governor_get_list: %s", state_msg);
    }
    for (g = 0; g < m->cpu.devs_count; ++g) {
        governor_tostr(govs[g], buffer);
        printf("%12s ", buffer);
        if (g != 0 && ((g+1) % 4) == 0) {
            printf("\n");
        }
    }
    if (((g) % 4) != 0) {
        printf("\n");
    }
}

static void governors_set(char **params)
{
    uint governor;
    governor_toint(params[0], &governor);
    debug("changing to governor: %u", governor);
    if (state_fail(mgt_cpufreq_governor_set(no_ctx, governor))) {
        debug("mgt_cpufreq_governor_set: %s", state_msg);
    }
    governors_print(NULL);
}

static void freqs_print(char **params)
{
    pstate_t freqs[1024];
    if (state_fail(mgt_cpufreq_get_current_list(no_ctx, freqs))) {
        debug("mgt_cpufreq_get_current_list: %s", state_msg);
    }
    mgt_cpufreq_data_print(freqs, m->cpu.devs_count, verb_channel);
}

static void freqs_set(char **params)
{
    if (state_fail(mgt_cpufreq_set_current(no_ctx, atoi(params[0]), atoi(params[1])))) {
        debug("mgt_cpufreq_get_current_list: %s", state_msg);
    }
    freqs_print(NULL);
}

declr_up_action_periodic(_test_cpufreq)
{
    command_register("governors_print", "", governors_print);
    command_register("governors_set", "%s", governors_set);
    command_register("freqs_print", "", freqs_print);
    command_register("freqs_set", "%s %s", freqs_set);
    return "[=]";
}