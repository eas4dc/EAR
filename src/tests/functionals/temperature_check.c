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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <metrics/common/omsr.h>
#include <metrics/temperature/temperature.h>
#include <common/types/configuration/cluster_conf.h>

int main(int argc,char *argv[])
{

    int fd_map[MAX_PACKAGES];
    unsigned long long values[NUM_SOCKETS];
    int i;

    init_temp_msr(fd_map);

    while (1)
    {
        read_temp_limit_msr(fd_map, values);
        for (i = 0; i < NUM_SOCKETS; i++)
        {
            printf("socket %d\t limit_reached: %llu\n", i, values[i]);
        }
        sleep(3);
    }


	return 0;
}
