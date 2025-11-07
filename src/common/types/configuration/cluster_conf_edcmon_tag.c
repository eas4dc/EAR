/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE

#include <common/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/string_enhanced.h>
#include <common/types/configuration/cluster_conf_edcmon_tag.h>
#include <common/types/generic.h>

static pdu_t from_str_to_pdu(char *desc)
{
    if (strcmp(desc, "storage") == 0)
        return storage;
    if (strcmp(desc, "network") == 0)
        return network;
    if (strcmp(desc, "management") == 0)
        return management;
    else
        return others;
}

static char *from_pdu_to_str(pdu_t t)
{
    switch (t) {
        case storage:
            return "storage";
        case network:
            return "network";
        case management:
            return "management";
        case others:
            return "others";
    }
    return "others";
}

static void set_default_edcmon_tag_values(edcmon_t *c_tag)
{
    c_tag->type            = others;
    c_tag->sensors_list[0] = '\0';
    c_tag->pdu_list[0]     = '\0';
    snprintf(c_tag->host, sizeof(c_tag->host), "any");
}

state_t EDCMON_token(char *token)
{
    if (strcasestr(token, "EDCMONTAG") != NULL) {
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}

state_t EDCMON_parse_token(edcmon_t **edcmon_i, unsigned int *num_edcmon_i, char *line)
{
    char *buffer_ptr, *second_ptr; // auxiliary pointers
    char *token;                   // group token
    char *key, *value;             // main tokens

    state_t found = EAR_ERROR;

    int i;
    int idx = -1;

    // initial value assignement
    edcmon_t *edcmons    = *edcmon_i;
    uint num_edcmon_tags = *num_edcmon_i;

    if (num_edcmon_tags == 0)
        edcmons = NULL;

    token = strtok_r(line, " ", &buffer_ptr);

    while (token != NULL) {
        key   = strtok_r(token, "=", &second_ptr);
        value = strtok_r(NULL, "=", &second_ptr);

        if (key == NULL || value == NULL || !strlen(key) || !strlen(value)) {
            token = strtok_r(NULL, " ", &buffer_ptr);
            continue;
        }

        strclean(value, '\n');
        strtoup(key);

        if (!strcmp(key, "EDCMONTAG")) {
            found = EAR_SUCCESS;
            for (i = 0; i < num_edcmon_tags; i++)
                if (!strcmp(edcmons[i].id, value))
                    idx = i;

            // If needed , we add more space
            if (idx < 0) {
                edcmons = realloc(edcmons, sizeof(edcmon_t) * (num_edcmon_tags + 1));
                idx     = num_edcmon_tags;
                num_edcmon_tags++;
                memset(&edcmons[idx], 0, sizeof(edcmon_t));
                set_default_edcmon_tag_values(&edcmons[idx]);
                strcpy(edcmons[idx].id, value);
            }
        } else if (!strcmp(key, "HOST")) {
            strncpy(edcmons[idx].host, value, sizeof(edcmons[idx].host) - 1);
        } else if (!strcmp(key, "PDU_TYPE")) {
            edcmons[idx].type = from_str_to_pdu(value);
        } else if (!strcmp(key, "PDU_IPS")) {
            strncpy(edcmons[idx].pdu_list, value, sizeof(edcmons[idx].pdu_list) - 1);
        } else if (!strcmp(key, "SENSOR_LIST")) {
            strncpy(edcmons[idx].sensors_list, value, sizeof(edcmons[idx].sensors_list) - 1);
            value = strtok_r(NULL, "\"", &buffer_ptr);
            if (value) {
                strncat(edcmons[idx].sensors_list, " ", sizeof(edcmons[idx].sensors_list) - 1);
                strncat(edcmons[idx].sensors_list, value,
                        sizeof(edcmons[idx].sensors_list) - strlen(edcmons[idx].sensors_list) - 1);
            }
            remove_chars(edcmons[idx].sensors_list, '"');
            remove_chars(edcmons[idx].sensors_list, '\n');
        }

        token = strtok_r(NULL, " ", &buffer_ptr);
    }

    // print_edcmon_tags_conf(&edcmons[idx], idx);

    *edcmon_i     = edcmons;
    *num_edcmon_i = num_edcmon_tags;
    return found;
}

void print_edcmon_tags_conf(edcmon_t *edcmon_tag, uint i)
{

    verbose(VCCONF, "EDCMON[%u]: Host %s ID %s Type %s Sensors='%s' IPs='%s'", i, edcmon_tag->host, edcmon_tag->id,
            from_pdu_to_str(edcmon_tag->type), edcmon_tag->sensors_list, edcmon_tag->pdu_list);
}
