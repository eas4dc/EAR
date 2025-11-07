/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <common/config.h>
#include <common/messaging/msg_internals.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/utils/sched_support.h>

#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_rapi_internals.h>

#define NUM_PARSE_ARGS 5

typedef struct msg_args_s {
    char *type;
    char *action;
    char *level;
    char *values;
    char *mode;
} msg_args_s;

bool expect_arg(char *args, size_t n_expect, const char *expect[static n_expect])
{
    for (size_t i = 0; i < n_expect; i++) {
        if (!strcasecmp(args, expect[i]))
            return true;
    }

    return false;
}

#define MAX_POSSIBILITIES 25
static const char *actions[2]  = {"set", "reset"};
static const char *type[3]     = {"pc", "powercap", "verbose"};
static const char *level_pc[5] = {"node", "cpu", "gpu", "domain", "device"};
static const char *modes[2]    = {"manual", "auto"};

void print_expected(size_t n_expect, const char *expect[static n_expect])
{
    printf("\t\tExpected one of {");
    for (size_t i = 0; i < n_expect - 1; i++) {
        printf("%s, ", expect[i]);
    }
    printf("%s}\n", expect[n_expect - 1]);
}

void check_arg(int32_t argc, char *argv[], int32_t id, size_t n_expect, const char *expect[static n_expect],
               char **to_fill)
{
    if (argc < id + 1 || !expect_arg(argv[id], n_expect, expect)) {
        printf("Usage is %s [node] [action] [type] [level] [values]\n", argv[0]);
        print_expected(n_expect, expect);
        exit(1);
    } else {
        *to_fill = argv[id];
    }
}

void parse_args(int32_t argc, char *argv[], msg_args_s *to_fill, char ***nodelist, int32_t *num_nodes)
{
    if (argc < 2) {
        printf("Usage is %s [node] [action] [type] [level] [values]\n", argv[0]);
        exit(1);
    }

    char *expanded_list = NULL;

    /* If not all nodes, expand the list and parse it */
    if (strcasecmp(argv[1], "all")) {
        expand_list_alloc(argv[1], &expanded_list);
        str_cut_list(expanded_list, nodelist, num_nodes, ",");
    }

    // check for actions
    check_arg(argc, argv, 2, 2, actions, &to_fill->action);
    // check for type
    check_arg(argc, argv, 3, 3, type, &to_fill->type);
    // if type is powercap, check for level and values. Check if mode is present
    if (!strcasecmp(to_fill->type, "pc") || !strcasecmp(to_fill->type, "powercap")) {
        check_arg(argc, argv, 4, 5, level_pc, &to_fill->level);
        if (argc < 6) {
            printf("Usage is %s [node] [action] [type] [level] [values]\n", argv[0]);
            printf("\t\tExpected a value or list of values\n");
            exit(0);
        } else {
            to_fill->values = argv[5];
            if (argc >= 7) {
                check_arg(argc, argv, 6, 2, modes, &to_fill->mode);
                if (argc >= 8) {
                    printf("Ignoring arguments after %s", argv[7]);
                }
            }
        }
    } else if (!strcasecmp(to_fill->type, "verbose")) {
        check_arg(argc, argv, 4, 5, level_pc, &to_fill->level);
    }

    printf("Got all the args with %s %s %s %s %s\n", to_fill->action, to_fill->type, to_fill->level, to_fill->values,
           to_fill->mode);
}

#define check_if_null_and_append(dest, m, field)                                                                       \
    {                                                                                                                  \
        if (m.field != NULL) {                                                                                         \
            char aux[512] = {0};                                                                                       \
            sprintf(aux, " " #field "=%s", m.field);                                                                   \
            strcat(dest, aux);                                                                                         \
        }                                                                                                              \
    }

int main(int32_t argc, char *argv[])
{
    cluster_conf_t cluster_conf = {0};
    char ear_path[256]          = {0};
    if (get_ear_conf_path(ear_path) == EAR_ERROR) {
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path, &cluster_conf);

    msg_args_s msg_args = {0};
    char **nodes        = {0};
    int32_t num_nodes   = 0;
    parse_args(argc, argv, &msg_args, &nodes, &num_nodes);

    char msg[1024] = {0};

    check_if_null_and_append(msg, msg_args, action);
    check_if_null_and_append(msg, msg_args, type);
    check_if_null_and_append(msg, msg_args, level);
    check_if_null_and_append(msg, msg_args, values);
    check_if_null_and_append(msg, msg_args, mode);

    printf("Sending the following message to EARDs: %s\n", msg);

    ear_send_message(&cluster_conf, msg, nodes, num_nodes);
}
