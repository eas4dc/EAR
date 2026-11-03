/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
// #define SHOW_DEBUGS 1
#include <common/config/config_env.h>
#include <metrics/common/script.h>
#include <management/gpu/archs/script.h>

static char        path[PATH_MAX];
static gpu_devs_t *devs;
static uint        devs_count;

static int find_path(const char *file_name, char *buffer)
{
    #define is(fmt, ...) \
    sprintf(buffer, fmt, __VA_ARGS__); \
    if (script_is_executable(buffer)) { \
       return 1; \
    }

    is("%s/%s/bin", ear_getenv(ENV_PATH_EAR), file_name);
    is("%s/%s/sbin", ear_getenv(ENV_PATH_EAR), file_name);
    is("%s/%s", ear_getenv(ENV_PATH_TMP), file_name);
    is("%s/%s", ear_getenv(ENV_PATH_ETC), file_name);
    is("%s/%s", ear_getenv(HACK_EAR_TMP), file_name);
    is("%s/%s", ear_getenv(HACK_EAR_ETC), file_name)
    is("/tmp/%s", file_name);
    is("/var/%s", file_name);
    is("/opt/%s", file_name);
    return 0;
}

void mgt_gpu_script_load(mgt_gpu_ops_t *ops)
{
    // Script is a secondary internal API. It is intended to bypass the
    // privileged write functions to a script given by the admins. In other
    // words, it only fills the empty functions left by another class.
    if (ops->get_devices == NULL) {
        return;
    }
    if (!find_path("ear-smi.sh", path)) {
        return;
    }
    debug("Opened %s", path);
    if (state_fail(ops->get_devices(NULL, &devs, &devs_count))) {
        return;
    }
    apis_put(ops->freq_limit_reset, mgt_gpu_script_freq_limit_reset);
    apis_put(ops->freq_limit_set  , mgt_gpu_script_freq_limit_set);
    apis_put(ops->power_cap_reset , mgt_gpu_script_power_cap_reset);
    apis_put(ops->power_cap_set   , mgt_gpu_script_power_cap_set);
    // Get devices
    debug("Loaded SCRIPT");
}

state_t mgt_gpu_script_freq_limit_reset(ctx_t *c)
{
    int d;
    // The script will receive:
    for (d = 0; d < devs_count; ++d) {
        script_exec(path, 0, 1, "FREQ RESET %d", devs[d].index);
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_script_freq_limit_set(ctx_t *c, ulong *khz)
{
    int d;
    for (d = 0; d < devs_count; ++d) {
        script_exec(path, 0, 1, "FREQ SET %d %lu", devs[d].index, khz[d]);
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_script_power_cap_reset(ctx_t *c)
{
    int d;
    for (d = 0; d < devs_count; ++d) {
        script_exec(path, 0, 1, "POWER RESET %d", devs[d].index);
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_script_power_cap_set(ctx_t *c, uint32_t *watts)
{
    int d;
    for (d = 0; d < devs_count; ++d) {
        script_exec(path, 0, 1, "POWER SET %d %u", devs[d].index, watts[d]);
    }
    return EAR_SUCCESS;
}