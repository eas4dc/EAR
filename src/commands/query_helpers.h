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
#include <unistd.h>

typedef struct query_addons {
    int limit;
    int job_id;
    int step_id;
    int start_time;
    int end_time;
    char e_tag[64];
    char app_id[64];
    char job_ids[64];
    char step_ids[64];
} query_adds_t;

#define add_string_filter(query, addition, value)           _add_string_filter(query, addition, value, 1);

#define add_string_filter_no_quotes(query, addition, value) _add_string_filter(query, addition, value, 0);

void _add_string_filter(char *query, char *addition, char *value, bool quotes);

void add_int_filter(char *query, char *addition, int value);
void add_int_comp_filter(char *query, char *addition, int value, char greater_than);
void add_int_list_filter(char *query, char *addition, char *value);

// reset the filter for a new query
void reset_query_filters();
