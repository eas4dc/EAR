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
#include <common/types/signature.h>
#include <common/types/projection.h>
#include <common/types/application.h>
#include <common/types/coefficient.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/database/db_helper.h>

int COLUMNS = 6;

static char buffer [SZ_PATH];
static char path_input[SZ_PATH];
static char *cofs_str[3] = { "ok", "def", "cus" };

/* */
static cluster_conf_t conf;

/* Merged applications */
static signature_t sign_mean;
static coefficient_t **cofs;
static application_t  *apps;
static int *cofs_s;
static int *cofs_n;
static int apps_n;

/* Frequency */
static ulong *f_dst;
static ulong  f_src;

/* Others */
static job_id step_id;
static job_id jobp_id;
static int opt_a;
static int opt_c;
static int opt_h;
static int opt_i;
static int opt_m;

/*
 *
 * Work bench
 *
 */

static void print_similars(application_t *app)
{
	application_t *apps;
    char proj_tim[COLUMNS][64];
    char proj_pow[COLUMNS][64];
	int napps, nsets;
	int icol, iapp, finished=0;

	//
	sprintf(buffer, "SELECT * from Applications WHERE (node_id = '%s') \
						AND (job_id,step_id) IN (SELECT id,step_id FROM Jobs WHERE policy = '%s' AND app_id = '%s' AND procs = %lu) \
						AND ((job_id != %lu) OR (step_id != %lu));",
					app->node_id,
						app->job.policy, app->job.app_id, app->job.procs,
						app->job.id, app->job.step_id);

	//
	napps = db_read_applications_query(&apps, buffer);
	
	while (!finished)
	{
		nsets = 0;
	
		for (icol = 0; icol < COLUMNS; ++icol)
		{
			sprintf(proj_tim[icol], "-");
			sprintf(proj_pow[icol], "-");

			for (iapp = 0; iapp < napps; ++iapp)
			{
				if (apps[iapp].job.def_f == f_dst[icol])
				{
					sprintf(proj_tim[icol], "%0.2lf", apps[iapp].signature.time); 				
					sprintf(proj_pow[icol], "%0.2lf", apps[iapp].signature.DC_power); 				
	
					apps[iapp].job.def_f = 1;
					iapp = napps;
					nsets += 1;
				}
			}	
		}

		finished = (nsets == 0);

		if (!finished) {
			tprintf("-||-||-|| | %s||%s||%s||%s||%s||%s|| | %s||%s||%s||%s||%s||%s",
				proj_tim[0], proj_tim[1], proj_tim[2], proj_tim[3], proj_tim[4], proj_tim[5],
				proj_pow[0], proj_pow[1], proj_pow[2], proj_pow[3], proj_pow[4], proj_pow[5]);
		}
	}

	free(apps);
}

static void mean()
{
	int n;

	for(n = 0; n < apps_n && opt_m; ++n)
	{
		sign_mean.CPI  += apps[n].signature.CPI;
		sign_mean.TPI  += apps[n].signature.TPI;
		sign_mean.time += apps[n].signature.time;
		sign_mean.DC_power += apps[n].signature.DC_power;
	}

	sign_mean.CPI  /= (double) apps_n;
	sign_mean.TPI  /= (double) apps_n;
	sign_mean.time /= (double) apps_n;
	sign_mean.DC_power /= (double) apps_n;
}

static void print()
{
	//double proj_cpi[COLUMNS];
	//double proj_tpi[COLUMNS];
	double proj_tim[COLUMNS];
	double proj_pow[COLUMNS];
	application_t *p_apps;
	coefficient_t *p_cofs;
	signature_t   *p_sign;
	int n, c, t1, t2, num;
	char *app_id;

	//
	app_id = apps[0].job.app_id;

	if ((num = strlen(app_id)) > 12) {
		app_id = &app_id[num-11];
	}

	// Headers
	if (opt_c && !opt_h)
	{
		verbose(0, "Node;Coe.;Freq.;T. Real;T. 1;T. 2;T. 3;T. 4;T. 5;P. Real;P. 1;P. 2;P. 3;P. 4;P. 5");
	}
	else if (!opt_c)
	{
		tprintf_init(fderr, STR_MODE_DEF, "12 3 11 11 11 11 11 11 11 11 11 11 11 11");

		if (!opt_h) {
			tprintf("%s||Coe|||T. Real||T. 1||T. 2||T. 3||T. 4||T. 5|||P. Real||P. 1||P. 2||P. 3||P. 4||P. 5",
				app_id);
			tprintf("--||---|||-------||----||----||----||----||----|||-------||----||----||----||----||----");
		}
	}

	if (opt_m) {
		apps_n = 1;
	}

	// For all nodes (or apps)
	for(n = 0; n < apps_n; ++n)
	{
		// Pointers
		num = cofs_n[n];
		p_cofs = cofs[n];
		p_apps = &apps[n];

		// Selecting signature (app or mean)
		if (opt_m) {
			p_sign = &sign_mean;
		} else {
			p_sign = &p_apps->signature;
		}

		// Real values
		proj_tim[0] = p_apps->signature.time;
		proj_pow[0] = p_apps->signature.DC_power;

		// Projections
		for (c = 0, t2 = 1; c < num; c++)
		{
			if (p_cofs[c].available && p_cofs[c].pstate_ref == f_src)
			{
				for (t1 = 1; t1 < COLUMNS; ++t1)
				{
					if (p_cofs[c].pstate == f_dst[t1])
					{
						//proj_cpi[t2] = project_cpi(p_sign, &p_cofs[c]);
						proj_tim[t2] = basic_project_time(p_sign, &p_cofs[c]);
						proj_pow[t2] = basic_project_power(p_sign, &p_cofs[c]);
						//printf("%d %0.2lf %lu\n", t2, proj_tim[t2], p_cofs[c].pstate);
						t2 += 1;
					}
				}
			}
		}

		if (opt_c)
		{
			verbose(0, "%s;%s;%lu;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf",
				p_apps->node_id, cofs_str[cofs_s[n]], p_apps->signature.def_f,
				proj_tim[0], proj_tim[1], proj_tim[2], proj_tim[3], proj_tim[4], proj_tim[5],
				proj_pow[0], proj_pow[1], proj_pow[2], proj_pow[3], proj_pow[4], proj_pow[5]);
		}
		else
		{
			tprintf("%s||%s|||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf|||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf",
 			p_apps->node_id, cofs_str[cofs_s[n]],
			proj_tim[0], proj_tim[1], proj_tim[2], proj_tim[3], proj_tim[4], proj_tim[5],
			proj_pow[0], proj_pow[1], proj_pow[2], proj_pow[3], proj_pow[4], proj_pow[5]);
		}

		if (opt_a) {
			print_similars(p_apps);
		}
	}

	if (opt_h) {
		return;
	}

	// Print legend
	verbose(0, "-------------------------");
	
	tprintf_init(fderr, STR_MODE_DEF, "5 12 12");
	tprintf("Idx||Freq. from||Freq. to");

	for (t1 = 1; t1 < COLUMNS; ++t1)
    {
		tprintf("%d||%lu||%lu", t1, f_src, f_dst[t1]);
	}
}

/*
 *
 * Initialization
 *
 */

static void read_applications()
{
	application_t *apps_aux;

	sprintf(buffer, "SELECT * FROM Applications WHERE job_id = %lu AND step_id = %lu",
			jobp_id, step_id);

	//
	apps_n = db_read_applications_query(&apps_aux, buffer);

	if (apps_n == 0) {
		verbose(0, "No apps found for the job_id '%lu'", jobp_id); //error
		exit(1);
	}
	
	//
	apps = calloc(apps_n, sizeof(application_t));
	memcpy(apps, apps_aux, apps_n * sizeof(application_t));
	free(apps_aux);

	//
	f_src = apps[0].signature.def_f;
}

void read_coefficients()
{
	coefficient_t *c;
	char *node;
	int island;
	int i, j;

	f_dst  = calloc(COLUMNS, sizeof(unsigned long));
	cofs   = calloc(apps_n, sizeof(coefficient_t *));
	cofs_n = calloc(apps_n, sizeof(int));
	cofs_s = calloc(apps_n, sizeof(int));

	for (i = 0; i < apps_n; ++i)
	{
		node = apps[i].node_id;
		island = get_node_island(&conf, node);

	//printf("%s %s %lu\n", apps[i].job.app_id, apps[i].node_id, apps[i].signature.time);

		if (island == EAR_ERROR) {
			verbose(0, "ERROR, no island found for node %s", node); //error
			exit(1);
		}

		//
		sprintf(buffer, "%s/island%d/coeffs.%s",
			conf.earlib.coefficients_pathname, island, apps[i].node_id);

		// Reading the coefficient
		if (opt_i)
		{
			cofs_s[i] = 2;
			cofs_n[i] = coeff_file_read(path_input, &cofs[i]);

			if (cofs_n[i] <= 0) {
				verbose(0, "no custom coefficients found"); //error
				exit(1);
			}
		} else {
			cofs_s[i] = 0;
			cofs_n[i] = coeff_file_read(buffer, &cofs[i]);

			if (cofs_n[i] <= 0) {
				//
				sprintf(buffer, "%s/island%d/coeffs.default",
						conf.earlib.coefficients_pathname, island);

				//
				cofs_s[i] = 1;
				cofs_n[i] = coeff_file_read(buffer, &cofs[i]);

				if (cofs_n[i] <= 0) {
					verbose(0, "no coefficients found for node %s", node);
					exit(1);
				}
			}
		}
	}


	// Getting coefficients format
	c = cofs[0];
	f_dst[0] = f_src;

	for(i = 0, j = 1; i < cofs_n[0] && j < COLUMNS; ++i)
	{
		if (c[i].available && c[i].pstate_ref == f_src && c[i].pstate != f_src)
		{
			f_dst[j] = c[i].pstate;
			j += 1;
		}
	}
}

void usage(int argc, char *argv[])
{
	int c;

	if (argc < 3)
	{
		verbose(0, "Usage: %s job.id step.id [OPTIONS]\n", argv[0]);
		verbose(0, "  The job.id of the job to project.");
		verbose(0, "  The step.id of the job to project.");
		verbose(0, "\nOptions:");
		verbose(0, "\t-A\tShows other jobs of the same application,");
		verbose(0, "\t\tnode, policy and number of processes.");
		verbose(0, "\t-C\tPrints the console output in CSV format.");
		verbose(0, "\t-H\tHides the header and the summary.");
		verbose(0, "\t-I <p>\tUse a custom coefficients file.");
		verbose(0, "\t-M <p>\tProject using the mean of all applications");
		verbose(0, "\t\tsignatures and custom coefficients file.");
		exit(1);
	}

	// Basic parametrs
	jobp_id = (unsigned long) atoi(argv[1]);
	step_id = (unsigned long) atoi(argv[2]);

	//
	while ((c = getopt (argc, argv, "ACHI:M:")) != -1)
	{
		switch (c)
		{
			case 'A':
				opt_a = 1;
				break;
			case 'C':
				opt_c = 1;
				break;
			case 'H':
				opt_h = 1;
				break;
			case 'I':
				opt_i = 1;
				strcpy(path_input, optarg);
				break;
			case 'M':
				opt_m = 1;
				opt_i = 1;
				strcpy(path_input, optarg);
				break;
			case '?':
				break;
			default:
				abort();
		}
	}
}

void init()
{
	// Initialization
	get_ear_conf_path(buffer);

	if (read_cluster_conf(buffer, &conf) != EAR_SUCCESS) {
		verbose(0, "Error reading cluster configuration."); //error
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
	//
	usage(argc, argv);

	//
	init();
	// Read
	read_applications();

	read_coefficients();

	// Work bench
	mean();

	print();

	return 0;
}
