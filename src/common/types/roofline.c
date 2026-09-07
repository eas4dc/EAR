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
#include <common/types/roofline.h>

static int create_filename(char *filename, size_t size, char *path, char *prefix, char *architecture)
{
    int n = snprintf(filename, size, "%s%s%s.data", path, prefix, architecture);
    return (n > 0 && (size_t) n < size);
}

void roofline_print(roofline_t *roofline)
{
    debug("Roofline data: peak bandwidth -> %lf; peak gflops -> %lf; treshold -> %lf", roofline->peak_bandwidth,
          roofline->peak_gflops, roofline->threshold);
}

state_t load_roofline(char *path, char *architecture, roofline_t *final_roofline)
{
    if (!path || !architecture || !final_roofline) {
        return EAR_ERROR;
    }

    char filename[128] = "";
    if (!create_filename(filename, sizeof(filename), path, "roofline.", architecture)) {
        return EAR_ERROR;
    }
    FILE *f = fopen(filename, "rb");

    char buff[1024];

    if (access(filename, F_OK) != 0) {
        return EAR_ERROR;
    }

    if (NULL != fgets(buff, sizeof(buff), f)) {
        char *val = buff;
        double gflops, gbs;
        val            = strtok(val, " \t\n");
        gbs            = atof(val);
        val            = NULL;
        val            = strtok(val, " \t\n");
        gflops         = atof(val);
        val            = NULL;
        roofline_t aux = {gbs, gflops, gflops / gbs};
        memcpy(final_roofline, &aux, sizeof(roofline_t));
        fclose(f);
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}
