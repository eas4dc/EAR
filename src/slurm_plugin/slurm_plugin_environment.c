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

#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>

// Buffers
static char buffer[SZ_PATH];

//
static int verbosity = -1;

static int setenv_local(const char *var, const char *val, int ow)
{
	if (var == NULL || val == NULL) {
		return 0;
	}
    if (setenv (var, val, ow) == -1) {
        return 0;
    }
    return 1;
}

static int setenv_remote(spank_t sp, char *var, const char *val, int ow)
{
	if (var == NULL || val == NULL) {
		return 0;
	}
    return (spank_setenv (sp, var, val, ow) == ESPANK_SUCCESS);
}

static int unsetenv_local(char *var)
{
	if (var == NULL) {
		return 0;
	}
	return unsetenv(var) == 0;
}

static int unsetenv_remote(spank_t sp, char *var)
{
	if (var == NULL) {
        	return 0;
    	}
	return (spank_unsetenv(sp, var) == ESPANK_SUCCESS);
}

static int getenv_local(char *var, char *buf, int len)
{
	char *c;

	if (var == NULL || buf == NULL) {
		return 0;
	}
	if ((c = getenv(var)) == NULL) {
		return 0;
	}

	snprintf(buf, len, "%s", c);
	return 1;
}

static int getenv_remote(spank_t sp, char *var, char *buf, int len)
{
	spank_err_t serrno;

	if (var == NULL || buf == NULL) {
		return 0;
	}
	
	serrno = spank_getenv (sp, var, buf, len);

	if (serrno != ESPANK_SUCCESS) {
		return 0;
	}
	if (strlen(buf) <= 0) {
		return 0;
	}

	return 1;
}

static int isenv_local(char *var, char *val)
{
	char *env;

	if (var == NULL || val == NULL) {
		return 0;
	}

	if ((env = getenv(var)) != NULL) {
        return (strcmp(env, val) == 0);
    }

    return 0;
}

static int isenv_remote(spank_t sp, char *var, char *val)
{
    if (var == NULL || val == NULL) {
		return 0;
	}

    if (getenv_remote(sp, var, buffer, sizeof(buffer))) {
        return (strncmp(buffer, val, strlen(val)) == 0);
    }

    return 0;
}

static int exenv_local(char *var)
{
	char *env;

	if (var == NULL) {
		return 0;
	}

	env = getenv(var);
	return (env != NULL) && (strlen(env) > 0);
}

static int exenv_remote(spank_t sp, char *var)
{
	spank_err_t serrno;

	if (var == NULL) {
		return 0;
	}

	serrno = spank_getenv (sp, var, buffer, SZ_PATH);

	return (serrno == ESPANK_SUCCESS || serrno == ESPANK_NOSPACE) &&
		   (buffer != NULL) && (strlen(buffer)) > 0;
}

static int repenv_local(char *var_old, char *var_new)
{
	if (!getenv_local(var_old, buffer, SZ_PATH)) {
		return 0;
	}
	return setenv_local(var_new, buffer, 1);
}

static int repenv_remote(spank_t sp, char *var_old, char *var_new)
{
	if (!getenv_remote(sp, var_old, buffer, SZ_PATH)) {
		return 0;
	}
	return setenv_remote(sp, var_new, buffer, 1);
}

/*
 * Agnostic functions
 */

int unsetenv_agnostic(spank_t sp, char *var)
{
	if (plug_context_is(sp, Context.local)) {
		return unsetenv_local(var);
	} else {
		return unsetenv_remote(sp, var);
	}
}

int setenv_agnostic(spank_t sp, char *var, const char *val, int ow)
{
	if (sp == NULL_C || plug_context_is(sp, Context.local)) {
		return setenv_local(var, val, ow);
	} else {
		return setenv_remote(sp, var, val, ow);
	}
}

int getenv_agnostic(spank_t sp, char *var, char *buf, int len)
{
	if (plug_context_is(sp, Context.local)) {
		return getenv_local(var, buf, len);
	} else {
		return getenv_remote(sp, var, buf, len);
	}
}

int isenv_agnostic(spank_t sp, char *var, char *val)
{
	if (plug_context_is(sp, Context.local)) {
		return isenv_local(var, val);
	} else {
		return isenv_remote(sp, var, val);
	}
}

int repenv_agnostic(spank_t sp, char *var_old, char *var_new)
{
	unsetenv_agnostic(sp, var_new);

	if (plug_context_is(sp, Context.local)) {
		return repenv_local(var_old, var_new);
	} else {
		return repenv_remote(sp, var_old, var_new);
	}
}

int apenv_agnostic(char *dst, char *src, int dst_capacity)
{
	char *pointer;
	int new_cap;
	int len_src;
	int len_dst;

	if ((dst == NULL) || (src == NULL) || (strlen(src) == 0)) {
		return 0;
	}

	len_dst = strlen(dst);
	len_src = strlen(src);
	new_cap = len_dst + len_src + (len_dst > 0) + 1;

	if (new_cap > dst_capacity) {
		return 0;
	}

	if (len_dst > 0)
	{
		strcpy(buffer, dst);
		pointer = &dst[len_src];
		strcpy(&pointer[1], buffer);
		strcpy(dst, src);
		pointer[0] = ':';
	} else {
		strcpy(dst, src);
	}

	return 1;
}

int exenv_agnostic(spank_t sp, char *var)
{
	if (sp == NULL_C || plug_context_is(sp, Context.local)) {
		return exenv_local(var);
        } else {
		return exenv_remote(sp, var);
	}
}

int valenv_agnostic(spank_t sp, char *var, int *val)
{
	*val = 0;
	if (getenv_agnostic(sp, var, buffer, SZ_PATH)) {
		*val = atoi(buffer);
	}
	return *val;
}

void printenv_agnostic(spank_t sp, char *var)
{
	if (!getenv_agnostic(sp, var, buffer, SZ_PATH)) {
		sprintf(buffer, "NULL");
	}
	plug_verbose(sp, 2, "%s = '%s'", var, buffer);
}

/*
 * Component
 */

int plug_component_setenabled(spank_t sp, plug_component_t comp, int enabled)
{
	sprintf(buffer, "%d", enabled);
	setenv_agnostic(sp, comp, buffer, 1);
	return ESPANK_SUCCESS;
}

int plug_component_isenabled(spank_t sp, plug_component_t comp)
{
	int var;
	return valenv_agnostic(sp, comp, &var);
}

/*
 * Subject
 */

char *plug_host(spank_t sp)
{
	static char  host_b[SZ_NAME_LARGE];
	static char *host_p = NULL;

	if (host_p == NULL) {
		host_p = host_b;
		gethostname(host_p, SZ_NAME_MEDIUM);
	}

	return host_p;
}

char *plug_context_str(spank_t sp)
{
        if (plug_context_is(sp, Context.srun)) {
                return "srun";
        } else if (plug_context_is(sp, Context.sbatch)) {
                return "sbatch";
        } else if (plug_context_is(sp, Context.remote)) {
                return "remote";
        } else {
                return "erun";
        }
}

int plug_context_is(spank_t sp, plug_context_t ctxt)
{
	int cur = spank_context();

	if (ctxt == Context.local) {
		return cur == Context.srun || cur == Context.sbatch;
	}

	return cur == ctxt;
}

int plug_context_was(plug_serialization_t *sd, plug_context_t ctxt)
{
	return sd->subject.context_local == ctxt;
}

/*
 * Verbosity
 */

int plug_verbosity_test(spank_t sp, int level)
{	
	if (verbosity == -1)
	{
		if (getenv_agnostic(sp, Var.comp_verb.cmp, buffer, 8) == 1) {
			verbosity = atoi(buffer);
		} else {
			verbosity = 0;
		}
	}

	return verbosity >= level;
}

int plug_verbosity_silence(spank_t sp)
{
	verbosity = 0;
	return 0;
}

/*
 *
 */

char *plug_acav_get(int ac, char *av[], char *string)
{
	size_t len = strlen(string);
	int i;

	for (i = 0; i < ac; ++i)
	{
		if ((strlen(av[i]) > len) && (strncmp(string, av[i], len) == 0))
		{
			sprintf(buffer, "%s", &av[i][len]);
			return buffer;
		}
	}

	return NULL;
}
