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

#include <common/system/folder.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_options.h>

#define SRUN_OPTIONS 8

static char buffer[SZ_PATH];
static char opt_mpi[SZ_PATH];
static char opt_pol[SZ_PATH];
static char opt_opt[SZ_PATH];

struct spank_option spank_options_manual[SRUN_OPTIONS] =
{
	{ "ear", "on|off", "Enables/disables Energy Aware Runtime Library",
	  1, 0, (spank_opt_cb_f) _opt_ear
	},
	{ 
	  "ear-policy", "type", opt_pol,
	  1, 0, (spank_opt_cb_f) _opt_ear_policy
	},
	{ "ear-cpufreq", "frequency", "Specifies the start frequency to be used by EAR policy (in KHz)",
	  1, 0, (spank_opt_cb_f) _opt_ear_frequency
	},
	{ "ear-policy-th", "value", "Specifies the threshold to be used by EAR policy (max 2 decimals)" \
	  " {value=[0..1]}",
	  1, 0, (spank_opt_cb_f) _opt_ear_threshold
	},
	{ "ear-user-db", "file",
	  "Specifies the file to save the user applications metrics summary " \
	  "'file.nodename.csv' file will be created per node. If not defined, these files won't be generated.",
	  1, 0, (spank_opt_cb_f) _opt_ear_user_db
	},
	{ "ear-verbose", "value", "Specifies the level of the verbosity" \
	  "{value=[0..1]}; default is 0",
	  1, 0, (spank_opt_cb_f) _opt_ear_verbose
	},
	{ "ear-learning", "value",
	  "Enables the learning phase for a given P_STATE {value=[1..n]}",
	  1, 0, (spank_opt_cb_f) _opt_ear_learning
	},
	{ "ear-tag", "tag", "Sets an energy tag (max 32 chars)",
	  1, 0, (spank_opt_cb_f) _opt_ear_tag
	},
	// Hidden options
#if 0
	{ "ear-mpi-dist", "dist", opt_mpi,
	  1, 0, (spank_opt_cb_f) _opt_ear_mpi_dist
	},
	{ "ear-traces", "path", "Saves application traces with metrics and internal details",
	  1, 0, (spank_opt_cb_f) _opt_ear_traces
	}
#endif
};

static int _opt_register_mpi(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function _opt_register_mpi");

	folder_t folder;
	char *file;
	state_t s;
	char *pop;
	char *pav;
	int cop;

	// Filing a default option string
	cop = sprintf(opt_opt, "default,");
	pop = &opt_opt[cop];

	if ((pav = plug_acav_get(ac, av, "prefix=")) == NULL) {
		return ESPANK_SUCCESS;
	}

	sprintf(buffer, "%s/lib/", pav);

	// Initilizing folder scanning
	s = folder_open(&folder, buffer);
	
	if (state_fail(s)) {
		return ESPANK_ERROR;
	}

	while ((file = folder_getnext(&folder, "libear.", ".so")))
	{
		cop = sprintf(pop, "%s,", file);
		pop = &pop[cop];
	}

	// Cleaning the last comma
	pop = &pop[-1];
	*pop = '\0';

	// Filling the option string with distribution options
	xsprintf(opt_mpi, "Selects the MPI distribution for compatibility of your application {dist=%s}", opt_opt);

	return ESPANK_SUCCESS;
}

static int _opt_register_pol(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function _opt_register_pol");

	folder_t folder;
	char *file;
	state_t s;
	char *pop;
	char *pav;
	int cop;

	// Filing a default option string
	cop = sprintf(opt_opt, "default,");
	pop = &opt_opt[cop];

	if ((pav = plug_acav_get(ac, av, "prefix=")) == NULL) {

		return ESPANK_SUCCESS;
	}

	sprintf(buffer, "%s/lib/plugins/policies/", pav);

	// Initilizing folder scanning
	s = folder_open(&folder, buffer);

	if (state_fail(s)) {
		return ESPANK_ERROR;
	}

	while ((file = folder_getnext(&folder, NULL, ".so")))
	{
		cop = sprintf(pop, "%s,", file);
		pop = &pop[cop];
	}

	// Cleaning the last comma
	pop = &pop[-1];
	*pop = '\0';

	// Filling the option string with distribution options
	xsprintf(opt_pol, "Selects an energy policy for EAR {type=%s}", opt_opt);

	return ESPANK_SUCCESS;
}

int _opt_register(spank_t sp, int ac, char **av)
{
	spank_err_t s;
	int length;
	int i;

	//
	_opt_register_mpi(sp, ac, av);
	_opt_register_pol(sp, ac, av);
	
	//
	length = SRUN_OPTIONS;

	for (i = 0; i < length; ++i)
	{
		if ((s = spank_option_register(sp, &spank_options_manual[i])) != ESPANK_SUCCESS)
		{
			plug_verbose(NULL_C, 2, "unable to register SPANK option %s",
						 spank_options_manual[i].name);
			return s;
		}
	}

	return ESPANK_SUCCESS;
}

/*
 *
 *
 *
 */

int _opt_ear (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear");

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}

		strncpy(buffer, optarg, 8);
		strtoup(buffer);

		if (strcmp(buffer, "ON") == 0) {
			setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
		} else if (strcmp(buffer, "OFF") == 0) {
			setenv_agnostic(NULL_C, Var.comp_libr.cmp, "0", 1);
		} else {
			plug_verbose(NULL_C, 2, "Invalid enabling value '%s'", buffer);
			return ESPANK_BAD_ARG;
		}
	}

	return ESPANK_SUCCESS;
}

int _opt_ear_learning (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear_learning");

	int ioptarg;

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}
		if ((ioptarg = atoi(optarg)) < 0) {
			return ESPANK_BAD_ARG;
		}

		xsnprintf(buffer, 4, "%d", ioptarg);
		setenv_agnostic(NULL_C, Var.learning.loc, buffer, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return ESPANK_SUCCESS;
}

int _opt_ear_policy (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear_policy");

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}

		strncpy(buffer, optarg, 32);
		//strtoup(buffer);

		/*if (policy_name_to_id(buffer) < 0) {
			plug_verbose(NULL, 2, "Invalid policy '%s'", buffer);
			return ESPANK_STOP;
		}*/
        if (strlen(buffer) < 1) {
			plug_verbose(NULL_C, 2, "Invalid policy '%s'", buffer);
			return ESPANK_STOP;
        }

		setenv_agnostic(NULL_C, Var.policy.loc, buffer, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return ESPANK_SUCCESS;
}

int _opt_ear_frequency (int val, const char *optarg, int remote)
{
    plug_verbose(NULL_C, 2, "function _opt_ear_threshold");

    ulong loptarg;

    if (!remote)
    {
        if (optarg == NULL) {
            return ESPANK_BAD_ARG;
        }
		
		loptarg = (ulong) atol(optarg);
		snprintf(buffer, 16, "%lu", loptarg);
		setenv_agnostic(NULL_C, Var.frequency.loc, buffer, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
    }

    return ESPANK_SUCCESS;
}

int _opt_ear_threshold (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear_threshold");

	double foptarg = -1;

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}
		if ((foptarg = atof(optarg)) < 0.0 || foptarg > 1.0) {
			return ESPANK_BAD_ARG;
		}

		snprintf(buffer, 8, "%0.2f", foptarg);
		setenv_agnostic(NULL_C, Var.policy_th.loc, buffer, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return ESPANK_SUCCESS;
}

int _opt_ear_user_db (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear_user_db");

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}

		setenv_agnostic(NULL_C, Var.path_usdb.loc, optarg, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return ESPANK_SUCCESS;
}

int _opt_ear_verbose (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear_verbose");

	int ioptarg;

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}

		ioptarg = atoi(optarg);
		if (ioptarg < 0) ioptarg = 0;
		if (ioptarg > 1) ioptarg = 1;
		snprintf(buffer, 4, "%d", ioptarg);

		setenv_agnostic(NULL_C, Var.verbose.loc, buffer, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return ESPANK_SUCCESS;
}

int _opt_ear_tag(int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_tag");

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}

		setenv_agnostic(NULL_C, Var.tag.loc, optarg, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}
	return ESPANK_SUCCESS;
}

/*
 *
 * Hidden options
 *
 */

#if 0
int _opt_ear_mpi_dist(int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_mpi_dist");

	if (!remote)
	{
		if (optarg == NULL) {
			return (ESPANK_BAD_ARG);
		}

		if (strcmp(optarg, "default") != 0) {
			setenv_agnostic(NULL_C, Var.version.loc, optarg, 1);
		}
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return (ESPANK_SUCCESS);
}

int _opt_ear_traces (int val, const char *optarg, int remote)
{
	plug_verbose(NULL_C, 2, "function _opt_ear_traces");

	if (!remote)
	{
		if (optarg == NULL) {
			return ESPANK_BAD_ARG;
		}

		setenv_agnostic(NULL_C, Var.path_trac.loc, optarg, 1);
		setenv_agnostic(NULL_C, Var.comp_libr.cmp, "1", 1);
	}

	return ESPANK_SUCCESS;
}
#endif
