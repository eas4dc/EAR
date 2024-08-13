/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


//#define SHOW_DEBUGS 1


#define _GNU_SOURCE
#include <sched.h>
#include <common/includes.h>
#include <common/config.h>
#include <common/system/symplug.h>
#include <common/sizes.h>
#include <library/policies/policy_ctx.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/utils.h>
#include <library/metrics/metrics.h>

extern const int     polsyms_n;
extern const char     *polsyms_nam[];
state_t policy_gpu_load(settings_conf_t *app_settings, polsym_t *psyms)
{
	if (masters_info.my_master_rank >= 0)
	{
#if USE_GPUS
    char *hack_gpu_policy_plugin = (app_settings->user_type == AUTHORIZED) ? ear_getenv(HACK_GPU_POWER_POLICY) : NULL;

		char gpupolicy_objc[SZ_PATH];

    // If hack, then the energy plugin is a path, otherwise is a shared object file.
		if (hack_gpu_policy_plugin)
		{
			debug("GPU policy hacked via path: %s", hack_gpu_policy_plugin);
			strncpy(gpupolicy_objc, hack_gpu_policy_plugin, sizeof(gpupolicy_objc) - 1);
		} else if (app_settings->policy_name)
		{
			char gpupolicy_name[SZ_FILENAME];
#ifdef GPU_OPT
			verbose_master(2, "GPU optimization ON. Policy base name: %s", app_settings->policy_name);
			xsnprintf(gpupolicy_name, sizeof(gpupolicy_name), "gpu_%s.so", app_settings->policy_name);
#else
			xsnprintf(gpupolicy_name, sizeof(gpupolicy_name), "gpu_monitoring.so");
#endif

			char my_plug_path[SZ_PATH_INCOMPLETE];
			if (state_fail(utils_create_plugin_path(my_plug_path, app_settings->installation.dir_plug,
																							ear_getenv(HACK_EARL_INSTALL_PATH), app_settings->user_type)))
			{
					return_msg(EAR_ERROR, "Plugins path can not be built.");
			}

			xsnprintf(gpupolicy_objc, sizeof(gpupolicy_objc), "%s/policies/%s", my_plug_path, gpupolicy_name);
		} else
		{
        return_msg(EAR_NOT_FOUND, "There is no gpu policy plugin configured.");
		}

		verbose_master(2, "Loading %s GPU policy...", gpupolicy_objc);
		if (state_fail(symplug_open(gpupolicy_objc, (void **) psyms, polsyms_nam, polsyms_n)))
		{
			verbose_master(2, "Error loading file %s (%s)", gpupolicy_objc, state_msg);
			return EAR_ERROR;
		}
#endif
	}
	return EAR_SUCCESS;
}
