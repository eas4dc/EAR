/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/utils/args.h>
#include <common/utils/string.h>
#include <stdio.h>
#include <string.h>

char *args_get(int argc, char *argv[], const char *arg_in, char *buffer)
{
    int is_required = 0;
    int next_value  = 0;
    int curr_value  = 0;
    int has_equal   = 0;
    int is_short    = 0;
    int is_long     = 0;

    char arg[128];
    char aux[128];
    char *p;
    int i;

    if (strlen(arg_in) == 0) {
        return NULL;
    }
    // Saving the argument in the stack.
    strcpy(aux, arg_in);
    // If value is mandatory
    if (aux[strlen(aux) - 1] == ':') {
        aux[strlen(aux) - 1] = '\0';
        is_required          = 1;
    }
    // Is long or short?
    is_short = (strlen(aux) == 1);
    is_long  = (strlen(aux) >= 2);
    //
    if (is_short)
        xsnprintf(arg, sizeof(arg), "-%s", aux);
    if (is_long)
        xsnprintf(arg, sizeof(arg), "--%s", aux);
    //
    if (buffer != NULL) {
        buffer[0] = '\0';
    }
    //
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], arg, strlen(arg)) != 0) {
            continue;
        }
        // Is equal, check if has value
        curr_value = (strlen(argv[i]) > strlen(arg));
        // Check if next is value
        next_value = (i < (argc - 1)) && ((strncmp(argv[i + 1], "--", 2) != 0) && (strncmp(argv[i + 1], "-", 1) != 0));
        // Check if has equal symbol (=)
        has_equal = (strchr(argv[i], '=') != NULL);
        // Is type -fValue
        if (is_short && curr_value && !has_equal) {
            p = &argv[i][strlen(arg)];
            if (buffer) {
                strcpy(buffer, p);
            }
            return p;
        }
        // Is type -f Value
        if (is_short && next_value) {
            if (buffer) {
                strcpy(buffer, argv[i + 1]);
            }
            return argv[i + 1];
        }
        // Is type --flag=Value
        if (is_long && curr_value && has_equal) {
            p = &argv[i][strlen(arg) + 1];
            if (buffer) {
                strcpy(buffer, p);
            }
            return p;
        }
        // Is type --flag Value
        if (is_long && next_value) {
            if (buffer) {
                strcpy(buffer, argv[i + 1]);
            }
            return argv[i + 1];
        }
        if ((is_long || is_short) && !is_required) {
            // True if value is not mandatory and both flags are completely equal
            if (strcmp(argv[i], arg) == 0) {
                return argv[i];
            }
        }
    }
    return NULL;
}
