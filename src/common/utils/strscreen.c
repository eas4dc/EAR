/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/output/verbose.h>
#include <common/utils/strscreen.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define Y            0
#define X            1
#define clrscr()     printf("\e[1;1H\e[2J")
#define buffer(y, x) s->buffer[(y) * s->width + (x)]

static void clean_buffer_background(strscreen_t *s)
{
    int y, x = 0;
    for (y = 0; y < s->height; ++y) {
        for (x = 0; x < s->width; ++x) {
            buffer(y, x) = s->bckchr;
        }
        buffer(y, x - 1) = '\n';
    }
    buffer(y - 1, x) = '\0';
}

void scprintf_init(strscreen_t *s, int height, int width, int fd, char bckchr)
{
    s->buffer       = calloc((width + 1 + 000) * height, sizeof(char));
    s->buffer_final = calloc((width + 1 + 100) * height, sizeof(char)); //+100 for the colors
    s->buffer_bench = calloc((width + 1 + 100) * height, sizeof(char)); //+100 for the colors
    s->divs         = calloc(16, sizeof(strscreen_t));                  // 16 subdivisions is fairly enough
    s->height       = height;
    s->width        = width;
    s->fd           = fd;
    s->bckchr       = bckchr;
    clean_buffer_background(s);
}

static void clean_matrix_background(strscreen_t *s)
{
    int x, y;
    for (y = 0; y < s->height; ++y)
        for (x = 0; x < s->width; ++x) {
            s->matrix[y][x] = s->bckchr;
        }
}

void scprintf_divide(strscreen_t *s, int p1[2], int p2[2], int *id, cchar *title)
{
    strscreen_t *div;
    int y;

    *id = s->divs_count;
    div = &(((strscreen_t *) s->divs)[*id]);
    // X: #######\n <- (n-2 = '#') (n-1 = '\n')
    // Y: #######\0 <- (n-1 = '#')
    if (p2[X] >= s->width)
        p2[X] = s->width - 3;
    if (p2[Y] >= s->height)
        p2[Y] = s->height - 2;
    div->title  = title;
    div->buffer = s->buffer;
    div->width  = (p2[X] - p1[X]) + 1; // +1 because both
    div->height = (p2[Y] - p1[Y]) + 1; // are inclusive.
    div->bckchr = ' ';
    div->matrix = calloc(div->height, sizeof(char *));
    // Setting division pointers
    for (y = 0; y < div->height; ++y) {
        div->matrix[y] = &div->buffer[(p1[Y] + y) * s->width + p1[X]];
    }
    s->divs_count++;
    clean_matrix_background(div);
}

void scsprintf(strscreen_t *s, int div_index, int append, int set_title, char *buffer)
{
    strscreen_t *div = &(((strscreen_t *) s->divs)[div_index]);
    size_t length    = strlen(buffer);
    int c, x;

    // Cleaning
    if (!append) {
        clean_matrix_background(div);
        if (set_title) {
            for (x = 1; x < (div->width - 1); ++x) {
                div->matrix[0][x] = '=';
            }
            div->matrix[0][3]                      = ' ';
            div->matrix[0][strlen(div->title) + 4] = ' ';
            memcpy(&div->matrix[0][4], div->title, strlen(div->title));
        }
        div->y = set_title;
    }
    // Copying characters
    for (c = 0, x = 1; c < length && div->y < div->height; ++c) {
        if (x == (div->width - 1)) {
            while (buffer[c] != '\n' && buffer[c] != '\0') {
                c++;
            }
        }
        if (buffer[c] == '\0') {
            continue;
        }
        if (buffer[c] == '\n') {
            x = 0;
            div->y++;
        } else {
            div->matrix[div->y][x] = buffer[c];
        }
        x++;
    }
}

static void clean_tag(strscreen_t *s, char *tag, char *new_string, uint spaces)
{
    static char *thirty_spaces = "                              ";
    char *ct; // Color tag
    ulong i, j, k;

    while ((ct = strstr(s->buffer_final, tag)) != NULL) {
        // Getting sizes
        i = ct - s->buffer_final;
        j = strlen(tag);
        k = strlen(new_string);
        // Copying the text after the <tag>
        strcpy(s->buffer_bench, &s->buffer_final[i + j]);
        // Copying the text after the <tag> before the color and the spaces
        strcpy(&s->buffer_final[i + k + spaces], s->buffer_bench);
        // Copying the color in the place where the tag was
        strncpy(&s->buffer_final[i], new_string, strlen(new_string));
        // Copying the spaces
        strncpy(&s->buffer_final[i + k], thirty_spaces, spaces);
    }
}

char *scprintf(strscreen_t *s)
{
    // Copying original buffer
    strcpy(&s->buffer_final[0], &s->buffer[0]);
    // Replacing color tags
    clean_tag(s, "<red>", "\x1b[31m", 0);
    clean_tag(s, "</red>", "\x1b[0m", 5 + 6);
    clean_tag(s, "<green>", "\x1b[32m", 0);
    clean_tag(s, "</green>", "\x1b[0m", 7 + 8);
    // Returning to the terminal original position
    clrscr();
    // Printing in case valid file descriptor
    if (s->fd >= 0) {
        dprintf(s->fd, "%s", s->buffer_final);
    }
    return s->buffer_final;
}
