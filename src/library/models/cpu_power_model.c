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
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
 * and COPYING.EPL files.
 */

//#define SHOW_DEBUGS 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/hardware/architecture.h>
#include <library/common/externs.h>
#include <library/models/cpu_power_model.h>
#include <library/common/verbose_lib.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/shared_configuration.h>

extern uint node_mgr_index;

typedef struct cpu_power_model_symbols {
    state_t (*init)       (settings_conf_t *libconf, conf_install_t *conf, architecture_t *arch );
    state_t (*status)     ();
    state_t (*project)    (lib_shared_data_t *data,shsignature_t *sig, node_mgr_sh_data_t *nmgr, uint who);
} cpu_power_models_sym_t;

// Static data
static cpu_power_models_sym_t cpu_power_models_syms_fun;

static const char *cpu_power_models_syms_nam[] = {
    "cpu_power_model_init_arch",
    "cpu_power_model_status_arch",
    "cpu_power_model_project_arch",
};

static const int cpu_power_models_funcs_n = 3;

static uint cpu_power_model_loaded = 0;

// static conf_install_t   copy_conf;
static architecture_t   copy_arch;
static settings_conf_t *copy_settings;

state_t cpu_power_model_load(settings_conf_t *libconf, architecture_t *arch_desc,
                             uint load_dummy)
{
    char basic_path[SZ_PATH_INCOMPLETE];

    char *obj_path = getenv(HACK_CPU_POWER_MODEL);
    char *ins_path = getenv(HACK_EARL_INSTALL_PATH);

    uint already_loaded = cpu_power_model_loaded;

    char *default_model = "cpu_power_model_default.so";
    char *dummy_model   = "cpu_power_model_dummy.so";

    char *model = default_model;
    if (load_dummy) {
        model = dummy_model;
    }

    if ((obj_path == NULL && ins_path == NULL) || libconf->user_type != AUTHORIZED) {

        xsnprintf(basic_path, sizeof(basic_path), "%s/models/%s",
                  libconf->installation.dir_plug, model);

        obj_path = basic_path;

    } else {

        if (obj_path == NULL) {

            snprintf(basic_path, sizeof(basic_path), "%s/plugins/models/%s", ins_path, model);

            obj_path = basic_path;
        }
    }
    verbose_master(2, "CPU power model path: %s", obj_path);

    state_t st = symplug_open(obj_path, (void **) &cpu_power_models_syms_fun,
                              cpu_power_models_syms_nam, cpu_power_models_funcs_n);
    if (st == EAR_SUCCESS) {
        cpu_power_model_loaded = 1;
    }

    if (!already_loaded) {

        // memcpy(&copy_conf, &libconf->installation, sizeof(conf_install_t));
        memcpy(&copy_arch, arch_desc, sizeof(architecture_t));

        copy_settings = libconf;
    }

    return st;
}

state_t cpu_power_model_init()
{
    state_t init_status = EAR_ERROR;

    if (cpu_power_models_syms_fun.init != NULL) {
        init_status = cpu_power_models_syms_fun.init(copy_settings,
                      &copy_settings->installation, &copy_arch);
    }

    if (init_status == EAR_SUCCESS) {
        return EAR_SUCCESS;
    }

    // If coefficients are not available, we will load the dummy cpu power model.
    if (state_ok(cpu_power_model_load(copy_settings, &copy_arch, 1))) {
        if (cpu_power_models_syms_fun.init != NULL) {
            init_status = cpu_power_models_syms_fun.init(copy_settings, &copy_settings->installation, &copy_arch);
        }
    } else {
        return EAR_ERROR;
    }

    return init_status;
}

state_t cpu_power_model_status()
{
	if (!cpu_power_model_loaded) return EAR_ERROR;
	if (cpu_power_models_syms_fun.status != NULL) return cpu_power_models_syms_fun.status();
	return EAR_SUCCESS;
}

state_t cpu_power_model_project(lib_shared_data_t *data, shsignature_t *sig, node_mgr_sh_data_t *nmgr)
{
	if (cpu_power_models_syms_fun.project != NULL) cpu_power_models_syms_fun.project(data, sig, nmgr, node_mgr_index);
	return EAR_SUCCESS;
}
