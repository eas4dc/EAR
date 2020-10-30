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
	
	char *pathname = getenv(VAR_OPT_TRAC);

	if (pathname!=NULL){ 
		debug("Dynais traces ON. Traces %s",pathname);
	}
	else {
		debug("%s is not defined", VAR_OPT_TRAC);
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
