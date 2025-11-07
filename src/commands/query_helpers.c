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

#define _XOPEN_SOURCE 700 // to get rid of the warning

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <commands/query_helpers.h>

static int query_filters = 0;

void reset_query_filters()
{
    query_filters = 0;
}

void _add_string_filter(char *query, char *addition, char *value, bool quotes)
{

    if (strchr(value, ' ') || strchr(value, '\'')) {
        printf("\"%s\" argument (\"%s\") is invalid.\n", addition, value);
        exit(1);
    }
    if (query_filters < 1) {
        strcat(query, " WHERE ");
    } else {
        strcat(query, " AND ");
    }
    strcat(query, addition);
    strcat(query, "=");
    if (quotes)
        strcat(query, "'");
    strcat(query, value);
    if (quotes)
        strcat(query, "'");
    //	sprintf(query, query, value);
    query_filters++;
}

#define add_string_filter(query, addition, value)           _add_string_filter(query, addition, value, 1);

#define add_string_filter_no_quotes(query, addition, value) _add_string_filter(query, addition, value, 0);

void add_int_filter(char *query, char *addition, int value)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");

    strcat(query_tmp, addition);
    strcat(query_tmp, "=");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters++;
}

void add_int_comp_filter(char *query, char *addition, int value, char greater_than)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");

    strcat(query_tmp, addition);
    if (greater_than)
        strcat(query_tmp, ">");
    else
        strcat(query_tmp, "<");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters++;
}

void add_int_list_filter(char *query, char *addition, char *value)
{
    if (query_filters < 1)
        strcat(query, " WHERE ");
    else
        strcat(query, " AND ");

    strcat(query, addition);
    strcat(query, " IN ");
    strcat(query, "(");
    strcat(query, value);
    strcat(query, ")");
    //	sprintf(query, query, value);
    query_filters++;
}
