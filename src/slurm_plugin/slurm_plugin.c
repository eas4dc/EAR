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

#define _GNU_SOURCE
#include <sched.h>
#include <sys/sysinfo.h>

#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_rcom.h>
#include <slurm_plugin/slurm_plugin_options.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// Spank
SPANK_PLUGIN(EAR_PLUGIN, 1);

//
plug_serialization_t sd;

#if !ESPANK_STACKABLE || ERUN
// Function order:
// 	- Local 1
// 	- Remote 1
int slurm_spank_init(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_init");

	// Deserialize components converts environment variables in variables of
	// sd context, also returns ESPANK_ERROR if plugin is disabled.
	if (plug_deserialize_components(sp) != ESPANK_SUCCESS) {
		return ESPANK_SUCCESS;
	}
	// Here the --ear-... flags are registered.
	_opt_register(sp, ac, av);

	return ESPANK_SUCCESS;
}

// Function order:
// 	- Local 2
// 	- Remote 0
int slurm_spank_init_post_opt(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_init_post_opt");

	// Disabling policy
	// 	- Disable plugin means no plugin, so no serialization, no library
	//	- Disable library means the plugin works but no library is loaded
	if (!plug_context_is(sp, Context.local)) {
		return ESPANK_SUCCESS;
	}
	// Deserialize components converts environment variables in variables of
	// sd context, also returns ESPANK_ERROR if plugin is disabled.
	if (plug_deserialize_components(sp) != ESPANK_SUCCESS) {
		return ESPANK_SUCCESS;
	}
	// Deserialize local cleans the environment and gets user information.
	// If failed during the deserialization or plugstack reading, then the
	// plugin is disabled. The later serialization is also cancelled,
	// preventing the plugin to work in remote environment.
	if (plug_deserialize_local(sp, &sd) != ESPANK_SUCCESS) {
		return plug_component_setenabled(sp, Component.plugin, 0);
	}
	// Serialize remote converts variables in environment variables to be
	// received by the remote environment. 
	plug_serialize_remote(sp, &sd);
	// Printing the environment.
	plug_print_variables(sp);

	return ESPANK_SUCCESS;
}
#endif

// Helper function (second part of slurm_spank_user_init).
int slurm_spank_user_init_eard(spank_t sp)
{
	plug_verbose(sp, 2, "function slurm_spank_user_init_eard");

	if (!sd.erun.is_erun || sd.erun.is_master) {
		// If no shared services, EARD contact won't work, so plugin disabled.
		if (fail(plug_shared_readservs(sp, &sd))) {
			return ESPANK_ERROR;
		}
		// If no frequencies, then no more processing.
		if (fail(plug_shared_readfreqs(sp, &sd))) {
			return ESPANK_ERROR;
		}
		// Same.
		if (fail(plug_read_application(sp, &sd))) {
			return ESPANK_ERROR;
		}
		// Contacting EARD to tell that a job is starting.
		if (fail(plug_rcom_eard_job_start(sp, &sd))) {
			return ESPANK_ERROR;
		}
	}

	// LD_PRELOAD can be DISABLED by plugstack.conf or flag is OFF
	// LD_PRELOAD can be DISABLED by EARD when reading settings
	// LD_PRELOAD can be DISABLED if energy tag flag is present (during remote deserialization)
	//
	// If is an SBATCH remote environment, then there is no serialization
	// to task, so then is no libearld.so.
	if (plug_context_was(&sd, Context.srun)) {
		if (plug_component_isenabled(sp, Component.library)) {
			if (fail(plug_shared_readsetts(sp, &sd))) {
				return ESPANK_ERROR;
			}
		} else {
			plug_verbose(sp, 2, "library is not enabled");
		}
		if (plug_component_isenabled(sp, Component.library)) {
			plug_serialize_task_settings(sp, &sd);
			plug_serialize_task_preload(sp, &sd);
		}
	}

	return ESPANK_SUCCESS;
}

// Function order:
// 	- Local 0
// 	- Remote 2
int slurm_spank_user_init(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_user_init");

	// If we are not in remote environment (not sure if required).
	if (!plug_context_is(sp, Context.remote)) {
		return ESPANK_SUCCESS;
	}
	// Components are special environment variables to control the plugin
	// behaviour and are treated differently. 
	if (plug_deserialize_components(sp) != ESPANK_SUCCESS) {
		return ESPANK_SUCCESS;
	}
	// Reading plugstack.conf file.
	if (plug_read_plugstack(sp, ac, av, &sd) != ESPANK_SUCCESS) {
		return plug_component_setenabled(sp, Component.plugin, 0);
	}
	// Deserialization saves current environment variables in variables. It can
	// also undefine the environment partially. It can return ESPANK_ERROR if
	// detects a problem.
	if (plug_deserialize_remote(sp, &sd) != ESPANK_SUCCESS) {
		return plug_component_setenabled(sp, Component.plugin, 0);
	}
	// Just the master (one node) have to contact EARGMD.
	if (sd.subject.is_master && !sd.erun.is_erun) {
		plug_rcom_eargmd_job_start(sp, &sd);
	}
	// The second part of this function (^).
	if (fail(slurm_spank_user_init_eard(sp))) {
		// Loading library anyway, because if a single node in a job runs
		// without the library, it can freeze during EAR synchronization.
		if (plug_component_isenabled(sp, Component.library)) {
			plug_serialize_task_preload(sp, &sd);
		}
	}
	//
	plug_print_variables(sp);

	return ESPANK_SUCCESS;
}

int slurm_spank_task_init(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_task_init");

	char buffer1[2048];
	char buffer2[2048];
	cpu_set_t mask;
	int cpus_count;
	int cpu_set;
	int task;
	int i;

	// By default is error
	cpu_set = -1;
	task = -1;
	// Local task
	if (!getenv_agnostic(sp, "SLURM_LOCALID", buffer2, 32)) {
		return ESPANK_SUCCESS;
	}
	task = atoi(buffer2);
	if (!plug_verbosity_test(sp, 2)) {
		return ESPANK_SUCCESS;
	}
	if (task > 0 && !plug_verbosity_test(sp, 3)) {
		return ESPANK_SUCCESS;
	} 
	// Getting mask and number of processors
	CPU_ZERO(&mask);
	cpus_count = get_nprocs();
	// 
	if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
		plug_verbose(sp, 2, "affinity of local task %d: ERROR", task);
	} else {
		sprintf(buffer1, "TASK[%d]: ", task);
		for (i = 0; i < cpus_count; ++i) {
			cpu_set = CPU_ISSET(i, &mask);
			strcpy(buffer2, buffer1);
			xsprintf(buffer1, "%s CPU[%d]=%d", buffer2, i, cpu_set);
		}
		plug_verbose(sp, 2, "%s", buffer1);
	}
	
	return (ESPANK_SUCCESS);
}

// Function order:
// 	- Local 0
// 	- Remote 3e
int slurm_spank_task_exit (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_task_exit");

	spank_err_t err;
	int status = 0;
	
	// Plugin enabled test
	if (!plug_component_isenabled(sp, Component.plugin)) {
		return ESPANK_SUCCESS;
	}
	// Getting the reason for the end of the process.
	if (sd.subject.exit_status == 0)
	{
		err = spank_get_item (sp, S_TASK_EXIT_STATUS, &status);
		// If get item goes OK
		if (err == ESPANK_SUCCESS) {
			sd.subject.exit_status = WEXITSTATUS(status);
		}
	}

	return ESPANK_SUCCESS;
}

int slurm_spank_exit (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_exit");

	// If remote context
	if (!plug_context_is(sp, Context.remote)) {
		return ESPANK_SUCCESS;
	}
	// Plugin enabled test
	if (!plug_component_isenabled(sp, Component.plugin)) {
		return ESPANK_SUCCESS;
	}
	// This function is called after task exit functions and once per node.
	plug_rcom_eard_job_finish(sp, &sd);
	// Master is the node master.
	if (sd.subject.is_master) {
		plug_rcom_eargmd_job_finish(sp, &sd);
	}

	return ESPANK_SUCCESS;
}

#if 0
int slurm_spank_slurmd_init (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_slurmd_init");
	return (ESPANK_SUCCESS);
}

int slurm_spank_slurmd_exit (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_slurmd_exit");
	return (ESPANK_SUCCESS);
}

int slurm_spank_job_prolog (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_job_prolog");
	return (ESPANK_SUCCESS);
}

int slurm_spank_task_init_privileged (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_task_init_privileged");
	return (ESPANK_SUCCESS);
}

int _slurm_spank_local_user_init (spank_t sp, int ac, char **av)
{
        plug_verbose(sp, 2, "function slurm_spank_local_user_init");
        return (ESPANK_SUCCESS);
}

int _slurm_spank_init_post_opt(spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_init_post_opt");
	return (ESPANK_SUCCESS);
}

int slurm_spank_task_post_fork (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_task_post_fork");
	return (ESPANK_SUCCESS);
}

int slurm_spank_job_epilog (spank_t sp, int ac, char **av)
{
	plug_verbose(sp, 2, "function slurm_spank_job_epilog");
	return (ESPANK_SUCCESS);
}
#endif
