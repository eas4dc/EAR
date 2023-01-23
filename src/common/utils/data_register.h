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

#ifndef COMMON_UTILS_DATA_REGISTER_H
#define COMMON_UTILS_DATA_REGISTER_H

#include <common/types/types.h>

// This is a memory register. You can add values and these values will
// be saved in a buffer. Later you can print these big array of values
// in a customized style.

typedef struct dreg_s {
    char  *buffer;
    uint   rows;
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
