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


#include <library/models/energy_model.h>
#include <library/common/verbose_lib.h>
#include <common/system/symplug.h>


#define freturn(call, ...) \
{ \
	if (call == NULL) { \
		return EAR_SUCCESS; \
	} \
	return call (__VA_ARGS__); \
}

#define assert_null(ptr) \
	if (!ptr) return_msg(EAR_ERROR, Generr.input_null);

#define free_and_null(ptr)\
	free(ptr);\
	ptr = NULL;


struct energy_model_s
{
	state_t (*model_init)	(char *ear_etc_path, char *ear_tmp_path, void *arch_desc);
	state_t (*model_project_time) (signature_t *signature, ulong from_ps, ulong to_ps, double *proj_time); 
	state_t (*model_project_power) (signature_t *signature, ulong from_ps, ulong to_ps, double *proj_power);
	uint (*model_projection_available) (ulong from_ps, ulong to_ps);
	uint (*model_any_projection_available) ();
};


typedef enum em
{
	CPU,
	GPU
} em_t;


static const char *energy_model_sym_names[] = {
	"energy_model_init",
	"energy_model_project_time",
	"energy_model_project_power",
	"energy_model_projection_available",
	"energy_model_any_projection_available"
};


static const int model_sym_cnt = 5;


static energy_model_t energy_model_load(uint user_type, conf_install_t *data, architecture_t *arch_desc, em_t em_type);


static state_t build_energy_model_path(char *energy_model_path, size_t energy_model_path_size, char *base_plugin_path, char *base_energy_model_obj, uint user_type, em_t em_type);


energy_model_t energy_model_load_cpu_model(uint user_type, conf_install_t *data, architecture_t *arch_desc)
{
	return energy_model_load(user_type, data, arch_desc, CPU);
}


energy_model_t energy_model_load_gpu_model(uint user_type, conf_install_t *data, architecture_t *arch_desc)
{
	return energy_model_load(user_type, data, arch_desc, GPU);
}


state_t energy_model_dispose(energy_model_t energy_model)
{
	assert_null(energy_model);

	free_and_null(energy_model);
	return EAR_SUCCESS;
}


state_t energy_model_project_time(energy_model_t energy_model, signature_t *signature, ulong from_ps, ulong to_ps, double *proj_time)
{
	assert_null(energy_model);
	freturn(energy_model->model_project_time, signature, from_ps, to_ps, proj_time);
}


state_t energy_model_project_power(energy_model_t energy_model, signature_t *signature, ulong from_ps, ulong to_ps, double *proj_power)
{
	assert_null(energy_model);
	freturn(energy_model->model_project_power, signature, from_ps, to_ps, proj_power);
}


uint energy_model_projection_available(energy_model_t energy_model, ulong from_ps, ulong to_ps)
{
	if (energy_model)
	{
		// In the case where the loaded plug-in does not implement this method, a 0 (false) is returned.
		freturn(energy_model->model_projection_available, from_ps, to_ps);
	} else
	{
		return 0;
	}
}


uint energy_model_any_projection_available(energy_model_t energy_model)
{
	if (energy_model)
	{
		// In the case where the loaded plug-in does not implement this method, a 0 is returned.
		freturn(energy_model->model_any_projection_available);
	} else
	{
		return 0;
	}
}


static state_t build_energy_model_path(char *energy_model_path, size_t energy_model_path_size, char *base_plugin_path, char *base_energy_model_obj, uint user_type, em_t em_type)
{
	if (!energy_model_path || !base_plugin_path || !base_energy_model_obj)
	{
		error_lib("Input arguments are NULL: energy_model_path (%p) base_plugin_path (%p) base_energy_model_obj (%p)",
							energy_model_path, base_plugin_path, base_energy_model_obj);
		return EAR_ERROR;
	}

	// Build the energy model plugins' base path
	int ret = snprintf(energy_model_path, energy_model_path_size, "%s/models/", base_plugin_path);
	if (ret <= 0 || ret >= energy_model_path_size)
	{
		// An error occurred on the above snprintf. You can escape below code for reading.
		if (ret >= energy_model_path_size)
		{
			error_lib("%s must be at most %d bytes long.", base_plugin_path, SZ_PATH_INCOMPLETE - 9);
		} else
		{
			// ret <= 0
			error_lib("An internal error occurred: %d", errno);
		}
		return EAR_ERROR;
	}

	// No error occurred on snprintf

	char *energy_model_obj = base_energy_model_obj;

	// Hacks can be read if the user is authorized or if USER_TEST is enabled at config time
	uint auth_user = (user_type == AUTHORIZED || USER_TEST);

	uint hacking_err = 0; // Util variable to check later for errors in the below conditional block.

	if (auth_user)
	{
		char *hack_energy_model_obj = NULL;

		switch(em_type)
		{
			case CPU:
				hack_energy_model_obj = ear_getenv(HACK_POWER_MODEL);
				break;
			case GPU:
				hack_energy_model_obj = ear_getenv(HACK_GPU_ENERGY_MODEL);
				break;
			default:
				verbose_warning_master("Energy model type (%d) not recognized.", em_type);
		}

		if (hack_energy_model_obj)
		{
			energy_model_obj = hack_energy_model_obj; // Energy model hacked
		}

		char *hack_earl_path = ear_getenv(HACK_EARL_INSTALL_PATH);
		if (hack_earl_path)
		{
			// Base path hacked
			ret = snprintf(energy_model_path, energy_model_path_size, "%s/plugins/models/", hack_earl_path);

			// An error occurred. You can escape below code for reading.
			if (ret >= energy_model_path_size)
			{
				error_lib("%s must be at most %d bytes long.", base_plugin_path, SZ_PATH_INCOMPLETE - 17);
				hacking_err = 1;
			} else if (ret < 0)
			{
				error_lib("An internal error occurred: %d", errno);
				hacking_err = 1;
			}
		}
	}

	if (!hacking_err)
	{
		size_t max_permitted_bytes = energy_model_path_size - strlen(energy_model_path) - 1;

		uint cpu_model = (em_type == CPU);
		uint gpu_model = (em_type == GPU);

		uint def_model = !strncmp(energy_model_obj, "default", strlen("default"));

		// We use the default CPU power model if "default" was set or when no object model specified
		if (cpu_model && (def_model || !energy_model_obj))
		{
			strncat(energy_model_path, "avx512_model.so", max_permitted_bytes);
		} else if (cpu_model || (gpu_model && energy_model_obj))
		{
			strncat(energy_model_path, energy_model_obj, max_permitted_bytes);
		} else
		{
			if (gpu_model)
			{
				error_lib("GPU model requested without any energy plugin specified.");
			} else
			{
				error_lib("Unknown energy model type requested: %d", em_type);
			}

			return EAR_ERROR;
		}

		debug("Energy model path to be loaded: %s", energy_model_path);

	} else // An error occurred inside hacking code block.
	{
		return EAR_ERROR;
	}

	return EAR_SUCCESS;
}

static energy_model_t energy_model_load(uint user_type, conf_install_t *data, architecture_t *arch_desc, em_t em_type)
{
	if (!data || !arch_desc)
	{
		error_lib("Input arguments are NULL: data (%p) arch_desc (%p)", data, arch_desc);
		return NULL;
	}

	energy_model_t energy_model = (energy_model_t) malloc(sizeof * energy_model);
	if (!energy_model)
	{
		// An error occurred mallocing energy_model.
		error_lib("Allocating resources for the %s energy model.", (em_type == CPU) ? "CPU" : "GPU");
		return energy_model;
	}

	// Build the energy model plugins' base path
	char energy_model_base_path[SZ_PATH_INCOMPLETE];

	state_t ret = build_energy_model_path(energy_model_base_path, sizeof(energy_model_base_path),
																				data->dir_plug, data->obj_power_model, user_type, em_type);

	if (state_fail(ret))
	{
		// An error occurred building the energy model plug-in path.
		free_and_null(energy_model);
		return energy_model;
	}

	// Load the model plug-in symbols
	verbose_info2_master("Energy models: Loading '%s'", energy_model_base_path);

	ret = symplug_open(energy_model_base_path, (void *) energy_model,
										 energy_model_sym_names, model_sym_cnt);

	if (state_fail(ret))
	{
		verbose_error_master("An error occurred loading plug-in symbols.");
		free_and_null(energy_model);
		return energy_model;
	}

	// Init the energy model plug-in
	if (energy_model->model_init)
	{
		ret = energy_model->model_init(data->dir_conf, data->dir_temp, arch_desc);
		if (state_fail(ret))
		{
			error_lib("On %s: energy_model_init", energy_model_base_path);
			free_and_null(energy_model);
		}
	}

	return energy_model;
}
