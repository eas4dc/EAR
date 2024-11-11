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

#ifndef _EAR_TYPES_MEDOID
#define _EAR_TYPES_MEDOID

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

#define N_MEDS 3
#define MED_ELEMS 4
#define CPU_BOUND 0
#define MEM_BOUND 1
#define MIX 2

#define N_EXTR 4 // number of metrics used
#define ID_CPI 0
#define ID_TPI 1
#define ID_GBS 2
#define ID_GFLOPS 3

typedef struct medoids
{
    double cpu_bound[MED_ELEMS];
    double memory_bound[MED_ELEMS];
    double mix[MED_ELEMS];
} medoids_t;

typedef struct extremes
{
    double cpi_extreme[2];
    double tpi_extreme[2];
    double gbs_extreme[2];
    double gflops_extreme[2];
} extremes_t;

state_t load_medoids(char *path, char *architecture, medoids_t *final_medoids);

state_t load_extremes(char *path, char *architecture, extremes_t *final_extremes);

void medoids_print(medoids_t *phases, int t);

void extremes_print(extremes_t *extremes, int t);

#endif
