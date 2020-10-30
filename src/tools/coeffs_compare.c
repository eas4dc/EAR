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

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <common/types/coefficient.h>

#define POWER 			0
#define TIME 			1
#define BOTH			2
#define IS_DIFFERENT	0
#define IS_EQUAL		1
#define MAX_DIFF		0.1
#define DEF_FREQ		2401000

char buffer[SZ_PATH];
int num_pstates;
int mode=BOTH;
int file_size;

void read_file(char *path, coefficient_t *coeffs)
{
	int rfd = open(path, O_RDONLY);

	if (rfd < 0) {
		error("Error opening file %s", strerror(errno));
		return;
	}

	if (read(rfd, coeffs, file_size) != file_size) {
		error("Error reading coefficients for path %s (%s)", path, strerror(errno));
	}

	close(rfd);
}

int equal(double A, double B, double th)
{
	double d;
	double aA,aB;
	aA=fabs(A);
	aB=fabs(B);

	if (A==B) return 1;

	if (aA > aB) {
		d = (aA - aB) / aB;
	} else {
		d = (aB - aA) / aA;
	}

	return (d < th);
}

int compare_coefficients(coefficient_t *avg, coefficient_t *per_node)
{
	int are_equal = 1;
	#if 0
	if ((!equal(avg->A, per_node->A, MAX_DIFF))|| (!equal(avg->B, per_node->B, MAX_DIFF)) || (!equal(avg->C, per_node->C, MAX_DIFF)) ||
	(!equal(avg->D, per_node->D, MAX_DIFF)) || (!equal(avg->E, per_node->E, MAX_DIFF)) || (!equal(avg->F, per_node->F, MAX_DIFF))) are_equal=0;
	#endif
	switch (mode)
	{
	case POWER:
		if (!equal(avg->C, per_node->C, MAX_DIFF)) are_equal=0;
		break;
	case TIME:
		if (!equal(avg->F, per_node->F, MAX_DIFF)) are_equal=0;
		break;
	case BOTH:
		if ((!equal(avg->C, per_node->C, MAX_DIFF))|| (!equal(avg->F, per_node->F, MAX_DIFF))) are_equal=0;
		break;
	}


	if (are_equal) {
		return IS_EQUAL;
	} else {
		return IS_DIFFERENT;
	}
}

double compute_diff(coefficient_t *avg, coefficient_t *per_node)
{
	#if 0
	return ((fabs(avg->A-per_node->A)/avg->A+fabs(avg->B-per_node->B)/avg->B+fabs(avg->C-per_node->C)/avg->C+fabs(avg->D-per_node->D)/avg->D+
	fabs(avg->E-per_node->E)/avg->E+fabs(avg->F-per_node->F)/avg->F)/6.0);
	#endif
	switch (mode)
	{
	case POWER:
		return fabs(avg->C-per_node->C)/avg->C;
		break;
	case TIME:
		return fabs(avg->F-per_node->F)/avg->F;
		break;
	case BOTH:
		return ((fabs(avg->C-per_node->C)/avg->C+fabs(avg->F-per_node->F)/avg->F)/2.0);
		break;
	}
	return 0.0;
}


int main(int argc, char *argv[])
{
	int fd_avg,fd_per_node;
	int i,j;
	coefficient_t *avg,*per_node;
	int avg_pstates,per_node_pstates;
	int total_warnings;

	if (argc < 4)
	{
		verbose(0, "Usage: %s path.coeffs.1 path.coeffs.2 [power|time|both]", argv[0]);
		verbose(0, "  Compares two coefficient files.");
		exit(1);
	}

	if (strcmp(argv[3],"power")==0) mode=POWER;
	else if (strcmp(argv[3],"time")==0) mode=TIME;
	else mode=BOTH;

	/* Reading avg coefficients */
	fd_avg=open(argv[1],O_RDONLY);
	if (fd_avg < 0)
	{
		verbose(0, "Invalid coeffs avg path %s (%s)", argv[1], strerror(errno));
		exit(1);
	}
	file_size = lseek(fd_avg, 0, SEEK_END);
	lseek(fd_avg,0,SEEK_SET);
	num_pstates = file_size / sizeof(coefficient_t);
	verbose(0, "Coefficients file size %d, pstates %d", file_size, num_pstates);
	avg_pstates=num_pstates;
	avg=(coefficient_t *)calloc(1,file_size);

	if (avg == NULL ){
		verbose(0, "Not enough memory ");
		exit(0);
	}

	if (read(fd_avg,avg,file_size)!=file_size){
		error("while reading coefficients");
		exit(0);
	}
	close(fd_avg);

	/* Reading per-node coefficients */

    fd_per_node=open(argv[2],O_RDONLY);
    if (fd_per_node < 0)
    {
		error("Invalid coeffs per_node path %s (%s)", argv[2], strerror(errno));
        exit(1);
    }
    file_size = lseek(fd_per_node, 0, SEEK_END);
    lseek(fd_per_node,0,SEEK_SET);
    num_pstates = file_size / sizeof(coefficient_t);
	per_node_pstates=num_pstates;
    verbose(0, "Coefficients file size %d, pstates %d", file_size, num_pstates);
    per_node=(coefficient_t *)calloc(1,file_size);

    if (per_node == NULL ){
        verbose(0, "Not enough memory ");
        exit(0);
    }

    if (read(fd_per_node,per_node,file_size)!=file_size){
		error("while reading coefficients");
        exit(0);
    }
    close(fd_per_node);


	total_warnings=0;
	for (i = 0; i <avg_pstates; i++)
	{
		for (j=0;j<per_node_pstates;j++)
		{
			if ((avg[i].pstate_ref == per_node[j].pstate_ref) &&
				(avg[i].pstate == per_node[j].pstate) &&
				(avg[i].available) &&
				(per_node[j].available))
			{
				if (compare_coefficients(&avg[i], &per_node[j]) == IS_DIFFERENT)
				{
					total_warnings++;
					verbose(0, "%s pstate_ref %lu and pstate %lu are different in %lf",
						argv[2], avg[i].pstate_ref, avg[i].pstate, compute_diff(&avg[i], &per_node[j]));
				}
			}
		}
	}

	if (total_warnings) verbose(0, "Total warnings %d", total_warnings);
	return 0;
}
