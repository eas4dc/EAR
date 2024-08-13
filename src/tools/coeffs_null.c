/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <common/output/verbose.h>
#include <common/types/coefficient.h>

static coefficient_t *coeffs;

void print_coefficients(coefficient_t *avg,int nump)
{
	int i;
	for (i=0;i<nump;i++){
		coeff_print(&avg[i]);
	}
}

int main(int argc, char *argv[])
{
    int fd;
    int i,j;
	char buffer[128];
	ulong maxf,minf;
	ulong fref,f;
	int nump;

    if (argc<4)
	{ 
		verbose(0, "Usage: %s file.name max.freq min.freq", argv[0]);
		verbose(0, "  Creates a coeffs file for the given range of frequencies with null values.");
        exit(1);
    }

	maxf = atol(argv[2]);
	minf = atol(argv[3]);
	
	sprintf(buffer,"coeffs.%s.default", argv[1]);
	verbose(0, "Creating null coeffs file '%s' with range '%lu-%lu' KHz", buffer, maxf, minf);

    fd=open(buffer,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd<0)
	{
		error("invalid coeffs path %s (%s)", buffer, strerror(errno));
		exit(1);
    }
	nump=((maxf-minf)/100000)+1;
	verbose(0, "Generating coeffs for %dx%d P_STATEs",nump,nump);
    coeffs=(coefficient_t*)calloc(sizeof(coefficient_t),nump*nump);
    if (coeffs==NULL) 
	{
		error("not enough memory");
		exit(1);
    }
	for (i=0;i<nump;i++)
	{
		fref=maxf-(i*100000);
		for (j=0;j<nump;j++)
		{
			f=maxf-(j*100000);
			coeffs[i*nump+j].available=0;
			coeffs[i*nump+j].pstate_ref=fref;
			coeffs[i*nump+j].pstate=f;
		}
	}
    /* The program reports coefficients in stdout and csv file */
    print_coefficients(coeffs,nump*nump);
	write(fd,coeffs,sizeof(coefficient_t)*(nump*nump));
	close(fd);
    return 0;
}
