/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/system/time.h>
#include <common/output/verbose.h>
#include <common/utils/data_register.h>

typedef struct dreg_col_s {
    char   fmt[8];
    char   show;
    uint   type;
    size_t size;
    size_t p;
} dreg_col_t;

#define DREG_TYPE_ULLONG 1
#define DREG_TYPE_UINT   2
#define DREG_TYPE_DOUBLE 3

void dreg_init(dreg_t *d, size_t colsize)
{
    d->colsize_full = colsize+sizeof(timestamp_t);
    d->colsize_mix  = colsize;
    d->buffer       = calloc(DREG_TYPE_ALLOC, d->colsize_full);
    d->rows         = 0;
    d->p            = 0;
}

void dreg_add(dreg_t *d, void *p)
{
    timestamp_get((timestamp_t *) &d->buffer[d->p]);
    d->p += sizeof(timestamp_t);
    memcpy(&d->buffer[d->p], p, d->colsize_mix);
    d->p += d->colsize_mix;
    d->rows += 1;
}

uint dreg_fmt_gettype(cchar *fmt)
{
    if (!strncmp(fmt, "llu", 3) || !strncmp(fmt, "-llu", 4)) {
        return DREG_TYPE_ULLONG;
    }
    if (!strncmp(fmt, "u", 1) || !strncmp(fmt, "-u", 2)) {
        return DREG_TYPE_UINT;
    }
    return DREG_TYPE_DOUBLE;
}

size_t dreg_fmt_getsize(cchar *fmt)
{
    uint type = dreg_fmt_gettype(fmt);

    if (type == DREG_TYPE_ULLONG) {
        return sizeof(ullong);
    }
    if (type == DREG_TYPE_UINT) {
        return sizeof(uint);
    }
    return sizeof(double);
}

void dreg_fmt_build(dreg_t *d, dreg_col_t *cols, uint *cols_count, cchar *fmt)
{
    char buffer[1024];
    size_t accum;
    uint all;
    char *p;
    uint c;
    
    all     = (fmt[0] == '*');
    fmt     = &fmt[all];
    accum   = sizeof(timestamp_t);
    c       = 0;

    if (all) {
        // Getting single type size
        cols[0].type = dreg_fmt_gettype(fmt);
        cols[0].size = dreg_fmt_getsize(fmt);
        // Counting the columns
        *cols_count = d->colsize_mix / cols[0].size;
        //
        for (c = 0; c < *cols_count; ++c) {
            sprintf(cols[c].fmt, "%%%s", fmt);
            cols[c].size = cols[0].size;
            cols[c].type = cols[0].type;
            cols[c].show = 1;
            cols[c].p    = accum;
            accum       += cols[0].size;
        }
        goto out;
    }
    // Not all the same
    strcpy(buffer, fmt);
    p = strtok(buffer, "|");
    //
    do {
        sprintf(cols[c].fmt, "%%%s", p);
        cols[c].size = dreg_fmt_getsize(p);
        cols[c].type = dreg_fmt_gettype(p);
        cols[c].show = !(p[0] == '-');
        cols[c].p    = accum;
        accum       += cols[c].size;
        *cols_count  = ++c;
    } while((p = strtok(NULL, "|")));
out:
    #if SHOW_DEBUGS
    for (c = 0; c < *cols_count; ++c) {
       debug("%u: '%s', type %u, show %d, %lu bytes, %lu p",
            c, cols[c].fmt, cols[c].type, cols[c].show, cols[c].size, cols[c].p);
    }
    #endif
    return;
}

void dreg_print(dreg_t *d, uint style, cchar *fmt, int fd)
{
    dreg_col_t cols[16];
    uint cols_count;
    uint percols;
    uint perrows;
    uint comma;
    char sep;
    char *q;
    uint r;
    uint c;

    percols = (fmt[0] == '!');
    perrows = !percols;
    fmt     = &fmt[percols];
    comma   = 0;
    sep     = ' '; 
    // Building the printing format
    dreg_fmt_build(d, cols, &cols_count, fmt);
    // Style
    if (style == DREG_STYLE_CSV) {
        sep = ';';
    }
    if (style == DREG_STYLE_ARRAY) {
        sep = ',';
    }
    // Print one by one
    if (perrows)
    {
    debug("per rows");
    dprintf(fd, "(");
    for (r = 0, q = d->buffer; r < d->rows; ++r, q += d->colsize_full) {
        for (c = 0; c < cols_count; ++c) {
            if (!cols[c].show) {
                continue;
            }
            if (comma) {
                dprintf(fd, "%c", sep);
            }
            #define drprintf(dtype, cast) \
            if (cols[c].type == dtype) { \
                cast *dp = (cast *) &q[cols[c].p]; \
                dprintf(fd, cols[c].fmt, *dp); \
            }
            drprintf(DREG_TYPE_ULLONG, ullong);
            drprintf(DREG_TYPE_UINT  , uint  );
            drprintf(DREG_TYPE_DOUBLE, double);
            ++comma;
        } 
    }
    dprintf(fd, ")\n");
    } 
    if (percols)
    {
    debug("per columns");
    for (c = 0; c < cols_count; ++c) {
        if (!cols[c].show) {
            continue;
        }
        comma = 0;
        dprintf(fd, "(");
        for (r = 0, q = d->buffer; r < d->rows; ++r, q += d->colsize_full) {
            if (comma) {
                dprintf(fd, "%c", sep);
            }
            drprintf(DREG_TYPE_ULLONG, ullong);
            drprintf(DREG_TYPE_UINT  , uint  );
            drprintf(DREG_TYPE_DOUBLE, double);
            ++comma;
            comma = 1; 
        }
        dprintf(fd, ")\n");
    }
    }
}

#if 0
int main(int argc, char *argv[])
{
    double test[2] = { 0.1, 0.2 };
    dreg_t d;
    
    dreg_init(&d, 2, sizeof(double)*2);
    dreg_add(&d, test);
    dreg_add(&d, test);
    //dreg_print(&d, DREG_STYLE_ARRAY, "!*0.2lf", verb_channel);
    dreg_print(&d, DREG_STYLE_ARRAY, "0.2lf|0.2lf", verb_channel);

    return 0;
}
#endif
