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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <common/string_enhanced.h>
#include <common/types/application.h>
#include <common/types/coefficient.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/database/db_helper.h>

//
char buffer[SZ_PATH];
char buffer_nodename[256];

//
static int opt_g;
static int opt_h;

//
static cluster_conf_t conf;
static application_t *apps;
static int n_apps;

//
static int diff_wapps;
static int diff_w060s;
static int diff_w120s;
static int diff_w400w; // A node oscillates bw 100-400W with no GPU
static int diff_w100w; // 
static int diff_w000g; // (3,400,000,000) * 2 * 8 * 8 = 435 GB/s max BW
static int diff_w450g; // (DDR4@3.4GHz OC) * (dual) * (8 bytes) * (8 channels)
static int diff_m060s;
static int diff_m120s;
static int diff_m400w; // A node oscillates bw 100-400W with no GPU
static int diff_m100w; //
static int diff_m000g; // (3,400,000,000) * 2 * 8 * 8 = 435 GB/s max BW
static int diff_m450g; // (DDR4@3.4GHz OC) * (dual) * (8 bytes) * (8 channels)
static ulong max_freq;
static ulong min_freq;

//
double max_pow = MAX_ERROR_POWER;
double min_pow = 040.0;
double top_pow = MIN_SIG_POWER;
double cei_pow = MAX_SIG_POWER;
double max_tim = 180.0;
double min_tim = 030.0;

//
double wtim;
double wpow;
double wgbs;
double mtim;
double mpow;
double mgbs;
char *name;
ulong freq;

/*
 *
 *
 *
 */

static void print_header()
{
	// If not general the header is printed
	if (!opt_g)
	{
		tprintf_init(fdout, STR_MODE_COL, "10 10 10 10 100");

		tprintf("Nodename|||Time||Power||Bandwidth|||Application");
		tprintf("--------|||----||-----||---------|||-----------");

		return;
	}

	// If general
	tprintf_init(fdout, STR_MODE_COL, "10 7 8 10 10 13 13 13 12 12 12");

	// If header
	if (opt_h) {
		tprintf("Nodename|||#Apps||#Execs|||F. Max||F. Min|||Time warns.||Pow. warns.||GBS. warns.|||Time mean||Pow. mean||GBS. mean.");
		tprintf("--------|||-----||------|||------||------|||-----------||-----------||-----------|||---------||---------||----------");
	}
}

static void print_individual()
{
	char *col_wtim;
	char *col_wpow;
	char *col_wgbs;
	int warn_wtim;
	int warn_wpow;
	int warn_wgbs;
	int aler_wtim;
	int aler_wpow;
	int aler_wgbs;
	//ulong freq;
	//char *name;

	if (opt_g) {
		return;
	}
	
	// Warning
	warn_wpow = (wpow > top_pow) | (wpow < cei_pow);

	// Warning color 
	col_wpow = (warn_wpow == 0) ? "": STR_YLW;

	// Alert
	aler_wtim = (wtim > max_tim) | (wtim < min_tim);
	aler_wpow = (wpow > max_pow) | (wpow < min_pow);
	aler_wgbs = (wgbs > 450.0)   | (wgbs < 000.0);

	// Alert color
	col_wtim = (aler_wtim == 0) ?       "": STR_RED;
	col_wpow = (aler_wpow == 0) ? col_wpow: STR_RED;
	col_wgbs = (aler_wgbs == 0) ?       "": STR_RED;
    
	if (warn_wpow || aler_wtim || aler_wpow || aler_wgbs) {
		tprintf("%s|||%s%0.2lf||%s%0.2lf||%s%0.2lf|||%s",
			buffer_nodename, col_wtim, wtim, col_wpow, wpow, col_wgbs, wgbs, name);
	}
}

static void print_summary()
{
	char *col_mtim;
	char *col_mgbs;
	char *col_mpow;
	char *col_wtim;
	char *col_wgbs;
	char *col_wpow;
	int warn_wtim;
	int warn_wgbs;
	int warn_wpow;
	int warn_mtim;
	int warn_mgbs;
	int warn_mpow;
	
	if (!opt_g) {
		return;
	}

	if (n_apps == 0 && opt_h) {
		return;
	}

	if (n_apps == 0)
	{
		tprintf("%s|||-||-|||-||-|||-||-||-|||-||-||-",
            buffer_nodename);
		return;
	}

	//
	warn_wgbs = diff_w450g + diff_w000g;
	warn_wpow = diff_w400w + diff_w100w;
	warn_wtim = diff_w120s + diff_w060s;
	warn_mgbs = diff_m450g + diff_m000g;
	warn_mpow = diff_m400w + diff_m100w;
	warn_mtim = diff_m120s + diff_m060s;
	col_wtim = (warn_wtim == 0) ? "": STR_RED;
	col_wpow = (warn_wpow == 0) ? "": STR_RED;
	col_wgbs = (warn_wgbs == 0) ? "": STR_RED;
	col_mtim = (warn_mtim == 0) ? "": STR_RED;
	col_mpow = (warn_mpow == 0) ? "": STR_RED;
	col_mgbs = (warn_mgbs == 0) ? "": STR_RED;

	// Table
	tprintf("%s|||%d||%d|||%lu||%lu|||%s%d||%s%d||%s%d|||%s%0.2f||%s%0.2f||%s%0.2f",
			buffer_nodename, diff_wapps, n_apps,
			max_freq, min_freq,
			col_wtim, warn_wtim,
			col_wpow, warn_wpow,
			col_wgbs, warn_wgbs,
			col_mtim, mtim,
			col_mpow, mpow,
			col_mgbs, mgbs
	);
}

static void analyze()
{
	int found_name;
	int found_freq;
	int a, i;

	if (n_apps == 0) {
		return;
	}

	//
	max_freq = apps[0].job.def_f;
	min_freq = apps[0].job.def_f;

	// Finding different apps
	for (a = 0; a < n_apps; a++)
	{
		name = apps[a].job.app_id;
		freq = apps[a].job.def_f;

		// Saved for warnings
		wtim = apps[a].signature.time;
		wpow = apps[a].signature.DC_power;
		wgbs = apps[a].signature.GBS;

		// Means
		mtim += wtim;
		mpow += wpow;
		mgbs += wgbs;

		found_name = 0;
		found_freq = 0;

		for (i = 0; i < a; i++)
		{
			found_name = found_name | (strcmp(name, apps[i].job.app_id) == 0);
			found_freq = found_freq | (freq == apps[i].job.def_f);
		}

		diff_wapps += !found_name;
		diff_w060s += (wtim < min_tim);
		diff_w120s += (wtim > max_tim);
		diff_w100w += (wpow < min_pow);
		diff_w400w += (wpow > max_pow);
		diff_w450g += (wgbs > 450.0);
		diff_w000g += (wgbs < 000.0);
		min_freq   = min_freq * (freq >= min_freq) + freq * (freq < min_freq);
		max_freq   = max_freq * (freq <= max_freq) + freq * (freq > max_freq);
		
		print_individual();
	}

	mtim /= (double) n_apps;
	mpow /= (double) n_apps;
	mgbs /= (double) n_apps;
	diff_m060s += (mtim < 040.0);
	diff_m120s += (mtim > 180.0);
	diff_m100w += (mpow < min_pow);
	diff_m400w += (mpow > max_pow);
	diff_m450g += (mgbs > 450.0);
	diff_m000g += (mgbs < 000.0);
}

/*
 *
 *
 *
 */

static void read_applications()
{
	init_db_helper(&conf.database);

	application_t *apps_aux;

	//
	n_apps = db_read_applications(&apps_aux, 1, 1000, buffer_nodename);

	if (n_apps <= 0) {
		n_apps = 0;
		return;
	}

	//
	apps = calloc(n_apps, sizeof(application_t));
	memcpy(apps, apps_aux, n_apps * sizeof(application_t));

	//
	free(apps_aux);
}

/*
 *
 *
 *
 */

static void usage(int argc, char *argv[])
{
	int c;

	if (argc < 2)
	{
		verbose(0, "Usage: %s node.name [OPTIONS...]\n", argv[0]);
		verbose(0, "  node.name is the name of the node to analyze");
		verbose(0, "\nOptions:");
		verbose(0, "\t-G \tShows one lined general summary");
		verbose(0, "\t-H \tShows the header in one lined summary mode.");

		exit(1);
	}

	// Basic parametrs
	strcpy(buffer_nodename, argv[1]);

	// Flags
	while ((c = getopt (argc, argv, "GH")) != -1)
	{
		switch (c)
		{
			case 'G':
				opt_g = 1;
				break;
			case 'H':
                opt_h = 1;
                break;
			case '?':
				break;
			default:
				abort();
		}
	}
}

static void init()
{
	my_node_conf_t *conf_node;

	if (get_ear_conf_path(buffer) == EAR_ERROR) {
		error("while getting ear.conf path");
		exit(1);
	}
	verbose(0, "reading '%s' configuration file", buffer);
	if (state_fail(read_cluster_conf(buffer, &conf))) {
		error("while reading ear.conf path file");
		exit(1);
	}
	if ((conf_node = get_my_node_conf(&conf, buffer_nodename)) == NULL) {
		error("while reading node configuration, using default configuration");
		goto print;
	}
	
	max_pow = conf_node->max_error_power;
	min_pow = conf_node->min_sig_power;
	top_pow = conf_node->max_sig_power;
	cei_pow = conf_node->min_sig_power;

	print:
	verbose(0, "--------------------------------------------------------");
	verbose(0, "alert max power: %0.2lf", max_pow);
	verbose(0, "alert min power: %0.2lf", min_pow);
	verbose(0, "warning min power: %0.2lf", top_pow);
	verbose(0, "warning min power: %0.2lf", cei_pow);
	verbose(0, "--------------------------------------------------------");
}

int main(int argc, char *argv[])
{
	usage(argc, argv);

	init();

	read_applications();

	print_header();

	analyze();

	print_summary();

    return 0;
}
