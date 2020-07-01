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

#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// Buffers
extern char buffer[SZ_PATH];

//
extern char **_argv;
extern int    _argc;

//
extern int _sp;

spank_err_t spank_getenv (spank_t spank, const char *var, char *buf, int len)
{
	char *c;

	if (var == NULL || buf == NULL) {
		return ESPANK_ERROR;
	}
	if ((c = getenv(var)) == NULL) {
		return ESPANK_ERROR;
	}

	snprintf(buf, len, "%s", c);
	return ESPANK_SUCCESS;
}

spank_err_t spank_setenv (spank_t spank, const char *var, const char *val, int overwrite)
{
	if (var == NULL || val == NULL) {
		return ESPANK_ERROR;
	}
	if (setenv (var, val, overwrite) == -1) {
		return ESPANK_ERROR;
	}
	return ESPANK_SUCCESS;
}

spank_err_t spank_unsetenv (spank_t spank, const char *var)
{
	if (var == NULL) {
		return ESPANK_ERROR;
	}
	return (spank_err_t)(unsetenv(var) == 0);
}

spank_context_t spank_context (void)
{
	return (spank_context_t) _sp;
}

spank_err_t spank_get_item (spank_t spank, spank_item_t item, int *p)
{
	if (item == S_JOB_ID) {
		*p = atoi(getenv("SLURM_JOB_ID"));
	} else if (item == S_JOB_STEPID) {
		*p = atoi(getenv("SLURM_STEP_ID"));
	} else {
		*p = 0;
	}
	return ESPANK_SUCCESS;
}

char *slurm_hostlist_shift (hostlist_t host_list)
{
	static int count = 1;

	if (count == 1) {
		count = 0;
		return host_list;
	} else {
		count = 1;
		return NULL;
	}
}

hostlist_t slurm_hostlist_create (char *node_list)
{
	char *copy = malloc(strlen(node_list));
	return strcpy(copy, node_list);
}

spank_err_t spank_option_register_print(spank_t sp, struct spank_option *opt)
{
	int n = strlen(opt->usage);
	int i = 0;
	int o = 0;
	char c;

	printf("\t--%s\t\t", opt->name);
	if (strlen(opt->name) < 5) {
		printf("\t");
	}
	if (n < 64) {
		printf("%s\n", opt->usage);
		return ESPANK_SUCCESS;
	}
	while (i < n)
	{
		c = opt->usage[i];
		printf("%c", c);
		o = (o == 1) | (i != 0 && (i % 64) == 0);
		if (o && (c == ' ' || c == ',')) {
			printf("\n\t\t\t\t");
			o = 0;
		}
		i += 1;
	}
	printf("\n");

	return ESPANK_SUCCESS;
}

spank_err_t spank_option_register_call(int argc, char *argv[], spank_t sp, struct spank_option *opt)
{
	char *p;

	sprintf(buffer, "--%s=", opt->name);
	p = plug_acav_get(argc, argv, buffer);

	if (p == NULL) {
		return ESPANK_SUCCESS;
	} if (strlen(p) == 0) {
		return ESPANK_SUCCESS;
	}

	opt->cb(0, p, 0);

	return ESPANK_SUCCESS;
}

spank_err_t spank_option_register(spank_t sp, struct spank_option *opt)
{
	if (plug_context_is(sp, Context.error)) {
		return spank_option_register_print(sp, opt);
	} else if (plug_context_is(sp, Context.srun)) {
		return spank_option_register_call(_argc, _argv, sp, opt);
	}
	return ESPANK_SUCCESS;
}
