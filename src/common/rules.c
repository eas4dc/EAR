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

#include <common/config.h>
#include <common/rules.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>


static int     num_rules = 0;
static rule_t  *ruleset = NULL;


state_t rule_add(rule_t *rule)
{
    ruleset = realloc(ruleset, sizeof(rule_t)*num_rules + 1);
    
    memcpy(&ruleset[num_rules], rule, sizeof(rule_t));

    num_rules++;

    return EAR_SUCCESS;
}

state_t rule_add_list(rule_t *rules, int count)
{
    if (rules == NULL || num_rules < 1) return EAR_ERROR;

    ruleset = realloc(ruleset, sizeof(rule_t)*num_rules + count);

    memcpy(&ruleset[num_rules], rules, sizeof(rule_t)*count);

    num_rules += count;

    return EAR_SUCCESS;
}

state_t rule_filter(char *filter, enum domain domain)
{
    int i;
    for (i = 0; i < num_rules; i++) {
        //if the rule is of the same type and name, keep it
        if (domain == ruleset[i].domain) {
            if (!strcmp(filter, ruleset[i].domain_id)) {
                continue;
            }
        }
        num_rules --;
        ruleset[i] = ruleset[num_rules]; //move the last one in this position 
        ruleset = realloc(ruleset, sizeof(rule_t)*num_rules); //remove the extra element

        i--; //re-do this position (which is now the last one)
    }

    if (num_rules == 0) ruleset = NULL; //to prevent issues with further reallocs down the line

    return EAR_SUCCESS;
}

state_t rule_check(time_t timestamp, uint64_t power)
{
    int i;
    for (i = 0; i < num_rules; i++) {
    //    if 
    }

    return EAR_SUCCESS;
}
