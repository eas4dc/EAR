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
#include <common/system/file.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <common/string_enhanced.h>
#include <common/types/coefficient.h>

// Buffers
char buffer[SZ_PATH];
char path_root[SZ_PATH];
char path_output[SZ_PATH];

// Coefficients
coefficient_t *coeffs_accum;
int *coeffs_num;
int coeffs_max;

// Customize program
int opt_a;
int opt_o;

/*
 *
 * Work bench
 *
 */

int find(coefficient_t *coeff)
{
	int found;
	int last;
	int i;

	for (i = 0; i < 200; ++i)
	{
		last  = (coeffs_accum[i].pstate_ref == 0);
		found = (coeffs_accum[i].pstate_ref == coeff->pstate_ref) &&
				(coeffs_accum[i].pstate     == coeff->pstate);

		if (last) {
			coeffs_max = i + 1;
		}

		if (found || last) {
			return i;
		}
	}

	return -1;
}

void accum(coefficient_t *coeff, int i)
{
    coeffs_accum[i].pstate_ref = coeff->pstate_ref;
    coeffs_accum[i].pstate     = coeff->pstate;

	if (!coeff->available) {
		return;
	}
	
	coeffs_accum[i].available = 1;
	coeffs_num[i] += 1;

	if (coeff->pstate_ref == coeff->pstate) {
		return;
	}

	if (opt_a) {
		coeffs_accum[i].A += (1.0 / coeff->A);
		coeffs_accum[i].B += (1.0 / coeff->B);
		coeffs_accum[i].C += (1.0 / coeff->C);
		coeffs_accum[i].D += (1.0 / coeff->D);
		coeffs_accum[i].E += (1.0 / coeff->E);
		coeffs_accum[i].F += (1.0 / coeff->F);
	} else {
		coeffs_accum[i].A += coeff->A;
		coeffs_accum[i].B += coeff->B;
		coeffs_accum[i].C += coeff->C;
		coeffs_accum[i].D += coeff->D;
		coeffs_accum[i].E += coeff->E;
		coeffs_accum[i].F += coeff->F;
	}
}

static void compute()
{
	double num;
	int i;

    for (i = 0; i < coeffs_max; ++i)
    {
        if (coeffs_accum[i].pstate_ref == coeffs_accum[i].pstate) {
            coeffs_accum[i].A = 1.0;
            coeffs_accum[i].D = 1.0;
        } else if (coeffs_accum[i].available)
        {
            num = (double) coeffs_num[i];

            if (opt_a) {
                coeffs_accum[i].A = num / coeffs_accum[i].A;
                coeffs_accum[i].B = num / coeffs_accum[i].B;
                coeffs_accum[i].C = num / coeffs_accum[i].C;
                coeffs_accum[i].D = num / coeffs_accum[i].D;
                coeffs_accum[i].E = num / coeffs_accum[i].E;
                coeffs_accum[i].F = num / coeffs_accum[i].F;
            } else {
                coeffs_accum[i].A = coeffs_accum[i].A / num;
                coeffs_accum[i].B = coeffs_accum[i].B / num;
                coeffs_accum[i].C = coeffs_accum[i].C / num;
                coeffs_accum[i].D = coeffs_accum[i].D / num;
                coeffs_accum[i].E = coeffs_accum[i].E / num;
                coeffs_accum[i].F = coeffs_accum[i].F / num;
            }
        }
    }
}

static void print()
{
	int i;

	//
	tprintf_init(fderr, STR_MODE_DEF, "10 10 6 2 13 10 10 10 10 10");

	//
	tprintf("F. from||F. to||N||Av||| A|| B|| C|| D|| E|| F|| G");

	//
	for (i = 0; i < coeffs_max; ++i)
	{
		tprintf("%lu||%lu||%d||%d||| %0.3lf||%0.3lf||%0.3lf||%0.3lf||%0.3lf||%0.3lf",
			coeffs_accum[i].pstate_ref, coeffs_accum[i].pstate, coeffs_num[i], coeffs_accum[i].available,
			coeffs_accum[i].A, coeffs_accum[i].B, coeffs_accum[i].C,
			coeffs_accum[i].D, coeffs_accum[i].E, coeffs_accum[i].F);

		#if 0
		coeff_print(&coeffs_accum[i]);
		#endif
	}
}

/*
 *
 * Read and write
 *
 */

static void write_coefficients()
{
	#define SZ_COEFF sizeof(coefficient_t)

	int fd;
	int i;

	verbose(0, "-----------------------------------------------------------------------------------------------------");

	if (!opt_o) {
		xsprintf(path_output, "%s/coeffs.default", path_root);
	}

	fd = -1;

    //
	fd = open(path_output, O_WRONLY | O_CREAT | O_TRUNC,
			  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);

    if (fd < 0) {
        printf("ERROR while opening file %s (%s)\n", path_output, strerror(errno));
		return;
    }

	//
	for (i = 0; i < coeffs_max; ++i)
    {
		if (write(fd, &coeffs_accum[i], SZ_COEFF) != SZ_COEFF) {
 			printf("ERROR while writting coefficients file\n");
			exit(1);
		}
    }
	
	verbose(0, "file written '%s'\n", path_output);

	close(fd);
}

static void read_file(char *path, char *node)
{
	coefficient_t coeff;
	size_t r;
	int fd;
	int i;

	fd = open(path, O_RDONLY);
	printf("Reading %s file\n",path);

	if (fd < 0) {
		printf("ERROR while opening file %s (%s)\n", path, strerror(errno));
 		exit(1);
	}

	//
	while((r = read(fd, &coeff, SZ_COEFF)) > 0)
	{
		if (r == SZ_COEFF)
		{
			i = find(&coeff);
	
			if (i >= 0) {
				accum(&coeff, i);
			}
		} else {
			printf("ERROR while reading file %s\n", strerror(errno));
			exit(1);
		}
	}

	close(fd);
}


static void read_coefficients(int argc, char *argv[], int n_nodes)
{
	char *node_name;
	int i;

	for (i = (argc - n_nodes); i < argc; i++)
	{
		printf("Processing node %d node %s\n",i , argv[i]);
		node_name = argv[i];

		/* The program reports coefficients in stdout and csv file */
		xsprintf(buffer, "%s/coeffs.%s", path_root, node_name);

		if (file_is_regular(buffer))
		{
			read_file(buffer, node_name);
		}
	}
}

/*
 *
 * Initialization
 *
 */


static int usage(int argc, char *argv[])
{
	int c;

	if (argc < 3)
	{
		verbose(0, "Usage: %s [OPTIONS...] coeffs.path node.list\n", argv[0]);
		verbose(0, "\nOptions:");
		verbose(0, "\t-A\tDoes the armonic mean, reducing the weight");
		verbose(0, "\t\tof radical coefficient values.");
		verbose(0, "\t-O <p>\tSaves the default coefficients file in a");
		verbose(0, "\t\tfile of custom location.");
		verbose(0, "  The coeffs.path includes the island.");
		verbose(0, "  The node.list is splitted by spaces.");
		
		exit(1);
	}

	// Basic parametrs

	while ((c = getopt (argc, argv, "AO:")) != -1)
	{
		switch (c)
		{
			case 'A':
				opt_a = 1;
				break;
			case 'O':
				strcpy(path_output, optarg);
				opt_o = 1;
				break;
			case '?':
				break;
			default:
				abort();
		}
	}

	return 1+ opt_a + (opt_o * 2);
}

void init()
{
	// Number of P_STATEs, more than 200 is impossible
	coeffs_accum = calloc(200, sizeof(coefficient_t));
	coeffs_num   = calloc(200, sizeof(unsigned int));
}

int main(int argc, char *argv[])
{
	int no_opt;
	int n_nodes;
	char **argvcopy = argv;

	// Initialization
	no_opt = usage(argc, argv);
	sprintf(path_root, argv[no_opt]);
	n_nodes = argc - (no_opt + 1 );

	init();

	//
	read_coefficients(argc, argvcopy, n_nodes);

	//	
	compute();

	print();

	//
	write_coefficients();

	return 0;
}
