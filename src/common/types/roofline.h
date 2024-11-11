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

#ifndef _EAR_TYPES_ROOFLINE
#define _EAR_TYPES_ROOFLINE

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

typedef struct roofline
{
    double peak_bandwidth;
    double peak_gflops;
    double threshold;
} roofline_t;

state_t load_roofline(char *path, char *architecture, roofline_t *final_roofline);

void roofline_print(roofline_t *roofline, int t);

#endif
