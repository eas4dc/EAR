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

#include <common/colors.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_test.h>
#include <slurm_plugin/slurm_plugin_options.h>
#include <slurm_plugin/slurm_plugin_environment.h>

char *fake = "FAKE";
char buffer1[SZ_PATH];
char buffer2[SZ_PATH];

typedef struct test_s {
	char *policy;
	char *policy_th;
	char *perf_pen;
	char *eff_gain;
	char *frequency;
	char *p_state;
	char *learning;
	char *tag;
	char *path_usdb;
	char *path_trac;
	char *path_temp;
	char *verbose;
} test_t;

#define TF1	"2600000"
#define TF2	"2400000"
#define TF3	"2100000"
#define TF4	"2155555"
#define TP1	"1"
#define TP2	"3"
#define TP3	"5"
#define TA1	"memory-intensive"

test_t test1 = { .learning = TP1, .p_state = TP1,                     .frequency = TF1 };
test_t test2 = { .learning = TP2, .p_state = TP2,                     .frequency = TF2 };
test_t test3 = { .learning = TP3, .p_state = TP3,                     .frequency = TF3 };
test_t test4 = { .verbose = "3",  .policy = "MONITORING_ONLY",        .frequency = TF2 };
test_t test5 = { .verbose = "4",  .policy = "MIN_TIME_TO_SOLUTION",   .frequency = TF2 };
test_t test6 = { .verbose = "2",  .policy = "MIN_ENERGY_TO_SOLUTION", .frequency = TF3 };
test_t test7 = { .verbose = "1",  .policy = "MONITORING_ONLY",        .frequency = TF4 };
test_t test8 = { .policy_th = "0.2", .policy = "MIN_TIME_TO_SOLUTION"                  };
test_t test9 = { .policy_th = "0.3", .policy = "MIN_TIME_TO_SOLUTION"                  };
test_t test10 = { .path_trac = "/tmptr", .path_usdb = "/tmpus",  .path_temp = "/tmpte" };
test_t test11 = { .tag = "invalid"                                                     };
test_t test12 = { .tag = TA1                                                           };

static void fake_build(spank_t sp)
{
	setenv_agnostic(sp, Var.policy.ear, fake, 1);
	setenv_agnostic(sp, Var.policy_th.ear, fake, 1);
	setenv_agnostic(sp, Var.perf_pen.ear, fake, 1);
	setenv_agnostic(sp, Var.eff_gain.ear, fake, 1);
	setenv_agnostic(sp, Var.frequency.ear, fake, 1);
	setenv_agnostic(sp, Var.p_state.ear, fake, 1);
	setenv_agnostic(sp, Var.learning.ear, fake, 1);
	setenv_agnostic(sp, Var.tag.ear, fake, 1);
	setenv_agnostic(sp, Var.path_usdb.ear, fake, 1);
	setenv_agnostic(sp, Var.path_trac.ear, fake, 1);
	setenv_agnostic(sp, Var.path_temp.ear, fake, 1);
	setenv_agnostic(sp, Var.verbose.ear, fake, 1);
}

static int fake_test(spank_t sp)
{
	if (isenv_agnostic(sp, Var.policy.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.policy_th.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.perf_pen.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.eff_gain.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.frequency.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.p_state.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.learning.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.tag.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.path_usdb.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.path_trac.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.path_temp.ear, fake)) return ESPANK_ERROR;
	if (isenv_agnostic(sp, Var.verbose.ear, fake)) return ESPANK_ERROR;
	return ESPANK_SUCCESS;
}

static void option_build(spank_t sp, test_t *test)
{
	if (test->verbose != NULL) {
		_opt_ear_verbose(0, test->verbose, 0);
	}
	if (test->policy != NULL) {
 		_opt_ear_policy (0, test->policy, 0);
	}
	if (test->frequency != NULL) {
 		_opt_ear_frequency(0, test->frequency, 0);
	}
	if (test->path_usdb != NULL) {
		_opt_ear_user_db (0, test->path_usdb, 0);
	}
	if (test->path_trac != NULL) {
 		_opt_ear_traces (0, test->path_trac, 0);
	}
	if (test->learning != NULL) {
		_opt_ear_learning (0, test->learning, 0);
	}
	if (test->policy_th != NULL) {
		_opt_ear_threshold (0, test->policy_th, 0);
	}
	if (test->tag != NULL) {
		_opt_ear_tag (0, test->tag, 0);
	}
}

void plug_test_build(spank_t sp)
{
	plug_verbose(sp, 2, "function plug_test_build");

	int test;

	//
	test = plug_component_isenabled(sp, Component.test);

	// Simulating sbatch variables inheritance
	fake_build(sp);

	// Simulating option variables
	switch (test) {
		case 1: option_build(sp, &test1); break;
		case 2: option_build(sp, &test2); break;
		case 3: option_build(sp, &test3); break;
		case 4: option_build(sp, &test4); break;
		case 5: option_build(sp, &test5); break;
		case 6: option_build(sp, &test6); break;
		case 7: option_build(sp, &test7); break;
		case 8: option_build(sp, &test8); break;
		case 9: option_build(sp, &test9); break;
		case 10: option_build(sp, &test10); break;
		case 11: option_build(sp, &test11); break;
		case 12: option_build(sp, &test12); break;
		default: return;
	}
}

static int option_compare(spank_t sp, char *var, char *res)
{
	if (plug_verbosity_test(sp, 3))
	{
		getenv_agnostic(sp, var, buffer1, SZ_PATH);
		plug_verbose(sp, 3, "'%s','%s','%s'", var, buffer1, res);
	}
	return isenv_agnostic(sp, var, res);
}

static int option_result(spank_t sp, test_t *test)
{
	if (test->verbose != NULL) {
		if (!option_compare(sp, Var.verbose.ear, test->verbose)) return ESPANK_ERROR;
	}
	if (test->policy != NULL) {
		if (!option_compare(sp, Var.policy.ear, test->policy)) return ESPANK_ERROR;
	}
	if (test->frequency != NULL) {
		if (!option_compare(sp, Var.frequency.ear, test->frequency)) return ESPANK_ERROR;
	}
	if (test->path_usdb != NULL) {
		if (!option_compare(sp, Var.path_usdb.ear, test->path_usdb)) return ESPANK_ERROR;
	}
	if (test->path_trac != NULL) {
		if (!option_compare(sp, Var.path_trac.ear, test->path_trac)) return ESPANK_ERROR;
	}
	if (test->learning != NULL) {
		if (!option_compare(sp, Var.learning.ear, test->learning)) return ESPANK_ERROR;
	}
	if (test->policy_th != NULL) {
		if (!option_compare(sp, Var.policy_th.ear, test->policy_th)) return ESPANK_ERROR;
	}
	if (test->tag != NULL) {
		if (!option_compare(sp, Var.tag.ear, test->tag)) return ESPANK_ERROR;
	}

	return ESPANK_SUCCESS;
}

int option_result_custom(spank_t sp, test_t *test, int num)
{
	switch (num) {
		case 7:
			if (!isenv_agnostic(sp, Var.frequency.ear, test->frequency)) {
				return ESPANK_SUCCESS;
			}
		case 11:
			if (!getenv_agnostic(sp, Var.tag.ear, buffer1, SZ_PATH)) {
				return ESPANK_SUCCESS;
			}
			break;
	}
	return ESPANK_ERROR;
}

void plug_test_result(spank_t sp)
{
	plug_verbose(sp, 2, "function plug_test_result");

	int result;
	int test;

	//
	test = plug_component_isenabled(sp, Component.test);

	//
	if (fake_test(sp) != ESPANK_SUCCESS) {
		plug_verbose(sp, 0, "plugin fake test %sFAILED%s (%d)", COL_RED, COL_CLR, test);
		return;
	}

	//
	switch (test) {
		case 1:  result = option_result(sp, &test1); break;
		case 2:  result = option_result(sp, &test2); break;
		case 3:  result = option_result(sp, &test3); break;
		case 4:  result = option_result(sp, &test4); break;
		case 5:  result = option_result(sp, &test5); break;
		case 6:  result = option_result(sp, &test6); break;
		case 7:  result = option_result_custom(sp, &test7, 7); break;
		case 8:  result = option_result(sp, &test8); break;
		case 9:  result = option_result(sp, &test9); break;
		case 10: result = option_result(sp, &test10); break;
		case 11: result = option_result_custom(sp, &test11, 11); break;
		case 12: result = option_result(sp, &test12); break;
		default: return;
	}

	if (result != ESPANK_SUCCESS) {
		plug_verbose(sp, 0, "plugin option test %sFAILED%s (%d)", COL_RED, COL_CLR, test);
		return;
	}

	plug_verbose(sp, 0, "plugin option test %sSUCCESS%s (%d)", COL_GRE, COL_CLR, test);
}
