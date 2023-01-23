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

#include <common/system/file.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// Externs
extern int _sp;
// Buffers
static char path_master_lock[SZ_PATH]; // */master.lock (sets the master process)
static char path_master_step[SZ_PATH]; // */master.step (sets the master process)
static char path_slave_lock[SZ_PATH];  // */slave.lock (master writes step_id for the slaves)
static char path_slave_step[SZ_PATH];  // */slave.step (master writes step_id for the slaves)
static char path_job[SZ_PATH];
static char command[SZ_PATH];
//
static int fd_master = -1;

#define PERMS000(T) 0
#define PERMS100(T) S_IR ## T
#define PERMS010(T) S_IW ## T
#define PERMS001(T) S_IX ## T
#define PERMS110(T) (PERMS100(T)) | (PERMS010(T))
#define PERMS111(T) (PERMS100(T)) | (PERMS010(T)) | (PERMS001(T))
#define PEXPA(...) __VA_ARGS__
#define PERMS(u, g, o) PEXPA(PERMS ## u)(USR) | PEXPA(PERMS ## g)(GRP) | PEXPA(PERMS ## o)(OTH)

// Master/slave 1
int lock_master(char *path_tmp, int job_id)
{
    plug_verbose(_sp, 3, "function lock_master");
    // Creating temp folder
    if ((access(path_tmp, W_OK) != 0) && (errno == ENOENT)) {
        mkdir(path_job, PERMS(111,000,000)); 
    }
    // Creating erun folder
    sprintf(path_job, "%s/erun", path_tmp);
    mkdir(path_job, PERMS(111,110,110)); 
    // Creating job folder
    sprintf(path_job, "%s/erun/%d", path_tmp, job_id);
    mkdir(path_job, PERMS(111,110,110));
    // Setting the paths
    xsprintf(path_master_lock, "%s/master.lock", path_job);
    xsprintf(path_master_step, "%s/master.step", path_job);
    xsprintf(path_slave_lock, "%s/slave.lock", path_job);
    xsprintf(path_slave_step, "%s/slave.step", path_job);
    //
	plug_verbose(_sp, 3, "Trying to get the lock %s", path_master_lock);
    if ((fd_master = file_lock_master(path_master_lock)) < 0) {
	    plug_error(_sp, "while taking lock_master %s", strerror(errno));
    }
    // Returning FD means it got the unique master lock
    return fd_master >= 0;
}

// Master 2
int master_getstep(int job_id, int step_id)
{
	plug_verbose(_sp, 3, "function master_getstep (%d,%d)", job_id, step_id);
    // Locking in node processes:
    // 1. Try to take master.lock
    //      2. If taken master.lock
    //          2.1. Test master.step
    //                SLURM_STEP_ID -> Set step_id to SLURM_STEP_ID
    //               !SLURM_STEP_ID
    //                   master.step -> Read the %step_id + 1
    //                  !master.step -> Set %step_id to 0
    //          2.2. Remove slave.step
    //          2.3. Create slave.step with the %step_id
    //          2.4. Write %step_id in slave.step
    //          2.5. Create step.lock
    //          2.6. Execute the application
    //          2.7. Remove master.lock
    //          2.8. Remove slave.lock
    //          2.9. Launch a cleanning command
    //          2.10. Update/recreate master.step with the %step_id
    //      3. If not taken master.lock
    //          3.1 Spin while can't access slave.lock
    //          3.2 Read the %step_id in slave.step
	if (step_id == 0) {
		if (state_ok(ear_file_read(path_master_step, (char *) &step_id, sizeof(int)))) {
        	plug_verbose(_sp, 2, "read %d step_id in the file '%s'", step_id, path_master_step);
			step_id += 1;
		} else {
			// In case the reading fails, force step_id to 0
			step_id = 0;
		}
	}
	return step_id;
}

// Master 3
void unlock_slave(int job_id, int step_id)
{
    plug_verbose(_sp, 3, "function unlock_step");
    // 1. Create and write slave.step
    ear_file_write(path_slave_step, (char *) &step_id, sizeof(int));
    // 2. Create and write step.lock
    ear_file_write(path_slave_lock, (char *) &step_id, sizeof(int));
}

// Slave 2
void spinlock_slave(int job_id)
{
    plug_verbose(_sp, 3, "function spinlock_slave");
    // 1. While !step.lock file, spinlock
    plug_verbose(_sp, 3," Waiting for step lock file %s", path_slave_lock);
    while (access(path_slave_lock, F_OK) != 0) {}
}

// Slave 2
int slave_getstep(int job_id, int step_id)
{
    // 1. Read the %step_id contained in step.step.id
    ear_file_read(path_slave_step, (char *) &step_id, sizeof(int));
    plug_verbose(_sp, 2, "read %d step_id in the file '%s'", step_id, path_slave_step);
    // 2. Return %step_id
    return step_id;
}

// Master 4
void files_clean(int job_id, int step_id)
{
	plug_verbose(_sp, 3, "function lock_clean");
	// 1. Remove master.lock (paths are set in lock_master())
	file_unlock_master(fd_master, path_master_lock);
	// 2. Remove slave.lock
	ear_file_clean(path_slave_lock);
	// 3. Remove slave.step
	ear_file_clean(path_slave_step);
	// 4. Creating the 'by the case' powerful cleaning command
	xsprintf(command, "rm -f %s/*.step 2> /dev/null", path_job);
	system(command);
	// 5. Creating the other 'by the case' powerful cleaning command
	xsprintf(command, "rm -f %s/*.lock 2> /dev/null", path_job);
	system(command);
	// 5. Write again master_step 
	ear_file_write(path_master_step, (char *) &step_id, sizeof(int));
}

// Master 2 (when error 1 or 2)
void folder_clean(int job_id)
{
	// 1. Creating the cleaning command
	xsprintf(command, "rm -rf %s 2> /dev/null", path_job);
    system(command);
}

// Flag --clean
void all_clean(char *path_tmp)
{
    xsprintf(command, "rm -rf %s/erun &> /dev/null", path_tmp);
	plug_verbose(_sp, 3, "Executing %s", command);
    system(command);
}
