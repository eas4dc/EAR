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

state_t utils_create_plugin_path(char *result_path, char *install_path, char *custom_path, int authorized_user)
{
	debug("install path: %s | custom path: %s | Authorised ? %d",
			install_path, custom_path, authorized_user == AUTHORIZED);

	int ret;
	if (custom_path != NULL && (authorized_user == AUTHORIZED || USER_TEST))
	{
		ret = sprintf(result_path, "%s/plugins", custom_path);
	} else {
		assert(install_path != NULL);
		if (install_path == NULL) return EAR_ERROR;
		ret = sprintf(result_path, "%s", install_path);
	}

	assert(ret > 0);
	state_t st = ret > 0 ? EAR_SUCCESS : EAR_ERROR;
	return st;
}


int utils_get_sched_num_nodes()
{
    char *cn;
    cn = ear_getenv(SCHED_STEP_NUM_NODES);
    if (cn != NULL) return atoi(cn);
    else return 1;
}
