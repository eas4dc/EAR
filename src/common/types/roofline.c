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
#include <common/types/roofline.h>
#include <common/output/verbose.h>

static void create_filename(char *filename, char *path, char *prefix, char *architecture)
{
    strcat(filename, path);
    strcat(filename, prefix);
    strcat(filename, architecture);
    strcat(filename, ".data");
}

void roofline_print(roofline_t *roofline, int t)
{
    /* var "t" is expected to be INFO_CLASSIFY*/
    /* We indicate it through a parameter since INFO_CLASSIFY is a constant of "clasify.c"*/
    verbose(t, "roofline data: peak bandwidth %lf peak gflops %lf treshold %lf\n",
           roofline->peak_bandwidth, roofline->peak_gflops, roofline->threshold);
}

state_t load_roofline(char *path, char *architecture, roofline_t *final_roofline)
{
	if (!path || !architecture || !final_roofline) {
		return EAR_ERROR;
	}

    char filename[128] = "";
    create_filename(filename, path, "roofline.", architecture);
    FILE *f = fopen(filename, "rb");

    char buff[1024];
    
    if (access(filename, F_OK) != 0)
    {
        return EAR_ERROR;
    }

    if (NULL != fgets(buff, sizeof(buff), f))
    {
        char *val = buff;
        double gflops, gbs;
        val = strtok(val, " \t\n");
        gbs = atof(val);
        val = NULL;
        val = strtok(val, " \t\n");
        gflops = atof(val);
        val = NULL;
        roofline_t aux = {gbs, gflops, gflops / gbs};
        memcpy(final_roofline, &aux, sizeof(roofline_t));
        fclose(f);
        return EAR_SUCCESS;
    }
    return EAR_ERROR;
}
