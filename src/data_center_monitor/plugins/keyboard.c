/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <ctype.h>
#include <data_center_monitor/plugins/keyboard.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct keyboard_s {
    cchar *command;
    cchar *format;
    uint params_count;
    callback_t callback;
} keyboard_t;

static pthread_t thread;
static keyboard_t keys[1024];
static uint keys_count;
static int is_closing;

declr_up_get_tag()
{
    *tag       = "keyboard";
    *tags_deps = NULL;
}

static uint format_count(cchar *format)
{
    const char *p = format;
    uint inword   = 0;
    uint count    = 0;

    do
        switch (*p) {
            case '\0':
            case ' ':
                if (inword) {
                    inword = 0;
                    count++;
                }
                break;
            default:
                inword = 1;
        }
    while (*p++);

    return count;
}

void command_register(cchar *command, cchar *format, callback_t callback)
{
    keys[keys_count].command      = command;
    keys[keys_count].format       = format;
    keys[keys_count].callback     = callback;
    keys[keys_count].params_count = format_count(format);
    debug("Registered command %s with %u parameters", keys[keys_count].command, keys[keys_count].params_count);
    ++keys_count;
}

static void command_exit(char **params)
{
    is_closing = 1;
    plugin_manager_close();
}

static int is(char *s1, const char *s2)
{
    return strcasecmp(s1, s2) == 0;
}

static void *command_read(void *x)
{
    static char command[128];
    static char *params[4];
    int k, p;

    params[0] = calloc(128, sizeof(char));
    params[1] = calloc(128, sizeof(char));
    params[2] = calloc(128, sizeof(char));
    params[3] = calloc(128, sizeof(char));
    command_register("exit", "", command_exit);
    sleep(1);

    while (!is_closing) {
        printf("Enter command: ");
        fflush(stdout);
        scanf("%s", command);
        for (k = 0; k < keys_count; ++k) {
            if (is(command, keys[k].command)) {
                for (p = 0; p < keys[k].params_count; ++p) {
                    scanf("%s", params[p]);
                }
                keys[k].callback(params);
            }
        }
    }
    // Waiting here until the parent dies
    while (is_closing)
        ;
    return NULL;
}

declr_up_action_init(_keyboard)
{
    pthread_create(&thread, NULL, command_read, NULL);
    return NULL;
}
