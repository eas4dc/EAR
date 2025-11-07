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

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/medoids.h>
#include <unistd.h>

static void create_filename(char *filename, char *path, char *prefix, char *architecture)
{
    strcat(filename, path);
    strcat(filename, prefix);
    strcat(filename, architecture);
    strcat(filename, ".data");
}

void medoids_print(medoids_t *phases)
{
    double *medoid = phases->cpu_bound;

    debug("K-medoids data: cpu-bound medoid id -> %u cpi -> %lf tpi -> %lf gbs -> %lf gflops -> %lf", CPU_BOUND,
          medoid[0], medoid[1], medoid[2], medoid[3]);

    medoid = phases->memory_bound;
    debug("K-medoids data: memory-bound medoid id -> %u cpi -> %lf tpi -> %lf gbs -> %lf gflops -> %lf", MEM_BOUND,
          medoid[0], medoid[1], medoid[2], medoid[3]);

    medoid = phases->mix;
    debug("K-medoids data: mix medoid id -> %u cpi -> %lf tpi -> %lf gbs -> %lf gflops -> %lf", MIX, medoid[0],
          medoid[1], medoid[2], medoid[3]);
}

void extremes_print(extremes_t *extremes)
{
    double *extreme = extremes->cpi_extreme;
    debug("K-medoids data: cpi mean -> %lf std -> %lf", extreme[1], extreme[0]);

    extreme = extremes->tpi_extreme;
    debug("K-medoids data: tpi mean -> %lf std -> %lf", extreme[1], extreme[0]);

    extreme = extremes->gbs_extreme;
    debug("K-medoids data: gbs mean -> %lf std -> %lf", extreme[1], extreme[0]);

    extreme = extremes->gflops_extreme;
    debug("K-medoids data: gflops mean -> %lf std -> %lf", extreme[1], extreme[0]);
}

state_t load_medoids(char *path, char *architecture, medoids_t *final_medoids)
{
    if (!path || !architecture || !final_medoids) {
        return EAR_ERROR;
    }
    char filename[128] = "";
    create_filename(filename, path, "medoids.", architecture);
    FILE *f = fopen(filename, "rb");

    char buff[4096];

    if (access(filename, F_OK) != 0) {
        return EAR_ERROR;
    }

    if (NULL != fgets(buff, sizeof(buff), f)) {
        char *val = buff;
        double medoids[N_MEDS * MED_ELEMS];
        for (int i = 0; i < N_MEDS; i++) {
            for (int j = 0; j < MED_ELEMS; j++) {
                val                        = strtok(val, " \t\n");
                medoids[i * MED_ELEMS + j] = atof(val);
                val                        = NULL;
            }
        }
        medoids_t aux = {{medoids[CPU_BOUND * MED_ELEMS], medoids[CPU_BOUND * MED_ELEMS + 1],
                          medoids[CPU_BOUND * MED_ELEMS + 2], medoids[CPU_BOUND * MED_ELEMS + 3]},
                         {medoids[MEM_BOUND * MED_ELEMS], medoids[MEM_BOUND * MED_ELEMS + 1],
                          medoids[MEM_BOUND * MED_ELEMS + 2], medoids[MEM_BOUND * MED_ELEMS + 3]},
                         {medoids[MIX * MED_ELEMS], medoids[MIX * MED_ELEMS + 1], medoids[MIX * MED_ELEMS + 2],
                          medoids[MIX * MED_ELEMS + 3]}};
        memcpy(final_medoids, &aux, sizeof(aux));
        fclose(f);
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}

state_t load_extremes(char *path, char *architecture, extremes_t *final_extremes)
{
    if (!path || !architecture || !final_extremes) {
        return EAR_ERROR;
    }
    char filename[128] = "";
    create_filename(filename, path, "extremes.", architecture);
    FILE *f = fopen(filename, "rb");

    char buff[4096];

    if (access(filename, F_OK) != 0) {
        return EAR_ERROR;
    }

    if (NULL != fgets(buff, sizeof(buff), f)) {
        char *val = buff;
        double extremes[N_EXTR * 2];
        for (int i = 0; i < N_EXTR; i++) {
            for (int j = 0; j < 2; j++) {
                val                 = strtok(val, " \t\n");
                extremes[i * 2 + j] = atof(val);
                val                 = NULL;
            }
        }
        extremes_t aux = {{extremes[ID_CPI * 2], extremes[ID_CPI * 2 + 1]},
                          {extremes[ID_TPI * 2], extremes[ID_TPI * 2 + 1]},
                          {extremes[ID_GBS * 2], extremes[ID_GBS * 2 + 1]},
                          {extremes[ID_GFLOPS * 2], extremes[ID_GFLOPS * 2 + 1]}};
        memcpy(final_extremes, &aux, sizeof(extremes_t));
        fclose(f);
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}
