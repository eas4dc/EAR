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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/projection.h>
#include <common/types/application.h>
#include <common/types/coefficient.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/database/db_helper.h>

// buffers
static char name_node[SZ_NAME_SHORT];
static char path_coeffs[SZ_PATH];
static char path_input[SZ_PATH];
static char buffer[SZ_PATH];

/* node */
static cluster_conf_t conf;

/* allocation */
static coefficient_t *coeffs;
static application_t *apps;
static application_t *mrgd;
static double *prjs;
static double *errs;
static double *errs_gen;
static double *errs_med;

/* indexes */
static int n_coeffs;
static int n_apps;
static int n_mrgd;
static int n_prjs;
static int n_errs;

// General metrics
static uint frq_base;

/* other */
static uint opt_a;
static uint opt_s;
static uint opt_g;
static uint opt_d;
static uint opt_i;
static uint opt_h;
static uint opt_c;

/*
 *
 * Work bench
 *
 */

static int find(application_t *apps, int n_apps, char *app_id, uint frq_base)
{
	int equal_id = 0;
	int equal_fq = 0;
	int i;

	for(i = 0; i < n_apps; ++i)
	{
		equal_id = strcmp(app_id, apps[i].job.app_id) == 0;
		equal_fq = frq_base == apps[i].signature.def_f;

		if (equal_id && equal_fq) {
			return i;
		}
	}

	return -1;
}

static void merge()
{
	char *app_id;
	double cpi, tpi, pow, tim, counter;
	int equal_id, equal_f;
	int i, j, k;
	uint freq;

	if (n_apps == 0 || n_coeffs == 0) {
		return;
	}

	// n_apps * n_coeffs * (cpi + time + power)
	n_prjs = n_apps * n_coeffs * 3;
	n_errs = n_apps * n_coeffs * 3;

	// Allocation
	mrgd = (application_t *) calloc(n_apps, sizeof(application_t));
	prjs = (double *) calloc(n_prjs, sizeof(double));
	errs = (double *) calloc(n_errs, sizeof(double));
	errs_med = (double *) calloc(4 * n_coeffs, sizeof(double));
	errs_gen = (double *) calloc(4, sizeof(double));

	// Initialization
	for(i = 0; i < n_apps; ++i) {
		mrgd[i].job.app_id[0] = '\0';
	}

	//
	for(i = 0, j = 0; i < n_apps; ++i)
	{
		freq = apps[i].signature.def_f;
		app_id = apps[i].job.app_id;

		if (find(mrgd, j, app_id, freq) == -1)
		{
			memcpy(&mrgd[j], &apps[i], sizeof(application_t));

			tpi = apps[i].signature.TPI;
			cpi = apps[i].signature.CPI;
			tim = apps[i].signature.time;
			pow = apps[i].signature.DC_power;
			counter = 1.0;

			for(k = i + 1; k < n_apps; ++k)
			{
				equal_id = strcmp(app_id, apps[k].job.app_id) == 0;
				equal_f  = freq == apps[k].signature.def_f;

				if (equal_id && equal_f)
				{
					tpi += apps[k].signature.TPI;
					cpi += apps[k].signature.CPI;
					tim += apps[k].signature.time;
					pow += apps[k].signature.DC_power;
					counter += 1.0;
				}
			}

			mrgd[j].signature.DC_power = pow / counter;
			mrgd[j].signature.time     = tim / counter;
			mrgd[j].signature.TPI      = tpi / counter;
			mrgd[j].signature.CPI      = cpi / counter;

			j += 1;
		}
	}

	n_mrgd = j;
}

static void compute()
{
	//double cpi_sign, tpi_sign, tim_sign, pow_sign;
	int c, a, n, i;

	if (n_apps == 0 || n_coeffs == 0) {
		return;
	}

	// Initializing columns
	for (a = 0; a < n_mrgd; ++a)
	{
		if (mrgd[a].signature.def_f == frq_base)
		{
			for (c = 0, n = 0; c < n_coeffs; c++)
            {
                if (coeffs[c].available && coeffs[c].pstate_ref == frq_base)
                {
                    n += find(mrgd, n_mrgd, mrgd[a].job.app_id, coeffs[c].pstate) != -1;
				}
			}

			// No more than one application to compare
			if (n <= 1) {
				continue;
			}

			for (c = 0; c < n_coeffs; c++)
			{
				if (coeffs[c].available && coeffs[c].pstate_ref == frq_base)
				{
					i = (n_coeffs * 3) * a + (3) * c;
					double *prjs_b = &prjs[i];
					double *errs_b = &errs[i];	

					// Error
					prjs_b[c+0] = basic_project_cpi(&mrgd[a].signature, &coeffs[c]);
					prjs_b[c+1] = basic_project_time(&mrgd[a].signature, &coeffs[c]);
					prjs_b[c+2] = basic_project_power(&mrgd[a].signature, &coeffs[c]);

					// Fin that application for that coefficient
					n = find(mrgd, n_mrgd, mrgd[a].job.app_id, coeffs[c].pstate);

					if (n != -1)
					{
						application_t *m = &mrgd[n];

						errs_b[c+0] = fabs((1.0 - (m->signature.CPI      / prjs_b[c+0])) * 100.0);
						errs_b[c+1] = fabs((1.0 - (m->signature.time     / prjs_b[c+1])) * 100.0);
						errs_b[c+2] = fabs((1.0 - (m->signature.DC_power / prjs_b[c+2])) * 100.0);
					}

					// Medium error
					if (coeffs[c].pstate != frq_base)
					{
						i = 4 * c;
						errs_med[i+0] += errs_b[c+0];
						errs_med[i+1] += errs_b[c+1];
						errs_med[i+2] += errs_b[c+2];
						errs_med[i+3] += 1.0;
					}
				}
			}
		}
	}

	// Coefficients medium error
	for (c = 0; c < n_coeffs * 4; c += 4)
	{
		if (errs_med[c+3] > 0.1)
		{
			// Medium error
			errs_med[c+0] = errs_med[c+0] / errs_med[c+3];
			errs_med[c+1] = errs_med[c+1] / errs_med[c+3];
			errs_med[c+2] = errs_med[c+2] / errs_med[c+3];

			// General error
			errs_gen[0] += errs_med[c+0];
			errs_gen[1] += errs_med[c+1];
			errs_gen[2] += errs_med[c+2];
			errs_gen[3] += 1.0;
		}
	}

	// General medium error
	errs_gen[0] = errs_gen[0] / errs_gen[3];
	errs_gen[1] = errs_gen[1] / errs_gen[3];
	errs_gen[2] = errs_gen[2] / errs_gen[3];
}

void print()
{
	#define LINE "-----------------------------------------------------------------------------------------------------"
	int c, a, n, i;
	char *col_time;
	char *col_powe;
	
	// Headers
	if (opt_c) {
		if (!opt_g) {
			verbose(0, "App Name;Freq. From;Freq. To;T. Real;T. Proj;T. Err;P. Real;P. Proj;P. Err");
		} else if(opt_g && opt_h) {
			verbose(0, "Node name;Freq. from;T. Err.;P. Err.");
		}
	}

	if (!opt_c) {
		tprintf_init(verb_channel, STR_MODE_COL, "18 11 15 12 12 15 12 12");

		if (opt_g && opt_h) {
			tprintf("Node name||Frequency|| | T. Real||T. Proj||T. Err|| | P. Real||P. Proj||P. Err");
			tprintf("---------||---------|| | -------||-------||------|| | -------||-------||------");
		}
	}

	// When no apps or coeffs found
	if (n_apps == 0 || n_coeffs == 0)
	{
		if (opt_g && !opt_h)
		{
        	if (opt_c) {
            	verbose(0, "%s;-;-;-", name_node);
        	} else {
            	tprintf("%s||-|| | -||-||-|| | -||-||-", name_node);
        	}
		}

		return;
	}

	for (a = 0; !opt_g && a < n_mrgd; ++a)
	{
		if (mrgd[a].signature.def_f == frq_base)
		{
			for (c = 0, n = 0; c < n_coeffs; c++)
			{
				if (coeffs[c].available && coeffs[c].pstate_ref == frq_base) {
					n += find(mrgd, n_mrgd, mrgd[a].job.app_id, coeffs[c].pstate) != -1;
				}
			}

			// No more than one application to compare
			if (n <= 1) {
				continue;
			}

			if (!opt_c) {
				tprintf("%s||@%lu|| | T. Real||T. Proj||T. Err|| | P. Real||P. Proj||P. Err",
					mrgd[a].job.app_id, mrgd[a].signature.def_f);
			}

			for (c = 0; c < n_coeffs; c++)
			{
				if (coeffs[c].available && coeffs[c].pstate_ref == frq_base)
				{
					i = (n_coeffs * 3) * a + (3) * c;
					double *prjs_b = &prjs[i];
            		double *errs_b = &errs[i];
					
					n = find(mrgd, n_mrgd, mrgd[a].job.app_id, coeffs[c].pstate);

					if (n != -1)
					{
						application_t *m = &mrgd[n];

						if ((!opt_s || (opt_s)) && opt_c)
						{
							verbose(0, "%s;%lu;%lu;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf",
								mrgd[a].job.app_id, mrgd[a].signature.def_f, coeffs[c].pstate,
								m->signature.time    , prjs_b[c + 1], errs_b[c + 1],
								m->signature.DC_power, prjs_b[c + 2], errs_b[c + 2]);
						}
						else if ((!opt_s || opt_s))
						{
							col_time = (errs_b[c+1] < 6.0) ? "": STR_YLW;
							col_powe = (errs_b[c+2] < 6.0) ? "": STR_YLW;
							col_time = (errs_b[c+1] < 8.0) ? col_time: STR_RED;
           					col_powe = (errs_b[c+2] < 8.0) ? col_powe: STR_RED;

							tprintf("->||%lu|| | %0.2lf||%0.2lf||%s%0.2lf|| | %0.2lf||%0.2lf||%s%0.2lf",
								coeffs[c].pstate,
								m->signature.time    , prjs_b[c + 1], col_time, errs_b[c + 1],
								m->signature.DC_power, prjs_b[c + 2], col_powe, errs_b[c + 2]);
						}
					}
					else
					{
						if (opt_c)
						{
							verbose(0, "%s;%s;--;%lu;--;--;--;--;--;--",
								name_node, mrgd[a].job.app_id, coeffs[c].pstate);
						}
						else
						{
							tprintf("->||%lu|| | -||-||-|| | -||-||-", coeffs[c].pstate);
						}
					}
				}
			}
		}
	}

	// Medium error per coefficient
	if (opt_s) {
		verbose(0, LINE);
	}

	if (opt_s)
	{
		if (opt_c) {
			verbose(0, "Freq. From;Freq. To;T. Err;P. Err");
		} else {
			tprintf("medium error||@%u|| | -||-||T. Err|| | -||-||P. Err", frq_base);
		}
	}

	for (c = 0, i = 0; opt_s && c < n_coeffs; ++c, i += 4)
	{
		if (errs_med[i+3] > 0.0)
		{
			if (opt_c) {
				verbose(0, "%s;%u;%lu;%0.2lf;%0.2lf",
					name_node, frq_base, coeffs[c].pstate, errs_med[i+1], errs_med[i+2]);
			} else {
				col_time = (errs_med[i+1] < 6.0) ? "": STR_YLW;
	            col_powe = (errs_med[i+2] < 6.0) ? "": STR_YLW;
	            col_time = (errs_med[i+1] < 8.0) ? col_time: STR_RED;
	            col_powe = (errs_med[i+2] < 8.0) ? col_powe: STR_RED;

				tprintf("->||%lu|| | -||-||%s%0.2lf|| | -||-||%s%0.2lf",
					coeffs[c].pstate, col_time, errs_med[i+1], col_powe, errs_med[i+2]);
			}
		}
	}

	// General medium error
	if (opt_s && !opt_g)
	{
		if (opt_c) {
			verbose(0, "%s;%u;%u;%0.2lf;%0.2lf",
				name_node, frq_base, frq_base, errs_gen[1], errs_gen[2]);
		} else {
			col_time = (errs_gen[1] < 6.0) ? "": STR_YLW;
            col_powe = (errs_gen[2] < 6.0) ? "": STR_YLW;
            col_time = (errs_gen[1] < 8.0) ? col_time: STR_RED;
            col_powe = (errs_gen[2] < 8.0) ? col_powe: STR_RED;

			tprintf("general error||%u|| | -||-||%s%0.2lf|| | -||-||%s%0.2lf",
				frq_base, col_time, errs_gen[1], col_powe, errs_gen[2]);
		}
	}

	if (opt_g)
	{
		if (opt_c) {
			verbose(0, "%s;%u;%0.2lf;%0.2lf",
				name_node, frq_base, errs_gen[1], errs_gen[2]);
		} else
		{
			col_time = (errs_gen[1] < 6.0) ? "": STR_YLW;
			col_powe = (errs_gen[2] < 6.0) ? "": STR_YLW;
			col_time = (errs_gen[1] < 8.0) ? col_time: STR_RED;
			col_powe = (errs_gen[2] < 8.0) ? col_powe: STR_RED;

			tprintf("%s||%u|| | -||-||%s%0.2lf|| | -||-||%s%0.2lf",
				name_node, frq_base, col_time, errs_gen[1], col_powe, errs_gen[2]);
		}
	}
}

/*
 *
 * Read
 *
 */

void read_applications()
{
	application_t *appsn;
	application_t *appsl;
	int n_appsn = 0;
	int n_appsl = 0;

	//
	n_appsn = db_read_applications(&appsn, 1, 1000, name_node);

	if (n_appsn <= 0)
	{
		if (!opt_g) {
			error("no learning apps found for the node '%s'", name_node); //error
		}

		return;
	}

	
	if (opt_a) {
		n_appsl = db_read_applications(&appsl, 0, 1000, name_node);
	}

	//
	n_apps = n_appsn + n_appsl;
	apps = calloc(n_apps, sizeof(application_t));

	//
	memcpy(apps, appsn, n_appsn * sizeof(application_t));

	if (opt_a) {
		memcpy(&apps[n_appsn], appsl, n_appsl * sizeof(application_t));
	}
}

void read_coefficients()
{
	char *node = name_node;
	int island;

	//
	island = get_node_island(&conf, node);

	if (island == EAR_ERROR)
	{
		if (!opt_g)
		{
			error("no island found for node %s", node); //error
			return;
		}
	}

	// If file is custom
	if (opt_i)
	{
		n_coeffs = coeff_file_read(path_input, &coeffs);
	}

	// If default is selected
	else if (!opt_d)
	{
		sprintf(path_coeffs, "%s/island%d/coeffs.%s",
			conf.earlib.coefficients_pathname, island, node);
		//
		n_coeffs = coeff_file_read(path_coeffs, &coeffs);
	}

	// Don't worked, looking for defaults
	if (n_coeffs <= 0)
	{
		sprintf(buffer, "%s/island%d/coeffs.default",
 			conf.earlib.coefficients_pathname, island);
		//
		n_coeffs = coeff_file_read(buffer, &coeffs);
		
		if (n_coeffs <= 0)
		{
			if (!opt_g) {
				error("no default coefficients found"); //error
			}
		}
	}
}

/*
 *
 * Initialization
 *
 */

void usage(int argc, char *argv[])
{
	int c = 0;

	if (argc < 3)
	{
		verbose(0, "Usage: %s hostname frequency [OPTIONS...]\n", argv[0]);
		verbose(0, "  The hostname of the node where to test the coefficients quality.");
		verbose(0, "  The frequency is the nominal base frequency of that node.\n");
		verbose(0, "Options:");
		verbose(0, "\t-A\tAdds also the applications database.");
		verbose(0, "\t-C\tPrints the console output in CSV format.");
		verbose(0, "\t-D\tUses the default coefficients.");
		verbose(0, "\t-G\tShows one lined general summary.");
		verbose(0, "\t-H\tShows the header when general summary is enabled.");
		verbose(0, "\t-I <p>\tUse a custom coefficients file.");
		verbose(0, "\t-S\tShows the medium and opt_g errors.");
		exit(1);
	}

	frq_base = (unsigned long) atoi(argv[2]);
	strcpy(name_node, argv[1]);

	//
	while ((c = getopt (argc, argv, "ACDGHI:S")) != -1)
	{
		switch (c)
		{

			case 'A':
				opt_a = 1;
				break;
			case 'C':
				opt_c = 1;
				break;
			case 'D':
				opt_d = 1;
				break;
			case 'G':
				opt_g = 1;
				break;
			case 'H':
				opt_h = 1;
				break;
			case 'I':
				opt_i = 1;
				strcpy(path_input, optarg);
				break;
			case 'S':
				opt_s = 1;
				break;
			case '?':
				break;
			default:
				abort ();
		}
	}

	//

	if (opt_g) {
		opt_s = 0;
	}

	if (opt_i) {
		opt_d = 0;
	}
}

void init()
{
	// Initialization
	get_ear_conf_path(buffer);

	if (read_cluster_conf(buffer, &conf) != EAR_SUCCESS){
		error("while reading cluster configuration."); //error
		exit(1);
	}

	init_db_helper(&conf.database);
}

/*
 *
 * Main
 *
 */

int main(int argc, char *argv[])
{
	// Initialization
	usage(argc, argv);

	init();

	// Read
	read_coefficients();

	read_applications();

	// Work bench
	merge();

	compute();

	print();

	return 0;
}
