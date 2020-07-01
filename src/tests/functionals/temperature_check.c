/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
*/
#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <metrics/common/msr.h>
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
