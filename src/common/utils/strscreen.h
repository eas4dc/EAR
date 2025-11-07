/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_STRSCREEN_H
#define COMMON_UTILS_STRSCREEN_H

#include <common/types.h>

typedef struct screen_s {
    void *divs;
    cchar *title;
    int divs_count;
    char *buffer;       // unprocessed buffer
    char *buffer_bench; // working bench buffer
    char *buffer_final; // buffer ready to print
    char **matrix;
    char bckchr;
    int height;
    int width;
    int fd;
    int y;
} strscreen_t;

void scprintf_init(strscreen_t *screen, int height, int width, int fd, char bckchr);

void scprintf_divide(strscreen_t *screen, int p1[2], int p2[2], int *id, cchar *title);

// Writes the content of a buffer in the 'id' division.
void scsprintf(strscreen_t *s, int id, int append, int set_title, char *buffer);

// Returns and writes in the 'fd' output the whole formatted screen.
char *scprintf(strscreen_t *s);

#endif // COMMON_UTILS_STRSCREEN_H
