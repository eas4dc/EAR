/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/system/symplug.h>
#include <global_manager/plugins/powercap_plugin.h>

static eargm_powercap_symbols_t *egm_syms = NULL;
static int32_t num_plugins_loaded         = 0;

#define S_NUM 3
static const char *names[S_NUM] = {"plugin_init", "plugin_hard_powercap_decision", "plugin_soft_powercap_decision"};
#define S_FLAGS RTLD_LOCAL | RTLD_NOW

static state_t static_load(const char *in_path, const char *plugs)
{
    eargm_powercap_symbols_t ops_aux = {0};
    char aux_plugs[SZ_PATH]          = {0};
    state_t s                        = EAR_SUCCESS;
    char path[SZ_PATH]               = {0};
    if (plugs == NULL) {
        return EAR_ERROR;
    }
    // Using current folder if install path is NULL
    if (in_path == NULL) {
        in_path = ".";
    }
    // Copying original list to prevent any change
    strcpy(aux_plugs, plugs);
    // List iterations
    char *curr_plug = strtok(aux_plugs, ":");
    while (curr_plug != NULL) {
        // Cleaning plugin auxiliar symbols
        ops_aux = (eargm_powercap_symbols_t) {0};

        // Loading /install/lib/plugins/eargm/lib.so
        sprintf(path, "%s/eargm/%s", in_path, curr_plug);
        if (state_fail(s = plug_open(path, (void **) &ops_aux, (cchar **) names, S_NUM, S_FLAGS))) {
            // Loading ./lib.so
            sprintf(path, "./%s", curr_plug);
            debug("Loading '%s' plugin", path);
            if (state_fail(s = plug_open(path, (void **) &ops_aux, (cchar **) names, S_NUM, S_FLAGS))) {
                // Loading /???/lib.so
                debug("Loading '%s' plugin", curr_plug);
                if (state_fail(s = plug_open(curr_plug, (void **) &ops_aux, (cchar **) names, S_NUM, S_FLAGS))) {
                    error("Unable to load report plugin %s (%d)", state_msg, s);
                    goto next;
                }
            }
        }
        debug("Loaded report plugin: %s", curr_plug);
        egm_syms = realloc(egm_syms, sizeof(eargm_powercap_symbols_t) * (num_plugins_loaded + 1));
        memcpy(&egm_syms[num_plugins_loaded], &ops_aux, sizeof(eargm_powercap_symbols_t));
        num_plugins_loaded++;
    next:
        curr_plug = strtok(NULL, ":");
    }
    if (num_plugins_loaded == 0)
        return EAR_ERROR;
    return EAR_SUCCESS;
}

state_t eargm_plugin_load(const char *path, const char *plugs)
{
    if (!state_ok(static_load(path, plugs))) {
        warning("Could not load EARGM plugins (%s).", plugs);
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

state_t eargm_plugin_init(cluster_conf_t conf[static 1])
{
    state_t ret = EAR_SUCCESS;
    for (int32_t i = 0; i < num_plugins_loaded; i++) {
        ret = egm_syms->plugin_init(conf);
    }
    return ret;
}

state_t eargm_plugin_hard_powercap_decision(powercap_status_t status[static 1], powercap_opt_t opt[static 1],
                                            bool send_options[static 1], cluster_powercap_ctx_t ctx[static 1])
{
    state_t ret = EAR_SUCCESS;
    for (int32_t i = 0; i < num_plugins_loaded; i++) {
        ret = egm_syms->plugin_hard_powercap_decision(status, opt, send_options, ctx);
    }
    return ret;
}

state_t eargm_plugin_soft_powercap_decision(power_check_t power[static 1])
{
    for (int32_t i = 0; i < num_plugins_loaded; i++) {
        egm_syms->plugin_soft_powercap_decision(power);
    }
    return EAR_SUCCESS;
}
