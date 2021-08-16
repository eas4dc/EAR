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
static char path_master[SZ_PATH];
static char path_step[SZ_PATH];
static char path_job[SZ_PATH];
static char command[SZ_PATH];
//
static int fd_master = -1;

int lock_clean(char *path_tmp, int step_id)
{
	plug_verbose(_sp, 3, "function lock_clean");
	// 1. Creating the cleaning command
	sprintf(command, "rm %s/erun.*.lock 2> /dev/null", path_tmp);
	// 2. Remove erun.master.lock
	file_unlock_master(fd_master, path_master);
	// 3. Remove erun.step.lock
	file_clean(path_step);
	// 5. Remove erun.%job_id.lock
	file_clean(path_job);
	// 6. Launch cleaning command to remove old erun files.
	system(command);
	// 5. Write again erun.%job_id.lock 
	file_write(path_job, (char *) &step_id, sizeof(int));
	return 0;
}

int lock_master(char *path_tmp)
{
    plug_verbose(_sp, 3, "function lock_master");
    // Getting the master lock
    sprintf(path_master, "%s/erun.master.lock", path_tmp);
    fd_master = file_lock_master(path_master);
    // Returning FD means it got the unique master lock
    return fd_master >= 0;
}

int lock_job(char *path_tmp, int job_id, int step_id)
{
	plug_verbose(_sp, 3, "function lock_step (%d,%d)", job_id, step_id);
	// Forming erun.%job_id.lock path
	sprintf(path_job, "%s/erun.%d.lock", path_tmp, job_id);
    // Locking in node processes:
    // 1. Try to take erun.master.lock.
    //      2. If taken erun.master.lock
    //          2.1. Test erun.%job_id.lock
    //                SLURM_STEP_ID -> Set step_id to SLURM_STEP_ID
    //               !SLURM_STEP_ID
    //                   erun.%job_id.lock -> Read the step_id + 1
    //                  !erun.%job_id.lock -> Set step_id to 0
    //          2.2. Remove erun.%job_id.lock
    //          2.3. Create erun.%job_id.lock with the step_id
    //          2.4. Create erun.step.lock
    //          2.5. Write %step_id in erun.step.lock
    //          2.6. Execute the application
    //          2.7. Remove erun.master.lock
    //          2.8. Remove erun.step.lock
    //          2.9. Launch a cleanning command
    //          2.10. Create erun.%job_id.lock with the step_id
    //      3. If not taken erun.master.lock
    //          3.1 Spin while can't access erun.step.lock
    //          3.2 Read the %step_id in erun.step.lock
	if (step_id == 0) {
		if (state_ok(file_read(path_job, (char *) &step_id, sizeof(int)))) {
        	    	plug_verbose(_sp, 2, "read %d step_id in the file '%s'", step_id, path_job);
			step_id += 1;
		} else {
			// In case the reading fails, force step_id to 0
			step_id = 0;
		}
	}
	return step_id;
}

int unlock_step(char *path_tmp, int step_id)
{
    plug_verbose(_sp, 3, "function unlock_step");
    // 1. Form the erun.step.lock path
    sprintf(path_step, "%s/erun.step.lock", path_tmp);
    // 2. Create and write erun.step.lock
    file_write(path_step, (char *) &step_id, sizeof(int));
    return step_id;
}

int spinlock_step(char *path_tmp, int step_id)
{
    plug_verbose(_sp, 3, "function spinlock_step");
    // 1. Form the erun.step.lock path
    sprintf(path_step, "%s/erun.step.lock", path_tmp);
    // 2. While !erun.step.lock file, spinlock
    while (access(path_step, F_OK) != 0);
    // 3. Read the step_id contained in erun.step.lock
    file_read(path_step, (char *) &step_id, sizeof(int));
    // 4. Return step_id
    plug_verbose(_sp, 2, "read %d step_id in the file '%s'", step_id, path_job);
    return step_id;
}
