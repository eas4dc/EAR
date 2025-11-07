/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/types/generic.h>
#include <time.h>

enum domain {
    NODE,
    TAG,
    SUBCLUSTER,
    CLUSTER
};

enum trigger_by {
    HOUR,
    DAY,
    DATE,
    POWER
};

enum action_type {
    SET,
    ENABLE,
    DISABLE
};

union trigger_value {
    time_t timestamp;
    uint64_t long_val;
    double double_val;
};

struct policy_action_data {
    char name[GENERIC_NAME];
    uint64_t def_pstate;
    double settings;
};

union action_data {
    struct policy_action_data policy;
    uint64_t powercap_value;
};

typedef struct rule {
    uint32_t rule_id;
    enum domain domain;
    char domain_id[GENERIC_NAME];
    enum trigger_by trigger_by;
    union trigger_value trigger_value;
    enum action_type action_type;
    union action_data action_data;

} rule_t;
