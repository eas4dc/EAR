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

#include <common/system/file.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// Externs
extern int _sp;

// Buffers
char path_mas[SZ_PATH];
char path_stp[SZ_PATH];
char path_job[SZ_PATH];

//
int fd_mas = -1;
int fd_stp = -1;
int fd_job = -1;

int lock_clean(char *path_tmp)
{
	plug_verbose(_sp, 3, "function lock_clean");

	// 1. Clean erun.master.lock
	// 2. Clean erun.step.lock
	file_unlock_master(fd_mas, path_mas);

	if (strlen(path_stp) > 0) {
		file_clean(path_stp);
	}

	return 0;
}

int lock_master(char *path_tmp)
{
	plug_verbose(_sp, 3, "function lock_master");

	//
	sprintf(path_mas, "%s/erun.master.lock", path_tmp);
	fd_mas = file_lock_master(path_mas);

	if (fd_mas >= 0) {
		return 1;
	}

	fd_mas = -1;
	return 0;
}

int lock_step(char *path_tmp, int job_id, int step_id)
{
	plug_verbose(_sp, 3, "function lock_step (%d,%d)", job_id, step_id);

	sprintf(path_job, "%s/erun.%d.lock", path_tmp, job_id);

	// 1. Test erun.1394394.lock
	//		 SLURM_STEP_ID -> Set step_id to SLURM_STEP_ID
	//		!SLURM_STEP_ID
	// 			!file -> Set step_id to 0
	// 			 file -> Read the step_id + 1
	// 2. Clean erun.1394394.lock
	// 3. Create erun.1394394.lock with the step_id
	// 4. Return step_id
	if (step_id == 0) {
		if (state_ok(file_read(path_job, (char *) &step_id, sizeof(int)))) {
			plug_verbose(_sp, 2, "read %d step_id in the file '%s'", step_id, path_job);
			step_id += 1;
		} else {
			step_id = 0;
		}
	}

	file_clean(path_job);
	file_write(path_job, (char *) &step_id, sizeof(int));

	return step_id;
}

int unlock_step(char *path_tmp, int step_id)
{
	plug_verbose(_sp, 3, "function unlock_step");

	//
	sprintf(path_stp, "%s/erun.step.lock", path_tmp);

	// 1. Create and write erun.step.lock
	file_write(path_stp, (char *) &step_id, sizeof(int));

	return step_id;
}

int spinlock_step(char *path_tmp, int step_id)
{
	plug_verbose(_sp, 3, "function spinlock_step");
	
	//
	sprintf(path_stp, "%s/erun.step.lock", path_tmp);
	
	// 1. While !erun.step.lock file
	// 2. Read erun.step.lock step_id
	// 3. Return step_id
	while (access(path_stp, F_OK) != 0);

	file_read(path_stp, (char *) &step_id, sizeof(int));
	plug_verbose(_sp, 2, "read %d step_id in the file '%s'", step_id, path_job);

	return step_id;
}
