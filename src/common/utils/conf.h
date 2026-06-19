/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_CONF_H
#define COMMON_UTILS_CONF_H
// clang-format off

// Examples of ear.conf opening:
//    ear_conf_t *c = ear_conf_read("/etc/ear/ear.conf");
//    ear_conf_print(c);
//
// Reading a simple 'DBIp' field:
//     ear_field_search(c->fields, "dbip");
//
// Searching a tag from a island node where our node is in the value. Note that
// the subkey selection 'tag' comes at the end in the search string:
//    ear_field_search(c->fields, "island.nodes=%s,tag", hostname)->value
//
// Searching with multiple conditions and selecting the subkey 'max':
//     ear_field_search(c->fields, "anomaly=imcfreq,anomaly.nodes=%s,max", hostname)->value
//
// You can also iterate over fields:
//    f = c->fields;
//    do {
//        f = ear_field_search(f->next, "island,tag");
//        printf("%s\n", f->value_safe);
//    }
//    while (!f->is_null);
//

typedef struct ear_conf_t  ear_conf_t; // ear conf
typedef struct ear_field_t ear_field_t;

// This configuration is linked list. A key has a link to another key of the
// same level (sibling), and to subkey (children). The subkey has a link to
// a subkey of the same level ('sibling'), and a sub2key ('children'), and so
// on. You can also iterate continuously, by using the 'next' pointer.

struct ear_field_t {
    ear_field_t *(*search) (ear_field_t *fields, char *fmt_key_value, ...);
    ear_field_t   *children; // sub-keys
    ear_field_t   *sibling;  // same level keys
    ear_field_t   *next; // To iterate like an array
    ear_field_t   *parent;
    char          *key;
    char          *value;
    char          *value_safe; // never null
    int            value_toint;
    char          *key_concat;
    int            is_null;
    int            deep;
};

struct ear_conf_t {
    ear_field_t *fields;
    char *file_path;
};

// Reads the ear.conf file.
ear_conf_t* ear_conf_read(char *file_path);

void ear_conf_print(ear_conf_t *fconf);

// Used to search values given keys. There are examples at the top of this header.
ear_field_t *ear_field_search(ear_field_t *fields, char *fmt_key_value, ...);

// clang-format on
#endif // COMMON_UTILS_CONF_H