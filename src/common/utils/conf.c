/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
//#define SHOW_DEBUGS 0
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <common/utils/conf.h>
#include <common/output/debug.h>
#include <common/utils/string.h>

static ear_field_t null = { .search     = ear_field_search,
        .children   = &null,
        .sibling    = &null,
        .key        = "null",
        .value      = NULL,
        .value_safe = "",
        .is_null    = 1 };

static char* ear_field_trim(char *str)
{
    char *end = NULL;
    while(isspace((unsigned char) *str)) {
        str++;
    }
    if(*str == '\0') {
        return str;
    }
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char) *end)) {
        end--;
    }
    end[1] = '\0';
    return str;
}

static char *strtolow(char *string)
{
    char *p = string;
    while (*string != '\0') {
        *string = tolower((unsigned char) *string);
        string++;
    }
    return p;
}

static char *range_clean(char *string)
{
    int in_range = 0;
    char *p = string;
    while (*string != '\0') {
        if (*string == '[') in_range = 1;
        if (*string == ']') in_range = 0;
        if (in_range && *string == ',') *string = ';';
        string++;
    }
    return p;
}

static ear_field_t *field_create(ear_field_t *parent, char *key, char *value, int deep)
{
    ear_field_t *field    = calloc(1, sizeof(ear_field_t));
    field->search         = ear_field_search;
    field->key            = key ? strtolow(strdup(key)) : NULL;
    field->value          = value ? range_clean(strdup(value)) : NULL;
    field->value_safe     = field->value ? field->value : "";
    field->value_toint    = atoi(value);
    field->children       = &null;
    field->sibling        = &null;
    field->parent         = parent;
    field->deep           = deep;
    field->key_concat     = parent ? calloc(strlen(parent->key_concat)+strlen(key)+2, 1) : strdup(field->key);
    if (parent) {
        sprintf(field->key_concat, "%s.%s", parent->key_concat, field->key);
    }
    return field;
}

static void line_parse(char *line, ear_field_t **link, int deep)
{
    static ear_field_t *last = &null;
    char *equals        = NULL;
    char *token         = NULL;
    char *value         = NULL;
    char *key           = NULL;
    ear_field_t *field  = NULL;
    ear_field_t *parent = NULL;
    int count       = 0;

    if (!line) {
        return;
    }
    // Parsing holes
    token = strtok(line, " \t");
    while (token != NULL) {
        if ((equals = strchr(token, '=')) == NULL) {
            token = strtok(NULL, " \t");
            continue;
        }
        *equals = '\0';
        key = ear_field_trim(token);
        value = ear_field_trim(equals + 1);
        // Creating new field for the new parsed line
        if ((field = field_create(parent, key, value, deep)) != NULL) {
            // Saving the parent for key concatenation and parent link
            parent = (parent == NULL) ? field: parent;
            // Setting the relationship
            *link = field;
            // Setting the array like iterator
            last->next = field;
            // Setting a new relationship for the following fields
            deep   = (count   == 0) ? deep + 1: deep;
            link   = (count++ == 0) ? &field->children: &field->sibling;
            last   = field;
        }
        token = strtok(NULL, " \t");
    }
    return;
}

ear_conf_t* ear_conf_read(char *file_path)
{
    char line[4096], line_copy[4096], *line_trimmed;
    ear_conf_t *conf = NULL;
    FILE *file = NULL;
    ear_field_t **link;

    if (!file_path) {
        return NULL;
    }
    if ((file = fopen(file_path, "r")) == NULL) {
        printf("error 1\n");
        return NULL;
    }
    if ((conf = calloc(1, sizeof(ear_conf_t))) == NULL) {
        fclose(file);
        return NULL;
    }
    conf->fields = NULL;
    conf->file_path = strdup(file_path);
    link = &conf->fields;

    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        // Skip empty lines and comments
        line_trimmed = ear_field_trim(line);
        if (strlen(line_trimmed) == 0 || line_trimmed[0] == '#') {
            continue;
        }
        debug("trimmed: %s", line_trimmed);
        // Parse the line
        strncpy(line_copy, line_trimmed, sizeof(line_copy) - 1);
        line_parse(line_copy, link, 0);
        // Setting a new relationship
        link = &((*link)->sibling);
    }
    fclose(file);
    return conf;
}

static void field_print(ear_field_t *field, int indent)
{
    if (field == NULL || field->is_null) {
        return;
    }
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    printf("%s = %s\n", field->key_concat, field->value);
    field_print(field->children, indent+1);
    field_print(field->sibling, indent);
}

void ear_conf_print(ear_conf_t *fconf)
{
    if (fconf == NULL) {
        return;
    }
    printf("Configuration: %s\n", fconf->file_path ? fconf->file_path : "(null)");
    field_print(fconf->fields, 0);
}

static int range_master(char *prefix, char *suffix, char *range, char *value, int ordinal)
{
    char  *dash          = NULL;
    int    range_start   = 0;
    int    range_end     = 0;
    char   expected[256] = {0};
    int    digits_len    = 0;

    // Dash processing (ie. 10-20)
    dash        = strchr(range, '-')? strchr(range, '-'): range;
    dash[0]     = (dash != range)? '\0': dash[0];
    range_start = atoi(range);
    range_end   = (dash != range)? atoi(dash + 1): range_start;
    digits_len  = strlen(range);
    //
    if (ordinal >= range_start && ordinal <= range_end) {
        // Build expected string and compare
        snprintf(expected, sizeof(expected), "%s%0*d%s",
                 prefix, digits_len, ordinal, suffix);
        debug("expected: %s == %s", expected, value);
        return strcmp(expected, value) == 0;
    }
    return 0;
}

static int pattern_slave(char *pattern, char *value)
{
    size_t value_len         = 0;
    char  *bracket_start     = NULL;
    char  *bracket_end       = NULL;
    char   prefix[128]       = {0};
    size_t prefix_len        = 0;
    char  *suffix            = NULL;
    size_t suffix_len        = 0;
    char   range[256]        = {0};
    size_t range_len         = 0;
    char **list              = NULL; // Range list
    uint   list_count        = 0;
    uint   list_ind          = 0;
    char   ordinal[32]       = {0};
    int    ordinal_int       = 0;
    int    retval            = 0;

    // If exact
    if (strcmp(pattern, value) == 0) return 1;
    // Processing value
    value_len          = strlen(value);
    // Processing range brackets
    if ((bracket_start = strchr(pattern      , '[')) == NULL) return 0;
    if ((bracket_end   = strchr(bracket_start, ']')) == NULL) return 0;
    // Processing prefix
    prefix_len         = bracket_start - pattern;
    strncpy(prefix, pattern, prefix_len);
    // Processing suffix
    suffix             = bracket_end + 1;
    suffix_len         = strlen(suffix);
    // Prefix and suffix comparisons
    if (strncmp(pattern, value, prefix_len) != 0) return 0;
    if (suffix_len > 0 && value_len < (prefix_len + suffix_len)) return 0; // Value length lower
    if (suffix_len > 0 && strcmp(value + value_len - suffix_len, suffix) != 0) return 0;
    // Processing range
    range_len = bracket_end - bracket_start - 1;
    strncpy(range, bracket_start + 1, range_len);
    range[range_len] = '\0';
    // Processing ordinal (ie. 4280 from node4280b)
    strncpy(ordinal, value + prefix_len, value_len - prefix_len - suffix_len);
    ordinal[value_len - prefix_len - suffix_len] = '\0';
    ordinal_int = atoi(ordinal);
    // If range is a comma separated list
    if (strtoa(range, ';', &list, &list_count) == NULL) {
        return 0;
    }
    while(list_ind < list_count && !retval) {
        retval = range_master(prefix, suffix, list[list_ind++], value, ordinal_int);
    }
    strtoa_free(list);
    return retval;
}

static int pattern_master(char *pattern, char *value)
{
    char **list        = NULL;
    uint   list_count  = 0;
    uint   list_ind    = 0;
    uint   retval      = 0;

    // Field key as pattern: nodes=node42[70-78;80],node43[10-20]
    if (strtoa(pattern, ',', &list, &list_count) == NULL) {
        return 0;
    }
    while(list_ind < list_count && !retval) {
        retval = pattern_slave(list[list_ind++], value);
    }
    strtoa_free(list);
    return retval;
}

static ear_field_t *field_traversal(ear_field_t *field, char *key, char *value)
{
    ear_field_t *field_returned = &null;
    int single_key = 0;

    if (field == NULL || field->is_null) {
        return &null;
    }
    debug("comparing [%s]=[%s] [%s][%s]=[%s]", key, value, field->key, field->key_concat, field->value);
    // A single key is a pure key, ie. 'node', not a concat format 'node.tag'
    if ((single_key = (strcmp(field->key, key) == 0)) || strcmp(field->key_concat, key) == 0) {
        if (value == NULL) {
            return single_key ? field : field->parent;
        } else if (pattern_master(field->value, value)) {
            return single_key ? field : field->parent;
        }
    }
    if ((field_returned = field_traversal(field->children, key, value))->is_null) {
        field_returned = field_traversal(field->sibling, key, value);
    }
    return field_returned;
}

static ear_field_t *field_search(ear_field_t *fields, char *fmt_complete)
{
    char  key[128] = {0};
    char *value = NULL;

    // Find the equal symbol and computing value pointer
    value = strchr(fmt_complete, '=');
    if (value != NULL) {
        // Empty values not allowed
        if (strlen(value+1) == 0) {
            return &null;
        }
        value += 1;
    }
    // Writing the key in its own buffer
    snprintf(key, sizeof(key), "%s", fmt_complete);
    if (value != NULL) {
        key[value - fmt_complete - 1] = '\0';
    }
    debug("searching [%s]=[%s]", key, value);
    // Field travel
    return field_traversal(fields, key, value);
}

static int is_parent(ear_field_t *parent, ear_field_t *child)
{
    if (parent->is_null || child->is_null) return 0;
    if (parent == child || parent == child->parent) return 1;
    // If is the same deep but is different field, then cant be parent
    if (parent->deep == child->deep) return 0;
    return is_parent(parent, child->children) | is_parent(parent, child->sibling);
}

ear_field_t *ear_field_search(ear_field_t *fields, char *key_value_fmt, ...)
{
    char         fmt_complete[256] = {0};
    ear_field_t *field_passed      = fields;
    ear_field_t *field_found       = NULL;
    va_list      args              = {0};
    char        *list_ptr          = fmt_complete;
    char       **list              = &list_ptr;
    uint         list_count        = 1;
    uint         list_ind          = 0;

    // Commas are present:
    //   - in values: anomaly.nodes=node4280,tag (1 comma)
    //   - in fields: anomaly=imcfreq nodes=node42[70-78,80],node43[10-20] (2 commas)
    if (fields == NULL || fields->is_null) {
        return &null;
    }
    // Variadic arguments processing
    va_start(args, key_value_fmt);
    vsprintf(fmt_complete, key_value_fmt, args);
    va_end(args);
    // Multiple search by comma separated formats
    if (strchr(fmt_complete, ',') != NULL) {
        if (strtoa(fmt_complete, ',', &list, &list_count) == NULL) {
            return &null;
        }
    }
    // With this first search, we use the first occurrence as the base search.
    if (!(field_found = field_search(fields, list[list_ind++]))->is_null) {
        field_passed = field_found;
    }
    printf("field_found: %s %s\n", field_found->key, field_found->value);
    while(list_ind < list_count && !field_passed->is_null) {
        // Going deep into the single field
        field_found = field_search(field_passed, list[list_ind++]);
        // If is comma separated search, and the field returned is not itself
        // or parent, it means it was found in a sibling field so does not
        // match all conditions of the search.
        if (!is_parent(field_passed, field_found)) {
            field_passed = field_passed->sibling;
            list_ind = 0;
        }
    }
    if (*list != list_ptr) {
        strtoa_free(list);
    }
    return field_found;
}