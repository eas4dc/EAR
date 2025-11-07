/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_DATA_REGISTER_H
#define COMMON_UTILS_DATA_REGISTER_H

#include <common/types/types.h>

// This is a memory register. You can add values and these values will
// be saved in a buffer. Later you can print these big array of values
// in a customized style.

typedef struct dreg_s {
    char *buffer;
    uint rows;
    size_t colsize_full;
    size_t colsize_mix;
    size_t p;
} dreg_t;

#define DREG_STYLE_TABLE 0
#define DREG_STYLE_CSV   1
#define DREG_STYLE_ARRAY 2
#define DREG_TYPE_ALLOC  1000

void dreg_init(dreg_t *d, size_t colsize);

void dreg_add(dreg_t *d, void *p);

void dreg_print(dreg_t *d, uint style, cchar *fmt, int fd);

#endif
