/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#if !SHOW_DEBUGS
#define NDEBUG
#endif
#include <assert.h>

#include <library/common/utils.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <common/environment_common.h>
#include <common/system/file.h>

state_t utils_create_plugin_path(char *result_path, char *install_path,
				 char *custom_path, int authorized_user)
{
	debug("install path: %s | custom path: %s | Authorised ? %d",
	      install_path, custom_path, authorized_user == AUTHORIZED);

	int ret;
	if (custom_path != NULL && (authorized_user == AUTHORIZED || USER_TEST)) {
		ret = sprintf(result_path, "%s/plugins", custom_path);
	} else {
		assert(install_path != NULL);
		if (install_path == NULL)
			return EAR_ERROR;
		ret = sprintf(result_path, "%s", install_path);
	}

	assert(ret > 0);
	state_t st = ret > 0 ? EAR_SUCCESS : EAR_ERROR;
	return st;
}

state_t utils_build_valid_plugin_path(char *result_path, size_t result_path_s,
				      const char *plug_endpt,
				      const char *plug_name,
				      settings_conf_t * sconf)
{
	if (!result_path || !plug_endpt || !plug_name || !sconf) {
		return EAR_ERROR;
	}

	/* Try to load the plug-in from a hacked path if the user is authorized. */
	int auth_user = sconf->user_type;
	char *hack_earl_install = NULL;
	if ((auth_user == AUTHORIZED) || USER_TEST) {
		hack_earl_install = ear_getenv(HACK_EARL_INSTALL_PATH);
	}

	/* Build and test whether the plug-in is in the hacked EARL */
	if (hack_earl_install) {
		snprintf(result_path, result_path_s - 1, "%s/plugins/%s/%s",
			 hack_earl_install, plug_endpt, plug_name);

		debug("Looking for %s", result_path);
		if (ear_file_exists(result_path))
			return EAR_SUCCESS;
	}

	/* Build and test whether the plug-in is in the official EARL */
	snprintf(result_path, result_path_s - 1, "%s/%s/%s",
		 sconf->installation.dir_plug, plug_endpt, plug_name);
	debug("Looking for %s", result_path);
	if (ear_file_exists(result_path))
		return EAR_SUCCESS;

	/* Test whether the plug-in is where plug_endpt literally says */
	debug("Looking for %s", plug_name)
	    if (ear_file_exists(plug_name)) {
		strncpy(result_path, plug_name, result_path_s - 1);
		return EAR_SUCCESS;
	}

	/* This code is reached if the base path was not hacked and the plug-in doesn't exist. */
	return EAR_ERROR;
}

int utils_get_sched_num_nodes()
{
	char *cn;
	cn = ear_getenv(SCHED_STEP_NUM_NODES);
	if (cn != NULL)
		return atoi(cn);
	else
		return 1;
}
