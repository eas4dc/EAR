/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#pragma once

#include <common/config.h>
#include <common/states.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/generic.h>

typedef struct tags {
    char id[GENERIC_NAME];
    char type;
    char is_default;
    char powercap_type;
    ulong max_avx512_freq;
    ulong max_avx2_freq;
    ulong max_power;
    ulong min_power;
    ulong max_temp;
    ulong cpu_temp_cap;
    ulong gpu_temp_cap;
    ulong error_power;
    ulong gpu_def_freq;
    int cpu_max_pstate; /* Used as lower limit for policies */
    int imc_max_pstate; /* Used as lower limit for policies */
    int idle_pstate;
    ulong imc_max_freq; /* Used to create the imcf list */
    ulong imc_min_freq; /* Used to create the imcf list */
    long powercap;
    long max_powercap;
    char energy_model[GENERIC_NAME];
    char energy_plugin[GENERIC_NAME];
    char powercap_plugin[GENERIC_NAME];
    char powercap_node_plugin[GENERIC_NAME];
    char powercap_gpu_plugin[GENERIC_NAME];
    char coeffs[GENERIC_NAME];
    char idle_governor[GENERIC_NAME];
    char default_policy[POLICY_NAME_SIZE];
} tag_t;

state_t TAG_token(char *token);
state_t TAG_parse_token(tag_t **tags_i, unsigned int *num_tags_i, char *line);
void print_tags_conf(tag_t *tag, int i);
void power2str(ulong power, char *str);
