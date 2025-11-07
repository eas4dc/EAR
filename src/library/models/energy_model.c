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

#include <common/sizes.h>
#include <common/system/symplug.h>
#include <library/common/utils.h>
#include <library/common/verbose_lib.h>
#include <library/models/energy_model.h>
#include <common/config.h>

#define freturn(call, ...)                                                                                             \
    {                                                                                                                  \
        if (call == NULL) {                                                                                            \
            return EAR_SUCCESS;                                                                                        \
        }                                                                                                              \
        return call(__VA_ARGS__);                                                                                      \
    }

#define assert_null(ptr)                                                                                               \
    if (!ptr)                                                                                                          \
        return_msg(EAR_ERROR, Generr.input_null);

#define free_and_null(ptr)                                                                                             \
    free(ptr);                                                                                                         \
    ptr = NULL;

extern uint gpu_optimize;

struct energy_model_s {
    state_t (*model_init)(char *ear_coeffs_path, char *ear_tmp_path, void *arch_desc);
    state_t (*model_project_time)(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_time);
    state_t (*model_project_power)(signature_t *signature, ulong from_ps, ulong to_ps, double *proj_power);
    uint (*model_projection_available)(ulong from_ps, ulong to_ps);
    uint (*model_any_projection_available)();
};

typedef enum em {
    CPU,
    GPU
} em_t;

static const char *energy_model_sym_names[] = {"energy_model_init", "energy_model_project_time",
                                               "energy_model_project_power", "energy_model_projection_available",
                                               "energy_model_any_projection_available"};

static const int model_sym_cnt = 5;

static energy_model_t energy_model_load(settings_conf_t *sconf, architecture_t *arch_desc, em_t em_type);

static state_t build_energy_model_path(char *energy_model_path, size_t energy_model_path_size, em_t em_type,
                                       settings_conf_t *sconf);

energy_model_t energy_model_load_cpu_model(settings_conf_t *sconf, architecture_t *arch_desc)
{
    return energy_model_load(sconf, arch_desc, CPU);
}

energy_model_t energy_model_load_gpu_model(settings_conf_t *sconf, architecture_t *arch_desc)
{
    return energy_model_load(sconf, arch_desc, GPU);
}

state_t energy_model_dispose(energy_model_t energy_model)
{
    assert_null(energy_model);

    free_and_null(energy_model);
    return EAR_SUCCESS;
}

state_t energy_model_project_time(energy_model_t energy_model, signature_t *signature, ulong from_ps, ulong to_ps,
                                  double *proj_time)
{
    assert_null(energy_model);
    freturn(energy_model->model_project_time, signature, from_ps, to_ps, proj_time);
}

state_t energy_model_project_power(energy_model_t energy_model, signature_t *signature, ulong from_ps, ulong to_ps,
                                   double *proj_power)
{
    assert_null(energy_model);
    freturn(energy_model->model_project_power, signature, from_ps, to_ps, proj_power);
}

uint energy_model_projection_available(energy_model_t energy_model, ulong from_ps, ulong to_ps)
{
    if (energy_model) {
        // In the case where the loaded plug-in does not implement this method, a 0 (false) is returned.
        freturn(energy_model->model_projection_available, from_ps, to_ps);
    } else {
        return 0;
    }
}

uint energy_model_any_projection_available(energy_model_t energy_model)
{
    if (energy_model) {
        // In the case where the loaded plug-in does not implement this method, a 0 is returned.
        freturn(energy_model->model_any_projection_available);
    } else {
        return 0;
    }
}

static state_t build_energy_model_path(char *energy_model_path, size_t energy_model_path_size, em_t em_type,
                                       settings_conf_t *sconf)
{
    if (!energy_model_path || !sconf) {
        error_lib("energy_model_path %p settings_conf %p", energy_model_path, sconf);
        return EAR_ERROR;
    }

    int read_env  = (sconf->user_type == AUTHORIZED || USER_TEST);
    char *hack_so = NULL;

    if (read_env) {
        switch (em_type) {
            case CPU:
                hack_so = ear_getenv(HACK_POWER_MODEL);
                break;
            case GPU:
                hack_so = ear_getenv(HACK_GPU_ENERGY_MODEL);
                break;
            default:
                verbose_warning_master("Energy model type (%d) not recognized.", em_type);
        }
    }

    if (hack_so) {
        if (state_ok(
                utils_build_valid_plugin_path(energy_model_path, energy_model_path_size, "models", hack_so, sconf))) {
            verbose_info2_master("Energy model hacked: %s", energy_model_path);
            return EAR_SUCCESS;
        }
    }

    char plug_name[SZ_PATH_SHORT];

    if (em_type == CPU) {
        /* If the configured energy model is 'default', we'll load avx512_model.so. */
        if (strncmp(sconf->installation.obj_power_model, "default", strlen("default")) == 0) {
            snprintf(plug_name, sizeof(plug_name) - 1, "avx512_model.so");
        } else {
            strncpy(plug_name, sconf->installation.obj_power_model, sizeof(plug_name));
        }
    } else {
        /* By now GPU energy plug-in can only be requested either through HACK_GPU_ENERGY_MODEL
         * or by defaut is gpu_nv_model. This may be changed in a future. */
        char *cgpu_optimize = ear_getenv(FLAG_GPU_ENABLE_OPT);
        if (cgpu_optimize == NULL)
            cgpu_optimize = ear_getenv(GPU_ENABLE_OPT);
        if (cgpu_optimize != NULL)
            gpu_optimize = atoi(cgpu_optimize);
        snprintf(plug_name, sizeof(plug_name) - 1, "gpu_%s_model.so", (gpu_optimize) ? "nv" : "dummy");
        verbose_info2_master("Energy model plug_name: %s", plug_name);
    }

    return utils_build_valid_plugin_path(energy_model_path, energy_model_path_size, "models", plug_name, sconf);
}

static energy_model_t energy_model_load(settings_conf_t *sconf, architecture_t *arch_desc, em_t em_type)
{
    if (!sconf || !arch_desc) {
        error_lib("Input arguments are NULL: sconf (%p) arch_desc (%p)", sconf, arch_desc);
        return NULL;
    }

    energy_model_t energy_model = (energy_model_t) malloc(sizeof *energy_model);
    if (!energy_model) {
        // An error occurred mallocing energy_model.
        error_lib("Allocating resources for the %s energy model.", (em_type == CPU) ? "CPU" : "GPU");
        return energy_model;
    }

    // Build the energy model plugins' base path
    char energy_model_base_path[SZ_PATH];

    state_t ret = build_energy_model_path(energy_model_base_path, sizeof(energy_model_base_path), em_type, sconf);

    if (state_fail(ret)) {
        // An error occurred building the energy model plug-in path.
        verbose_error_master("Building the %s energy model path.", (em_type == CPU) ? "CPU" : "GPU");
        free_and_null(energy_model);
        return energy_model;
    }

    // Load the model plug-in symbols
    verbose_info2_master("Energy models: Loading '%s'", energy_model_base_path);

    ret = symplug_open(energy_model_base_path, (void *) energy_model, energy_model_sym_names, model_sym_cnt);

    if (state_fail(ret)) {
        verbose_error_master("An error occurred loading plug-in symbols.");
        free_and_null(energy_model);
        return energy_model;
    }

    // Init the energy model plug-in
    if (energy_model->model_init) {
        ret = energy_model->model_init(sconf->lib_info.coefficients_pathname, sconf->installation.dir_temp, arch_desc);
        if (state_fail(ret)) {
            error_lib("On %s: energy_model_init", energy_model_base_path);
            free_and_null(energy_model);
        }
    }

    return energy_model;
}
