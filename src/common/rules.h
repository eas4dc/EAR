/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>

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
    time_t   timestamp;
    uint64_t long_val;
    double   double_val;
};

struct policy_action_data {
    char     name[GENERIC_NAME];
    uint64_t def_pstate;
    double   settings;
};

union action_data {
    struct policy_action_data policy;
    uint64_t powercap_value;
};


typedef struct rule {
    uint32_t            rule_id;
    enum domain         domain;
    char                domain_id[GENERIC_NAME];
    enum trigger_by     trigger_by;
    union trigger_value trigger_value;
    enum action_type    action_type;
    union action_data   action_data;

} rule_t;

