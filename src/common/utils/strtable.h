/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_STRTABLE_H
#define COMMON_UTILS_STRTABLE_H

#include <linux/limits.h>

#define STR_MAX_BUFFER  4096
#define STR_MAX_COLUMNS 20
#define STR_SIZE_BUFFER PIPE_BUF
#define STR_SYMBOL      "||"
#define STR_SYMBOL_VIS  "|||"
#define STR_RED         "<red>"
#define STR_GRE         "<gre>"
#define STR_YLW         "<ylw>"
#define STR_BLU         "<blu>"
#define STR_MGT         "<mgt>"
#define STR_CYA         "<cya>"
#define STR_COL_CHR     5
#define STR_MODE_DEF    0
#define STR_MODE_COL    1
#define STR_MODE_CSV    2
#define COL_RED         "\x1b[31m"
#define COL_GRE         "\x1b[32m"
#define COL_YLW         "\x1b[33m"
#define COL_BLU         "\x1b[34m"
#define COL_MGT         "\x1b[35m"
#define COL_CYA         "\x1b[36m"
#define COL_CLR         "\x1b[0m"
#define COL_CHR         5
#define CLR_CHR         4

typedef struct strtable_s {
    char buffer_raw[STR_MAX_BUFFER];
    char buffer_out[STR_MAX_BUFFER];
    unsigned int format[STR_MAX_COLUMNS];
    unsigned int blocked;
    unsigned int columns;
    unsigned int mode;
    int fd;
} strtable_t;

strtable_t tprintf_static __attribute__((weak));

// Obsolete, move to tprintf2 and convert tprintf2 to tprintf.
#define tprintf(...)                                                                                                   \
    {                                                                                                                  \
        snprintf(tprintf_static.buffer_raw, STR_SIZE_BUFFER - 1, __VA_ARGS__);                                         \
        tprintf_write(&tprintf_static);                                                                                \
    }

#define tprintf2(table, ...)                                                                                           \
    {                                                                                                                  \
        snprintf((table)->buffer_raw, STR_SIZE_BUFFER - 1, __VA_ARGS__);                                               \
        tprintf_write(table);                                                                                          \
    }

#define tsprintf(buffer, table, append, ...)                                                                           \
    {                                                                                                                  \
        snprintf((table)->buffer_raw, STR_SIZE_BUFFER - 1, __VA_ARGS__);                                               \
        tprintf_span(table);                                                                                           \
        strcpy(&buffer[append ? strlen(buffer) : 0], (table)->buffer_out);                                             \
    }

// Obsolete, move to tprintf_init2 and convert tprintf_init2 to tprintf_init.
int tprintf_init(int fd, int mode, char *format);

int tprintf_init2(strtable_t *t, int fd, int mode, char *format);

int tprintf_write(strtable_t *t);

int tprintf_span(strtable_t *t);

void tprintf_block(strtable_t *t, int block);

#endif