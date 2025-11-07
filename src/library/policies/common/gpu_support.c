/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <common/config.h>
#include <common/includes.h>
#include <common/sizes.h>
#include <common/system/file.h>
#include <common/system/symplug.h>
#include <library/common/externs.h>
#include <library/common/utils.h>
#include <library/common/verbose_lib.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy_ctx.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>

#define MASTER_LOCK_FILENAME ".gpu_optimize"

/* Negative means 'not configured', i.e., env var not defined. */
static int gpu_opt_enabled = -1;
static int fd_gpu_optimize = -1;
static char master_lock_file[SZ_PATH];

extern const int polsyms_n;
extern const char *polsyms_nam[];

extern uint gpu_optimize;

static void optimization_config(settings_conf_t *app_settings);

state_t policy_gpu_load(settings_conf_t *app_settings, polsym_t *psyms)
{
    /* Only a master process can load a GPU policy */
    if (masters_info.my_master_rank < 0) {
        return EAR_SUCCESS;
    }

#if USE_GPUS
    /* Try to load the hacked GPU policy plug-in */
    int read_env                 = (app_settings->user_type == AUTHORIZED || USER_TEST);
    char *hack_gpu_policy_plugin = (read_env) ? ear_getenv(HACK_GPU_POWER_POLICY) : NULL;

    char gpupolicy_objc[SZ_PATH];

    if (hack_gpu_policy_plugin) {
        if (state_ok(utils_build_valid_plugin_path(gpupolicy_objc, sizeof(gpupolicy_objc), "policies",
                                                   hack_gpu_policy_plugin, app_settings))) {
            goto load;
        }
    }

    /* If the above didn't apply, try to load the configured GPU policy plug-in */

    char gpupolicy_name[SZ_FILENAME];
    char *cgpu_optimize = ear_getenv(FLAG_GPU_ENABLE_OPT);
    if (cgpu_optimize == NULL)
        cgpu_optimize = ear_getenv(GPU_ENABLE_OPT);
    if (cgpu_optimize != NULL)
        gpu_optimize = atoi(cgpu_optimize);

    verbose_info2_master("GPU optimization %s. GPU policy plug-in base name: %s",
                         (gpu_optimize) ? "enabled" : "disabled",
                         (gpu_optimize) ? app_settings->policy_name : "monitoring");

    /*
     * If gpu_optimize is disabled, gpu_monitoring.so will be loaded.
     * Otherwise, 'gpu_'<cpu_policy_name>.so will be loaded.
     */

    snprintf(gpupolicy_name, sizeof(gpupolicy_name) - 1, "gpu_%s.so",
             (gpu_optimize) ? app_settings->policy_name : "monitoring");

    if (state_fail(utils_build_valid_plugin_path(gpupolicy_objc, sizeof(gpupolicy_objc), "policies", gpupolicy_name,
                                                 app_settings))) {
        verbose_warning_master("GPU plug-in %s doesn't exist.", gpupolicy_name);
        return EAR_ERROR;
    }

load:
    verbose_info2_master("[%d] Loading GPU policy plug-in: %s", getpid(), gpupolicy_objc);
    if (state_fail(symplug_open(gpupolicy_objc, (void **) psyms, polsyms_nam, polsyms_n))) {
        verbose_error_master("Loading file %s (%s)", gpupolicy_objc, state_msg);
        return EAR_ERROR;
    }
    // Decide whether the process has the optimization policy enabled/disabled.
    optimization_config(app_settings);
#endif
    return EAR_SUCCESS;
}

uint policy_gpu_opt_enabled()
{
    /* gpu_opt_enabled positive: I'm the 'master optimizer'
     * gpu_opt_enabled zero: I'm not the 'master optimizer'
     * gpu_opt_enabled negative: This feature was not requested.
     */
    if (gpu_opt_enabled > 0) {
        // I'm already the 'master optimizer'
        return gpu_opt_enabled;
    } else if (gpu_opt_enabled == 0) {
        // Try to get the lock and become the 'master optimizer'
        if (!ear_file_trylock(fd_gpu_optimize)) {
            verbose_info("[%d] GPU master optimizer changed!", getpid());
            gpu_opt_enabled = 1;
        }
        return gpu_opt_enabled;
    } else {
        return 1;
    }
}

void policy_gpu_unlock_opt()
{
    /* We can't use non-zero evaluation here,
     * since -1 means 'feature not enabled'. */
    if (gpu_opt_enabled > 0) {
        if (ear_file_unlock(fd_gpu_optimize) < 0) {
            verbose_error("Unlocking GPU optimize lock (%d): %s", errno, strerror(errno));
        }
        ear_file_lock_clean(fd_gpu_optimize, master_lock_file);
        gpu_opt_enabled = 0;
    }
}

static void optimization_config(settings_conf_t *app_settings)
{
    // In case FLAG_GPU_MASTER_OPTIMIZE is defined to 1, try to get a lock. If so, GPU optimization is enabled.
    // This is implemented to avoid multiprocess applications where all processes
    // are masters an all of them see all GPU devices.

    char *gpu_master_opt_str = ear_getenv(FLAG_GPU_MASTER_OPTIMIZE);
    if (!gpu_master_opt_str) {
        debug("GPU Master optimize not defined.");
        return;
    }

    verbose_info("GPU Master optimize defined: %s", gpu_master_opt_str);

    long gpu_master_opt = strtol(gpu_master_opt_str, NULL, 10);
    if (gpu_master_opt) {

        /* Build the job tmp dir path. */
        char *tmp = get_ear_tmp();
        char job_tmp_path[SZ_PATH_INCOMPLETE];
        snprintf(job_tmp_path, sizeof(job_tmp_path) - 1, "%s/%u", tmp, app_settings->id);

        /* Check whether it exists. */
        if (!ear_file_is_directory(job_tmp_path)) {
            verbose_warning("%s does not exist. GPU optimization lock can't be created.", job_tmp_path);
            return;
        }

        /* Create the complete lock path */
        snprintf(master_lock_file, sizeof(master_lock_file) - 1, "%s/%s", job_tmp_path, MASTER_LOCK_FILENAME);

        if ((fd_gpu_optimize = ear_file_lock_create(master_lock_file)) < 0) {
            verbose_error("Creating %s lock file: %s", master_lock_file, strerror(errno));
            return;
        }

        /* Try to acquire the lock */
        if (ear_file_trylock(fd_gpu_optimize) < 0) {
            gpu_opt_enabled = 0;
        } else {
            gpu_opt_enabled = 1;
        }

        verbose_info2("[%d] GPU optimization %s", getpid(), (gpu_opt_enabled) ? "enabled" : "disabled");
    }
}

#if 0
/*
 * This function is disabled since it is not used. But the idea to replace
 * the shared object name from a full path may be useful in a future.
 */
static void set_gpu_monitoring_plugin(char **path, size_t path_size)
{
	uint elem_cnt = 1;
	debug("Original path: %s", *path);
	char **policy_path = strtoa(*path, '/', NULL, &elem_cnt);

	// If we are already loading gpu_monitoring.so, we can skip below code
	if (strcmp(policy_path[elem_cnt - 1], "gpu_monitoring.so")) {

		char *aux_path = (char *)malloc(path_size);
		memset(aux_path, 0, path_size);
		snprintf(aux_path, path_size, "/");

		// Concat all path entries, except the last one, i.e., the policy plugin
		for (int i = 0; i < elem_cnt - 1; i++) {
			char path_part[SZ_PATH_INCOMPLETE];
			snprintf(path_part, sizeof(path_part), "%s/",
				 policy_path[i]);

			debug("Concatenating %s to %s", path_part, aux_path);
			strncat(aux_path, path_part,
				path_size - strlen(aux_path) - 1);
		}

		// Finally we concat gpu_monitoring.so plug-in name
		strncat(aux_path, "gpu_monitoring.so",
			path_size - strlen(aux_path) - 1);

		// Overwrite the original policy plug-in path
		memcpy(*path, aux_path, path_size);

		// Free the auxiliar policy plug-in path
		free(aux_path);
	}

	strtoa_free(policy_path);
}
#endif
