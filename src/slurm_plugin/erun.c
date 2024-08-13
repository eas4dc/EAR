/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <sys/wait.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>
#include <slurm_plugin/erun_lock.h>
#include <slurm_plugin/erun_signals.h>

//
extern plug_serialization_t sd;
// Buffers
char plug_tmp[SZ_NAME_LARGE];
char plug_etc[SZ_NAME_LARGE];
char plug_pfx[SZ_NAME_LARGE];
char plug_def[SZ_NAME_LARGE];
char path_app[SZ_PATH];
char path_tmp[SZ_PATH];
char path_sys[SZ_PATH];
char buffer[SZ_PATH];
//
static int _inactive;
static int _error;
static int _clean;
static int _help;
//
static int _at;
       int _sp;
//
static char *args[128];
char       **_argv;
int          _argc;

int plug_is_action(int _ac, int action)
{
	return _ac == action;
}

int help(int argc, char *argv[])
{
	printf("Usage: %s [OPTIONS]\n", argv[0]);
	printf("\nOptions:\n");
//	printf("\t--job-id=<arg>\t\tSet the JOB_ID.\n");
	printf("\t--nodes=<arg>\t\tSets the number of nodes.\n");
	printf("\t--program=<arg>\t\tSets the program to run.\n");
//	printf("\t--plugstack [ARGS]\tSet the SLURM's plugstack arguments. I.e:\n");
//	printf("\t\t\t\t--plugstack prefix=/hpc/opt/ear default=on...\n");
	printf("\t--clean\t\t\tRemoves the internal files.\n");
	printf("SLURM options:\n");

	return 0;
}

int pipeline(int argc, char *argv[], int sp, int at)
{
	_sp = sp;
	_at = at;

	if (plug_is_action(_at, Action.init)) {
		slurm_spank_init(_sp, argc, argv);

		if (plug_context_is(_sp, Context.srun))	{
			slurm_spank_init_post_opt(_sp, argc, argv);
		}
		else if (plug_context_is(_sp, Context.remote)) {
			slurm_spank_user_init(_sp, argc, argv);
            // The master calls user_init before any task is calling
            // task_init, because all the other tasks waits the master 
            // to finish its remote pipeline completely. Then there
            // isn't required a task wait here.
            slurm_spank_task_init(_sp, argc, argv);
		}
	}
	else if(plug_is_action(_at, Action.exit)) {
		if (plug_context_is(_sp, Context.remote)) {
            slurm_spank_task_exit(_sp, argc, argv);
            if (sd.erun.is_master) {
                slurm_spank_exit(_sp, argc, argv);
            }
        }
		if (plug_context_is(_sp, Context.srun)) {
			slurm_spank_exit(_sp, argc, argv);
		}
	}

	return ESPANK_SUCCESS;
}

int print_argv(int argc, char *argv[])
{
	int i;
	int j;

	for (i = 0, j = 0; i < argc; ++i) {
		strcpy(&buffer[j], argv[i]);
		j += strlen(argv[i])+1;
		buffer[j-1] = ' ';
	}
	buffer[j] = '\0';
	plug_verbose(_sp, 4, "input: %s", buffer);
	return 0;
}

static void program_parse(char *program, char *args[])
{
    int len = strlen(program);
    int i;
    int j;

    args[0] = program;
    for (i = 0, j = 1; i < len; ++i) {
        if (program[i] == ' ') {
            program[i] = '\0';
            if (program[i+1] != '\0' && program[i+1] != ' ') {
                args[j] = &program[i+1];
                ++j;
            }
        }
    }
    args[j] = NULL;

    plug_verbose(_sp, 4, "ERUN --program decomposition:");
    for (i = 0; args[i] != NULL; ++i) {
        plug_verbose(_sp, 4, "%d: %s", i, args[i]);
    }
}

static char *getenv_PBS_JOBID()
{
    static char job_id[32];
    char *c;
    char *p;
    if (!(p = getenv("PBS_JOBID"))) {
        return NULL;
    }
    if ((c = strchr(p, '[')) != NULL) { *c = '\0'; }
    if ((c = strchr(p, '.')) != NULL) { *c = '\0'; }
    sprintf(job_id, "%s", p);
    return job_id;
}

int job(int argc, char *argv[])
{
	int err_pfx = 1;
	int err_tmp = 1;
	int err_etc = 1;
	int err_def = 1;
	char *p = NULL;
	int i = 0;

	// Its ERUN
	sd.erun.is_erun = 1;
    // Setting ERUN reference
    setenv(Var.is_erun.ear, "1", 1);
    // Enabling plugin and ERUN components
	setenv(Var.comp_plug.comp, "1", 1);

	// Clean
	for (i = 0; i < argc; ++i) {
		if ((strlen(argv[i]) >= 7) && (strncmp("--clean", argv[i], 7) == 0)) {
			_clean = 1;
		}
	}
	// Help?
	for (i = 0; i < argc; ++i) {
		if ((strlen(argv[i]) >= 6) && (strncmp("--help", argv[i], 6) == 0)) {
			_help = 1;
		}
	}
	// Setting the program and the job name
	for (i = 0; i < argc; ++i) {
		if ((strlen(argv[i]) > 8) && (strncmp("--program=", argv[i], 10) == 0)) {
			p = &argv[i][10];
		}
	}

    // Getting JOB_NAME
	if (p != NULL) {
		// Setting the job name
		sprintf(path_app, "%s", p);
        program_parse(path_app, args);
		setenv(Var.name_app.slurm, path_app, 1);
	} else {
		_help = !_clean;
	}

    // Converting configuration enrivonment variables
	// (INSTALL_PATH, ETC, TMP) in input parameters.
	_argc = argc + 4;
	_argv = malloc(sizeof(char **) * _argc);

	_argv[argc + 0] = plug_def;
	_argv[argc + 1] = plug_pfx;
	_argv[argc + 2] = plug_etc;
	_argv[argc + 3] = plug_tmp;
	
	for (i = 0; i < argc; ++i) {
		_argv[i] = argv[i];
	}
	if ((p = getenv("EAR_INSTALL_PATH")) != NULL) {
		sprintf(plug_pfx, "prefix=%s", p);
		err_pfx = 0;
	}
	if ((p = getenv("EAR_ETC")) != NULL) {
		sprintf(plug_etc, "sysconfdir=%s", p);
		err_etc = 0;
	}
	if ((p = getenv("EAR_TMP")) != NULL) {
		sprintf(plug_tmp, "localstatedir=%s", p);
		sprintf(path_tmp, "%s", p);
		err_tmp = 0;
	} else {
		sprintf(path_tmp, "/tmp");
	}
	if ((p = getenv("EAR_DEFAULT")) != NULL) {
		sprintf(plug_def, "default=%s", p);
		err_def = 0;
	} else {
		sprintf(plug_def, "default=on");
		err_def = 0;
	}
	for (i = 0; i < argc; ++i) {
		_argv[i] = argv[i];
	}

    // Getting N_NODES
	char *n_nodes;
	for (i = 0; i < argc; ++i) {
		if ((strlen(argv[i]) > 7) && (strncmp("--nodes=", argv[i], 8) == 0)) {
			setenv(Var.job_node_count.slurm, &argv[i][8], 1);
		}
	}
    // OpenMPI version of N_NODES
	if ((n_nodes = getenv(Var.job_node_count.slurm)) == NULL) {
		char *size_world = getenv("OMPI_COMM_WORLD_SIZE");
		char *size_local = getenv("OMPI_COMM_WORLD_LOCAL_SIZE");
		char buffer[8];

		if (size_world != NULL && size_local != NULL) {
			i = atoi(size_world) / atoi(size_local);
			sprintf(buffer, "%d", i);
			n_nodes = buffer;
		}
	}
	if (n_nodes != NULL) {
		setenv(Var.step_node_count.slurm, n_nodes, 1);
	}

    // Getting JOB_ID
    char *job_id;
    char *step_id;
    // Patching other job managers
    if (!(job_id = getenv(Var.job_id.slurm))) {
        if (!(job_id = getenv("OAR_JOB_ID")))
            if (!(job_id = getenv_PBS_JOBID()))
                if (!(job_id)) job_id = "0";
        // Setting the SLURM version of the variable
        setenv(Var.job_id.slurm, job_id, 1);
    }
    if ((step_id = getenv(Var.step_id.slurm)) == NULL) {
        if (step_id == NULL) step_id = "0";
        // Setting the SLURM version of the variable
        setenv(Var.step_id.slurm, step_id, 1);
    } else {
        sd.erun.is_step_id = 1;
    }
    // Saving variables
    sd.erun.job_id  = atoi(job_id);
    sd.erun.step_id = atoi(step_id);

	// Input parameters final
	print_argv(_argc, _argv);
	
	// Going inactive?
	_inactive = isenv_agnostic(_sp, Var.was_srun.mod, "1");

	if (_inactive) {
		_error = 2;
	}
	if (err_pfx | err_etc | err_tmp | err_def) {
		_error = 1;
	}
	return 0;
}

int step(int argc, char *argv[])
{
    // If STEP_ID wasn't read
    if (!sd.erun.is_step_id) {
        sprintf(buffer, "%d", sd.erun.step_id);
        setenv("SLURM_STEP_ID", buffer, 1);
    }
    // Currently, SLURM_LOCALID is only used to determine the master node. In
    // case its value gains new uses, a lock would have to be used to numerate
    // each local task id.
    if (getenv(Var.local_id.slurm) == NULL) {
        if (sd.erun.is_master) {
            setenv(Var.local_id.slurm, "0", 1);
        } else {
            setenv(Var.local_id.slurm, "1", 1);
        }
    }
	plug_verbose(_sp, 2, "program: '%s'", path_app);
	plug_verbose(_sp, 2, "job/step id: '%d/%d'", sd.erun.job_id, sd.erun.step_id);

	return 0;
}

static int execute(int argc, char *argv[])
{
	pid_t fpid;
	//
	fpid = fork();

	if (fpid == 0)
	{
		// Setting SLURMs task pid
		sprintf(buffer, "%d", getpid());
		setenv(Var.task_pid.ear, buffer, 1);

		// Executting
		if (execvp(args[0], args) == -1) {
			plug_verbose(_sp, 0, "failed to run the program (%s: %s)", args[0], strerror(errno));
			exit(0);
		}
		// No return
	} else if (fpid == -1) {
		plug_verbose(_sp, 0, "failed to run the program (fork)");
	} else {
		waitpid(fpid, &sd.subject.exit_status, 0);
		plug_verbose(_sp, 2, "program returned with status %d",
			sd.subject.exit_status);
	}	

	return 0;
}

int main(int argc, char *argv[])
{
	// Creating job and reading arguments
	job(argc, argv);

    // Signals initialization
    signals();

	// Clean
	if (_clean) {
        // Cleaning $EAR_TMP/erun folder
	    plug_verbose(_sp, 0, "cleaning erun folder");
		all_clean(path_tmp);
       
         return 0;
	}

	// Help
	if (_help) {
		help(_argc, _argv);
		// No context initializations
		pipeline(_argc, _argv, Context.error, Action.init);
		// no context finalization
		pipeline(_argc, _argv, Context.error, Action.exit);

		return 0;
	}

    // Error pipeline
	if (_error) {
		if (_error == 1) { // Environment variable missing
			if ((sd.erun.is_master = lock_master(path_tmp, sd.erun.job_id))) {
				plug_verbose(_sp, 0, "missing environment vars, is the EAR module loaded? (only master prints this message)");
				//
				sleep(2);
				//
				folder_clean(sd.erun.job_id);
			}
			return 0;	
		}
		if (_error == 2) { // Going inactive because detected SRUN
			if ((sd.erun.is_master = lock_master(path_tmp, sd.erun.job_id))) {
				plug_verbose(_sp, 0, "detected SRUN, going inactive (only master prints this message)");	
			}
			//	
			execute(_argc, _argv);
			//
			if (sd.erun.is_master) {	
				folder_clean(sd.erun.job_id);
			}
			return 0;	
		}
	} // _error

    // Is master?
    switch ((sd.erun.is_master = lock_master(path_tmp, sd.erun.job_id))) {
        case 1: // Case master
            plug_verbose(_sp, 2, "subject '%d' is erun master? '%d' (got the lock file)",
                         getpid(), sd.erun.is_master);
            // Read old step id in master.step.id
            sd.erun.step_id = master_getstep(sd.erun.job_id, sd.erun.step_id);
            break;
        case 0: // Case not master
            plug_verbose(_sp, 3, "subject '%d' is erun master? '%d' (missed the lock file and then spinlock)",
                         getpid(), sd.erun.is_master);
            // Spinlock over lock.slave
            spinlock_slave(sd.erun.job_id);
            // Get slave.step.id
            sd.erun.step_id = slave_getstep(sd.erun.job_id, sd.erun.step_id);
            break;
        case -1: // Case some error with lock files
            execute(_argc, _argv);
            return 0;
    }

	// Creating step
	step(_argc, _argv);
	// Local context initialization
	pipeline(_argc, _argv, Context.srun, Action.init);
	// Remote context initialization
	pipeline(_argc, _argv, Context.remote, Action.init);

	if (sd.erun.is_master) {
		// Free lock.step
		unlock_slave(sd.erun.job_id, sd.erun.step_id);
	}

	execute(_argc, _argv);

	if (sd.erun.is_master) {
		sleep(1);
	}

	// Remote context finalization
	pipeline(_argc, _argv, Context.remote, Action.exit);
	// Local context finalization
	pipeline(_argc, _argv, Context.srun, Action.exit);

	if (sd.erun.is_master) {
		files_clean(sd.erun.job_id, sd.erun.step_id);
	}

	return 0;
}
