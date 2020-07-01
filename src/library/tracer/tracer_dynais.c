/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/config.h>
#include <common/system/file.h>
#include <common/system/time.h>
#include <common/output/verbose.h>
#include <library/tracer/tracer_dynais.h>


static char buffer[SZ_PATH];
static int enabled=0;
static int fd;

void traces_init(char *app,int global_rank, int local_rank, int nodes, int mpis, int ppn)
{
	char myhost[128];
	
	char *pathname = getenv("SLURM_EAR_TRACE_PATH");

	if (pathname!=NULL){ 
		debug("Dynais traces ON. Traces %s",pathname);
	}
	else {
		debug("SLURM_EAR_TRACE_PATH is not defined");
	}

	if (enabled || pathname == NULL) {
		return;
	}

	gethostname(myhost,sizeof(myhost));
	sprintf(buffer, "%s/%s.%s.%d", pathname,app,myhost,global_rank);
	debug("saving trace in %s\n", buffer);

	fd = open(buffer, F_WR | F_CR | F_TR, F_UR | F_UW | F_GR | F_GW | F_OR | F_OW);
	enabled = (fd >= 0);
}

void traces_mpi_call(int global_rank, int local_rank, ulong ev, ulong a1, ulong a2, ulong a3)
{
	unsigned long *b;
	ssize_t w;

	if (!enabled) {
		return;
	}

	b = (unsigned long *) buffer;
	b[0] = ev;
	b[1] = (ulong) timestamp_getconvert(TIME_USECS);

	write(fd, b, 16);
}

void traces_mpi_end()
{
	if (!enabled) {
		return;
	}

	enabled = 0;

	close(fd);
}
