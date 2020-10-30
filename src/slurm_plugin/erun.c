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

#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>
#include <slurm_plugin/erun_lock.h>

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
char **_argv;
int    _argc;

//
char *_errstr;

//
int _inactive;
int _error;
int _clean;
int _help;

//
int _master;
int _step;
int _job;
int _sp;
int _at;

int plug_is_action(int _ac, int action)
{
	return _ac == action;
}

int help(int argc, char *argv[])
{
	printf("Usage: %s [OPTIONS]\n", argv[0]);
	printf("\nOptions:\n");
	printf("\t--job-id=<arg>\t\tSet the JOB_ID.\n");
	printf("\t--nodes=<arg>\t\tSets the number of nodes.\n");
	printf("\t--program=<arg>\t\tSets the program to run.\n");
	printf("\t--plugstack [ARGS]\tSet the SLURM's plugstack arguments. I.e:\n");
	printf("\t\t\t\t--plugstack prefix=/hpc/opt/ear default=on...\n");
	printf("\t--clean\t\t\tRemoves the internal files.\n");
	printf("SLURM options:\n");

	return 0;
}

int clean(int argc, char *argv[])
{
	plug_verbose(_sp, 0, "cleaning temporal folder");
	//
	sprintf(path_sys, "rm %s/erun.*.lock &> /dev/null", path_tmp);
	//
	system(path_sys);
	
	return 0;
}

int fake_slurm_spank_user_init(spank_t sp, int ac, char **av)
{
	int plug_shared_readsetts(spank_t sp, plug_serialization_t *sd);
	plug_verbose(sp, 2, "function fake_slurm_spank_user_init");

	//
	plug_deserialize_remote(sp, &sd);
	
	if (!plug_component_isenabled(sp, Component.plugin)) {
		return ESPANK_SUCCESS;
	}

	//
	if (sd.subject.context_local == Context.srun)
	{
		if (plug_component_isenabled(sp, Component.library))
		{
			plug_shared_readsetts(sp, &sd);

			plug_serialize_task(sp, &sd);
		}
	}
	
	return ESPANK_SUCCESS;
}

int pipeline(int argc, char *argv[], int sp, int at)
{
	_sp = sp;
	_at = at;

	if (plug_is_action(_at, Action.init))
	{
		slurm_spank_init(_sp, argc, argv);

		if (plug_context_is(_sp, Context.srun))
		{
			slurm_spank_init_post_opt(_sp, argc, argv);
		}
		else if (plug_context_is(_sp, Context.remote))
		{
			if (_master) {
				slurm_spank_user_init(_sp, argc, argv);
			} else {
				fake_slurm_spank_user_init(_sp, argc, argv);
			}

		}
	}
	else if(plug_is_action(_at, Action.exit))
	{
		if (plug_context_is(_sp, Context.remote))
		{
			slurm_spank_task_exit(_sp, argc, argv);

			if (_master) {
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

int job(int argc, char *argv[])
{
	int err_pfx = 1;
	int err_tmp = 1;
	int err_etc = 1;
	int err_def = 1;
	char *p = NULL;
	int i = 0;

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

	//
	if (p != NULL) {
		// Setting the job name
		sprintf(path_app, "%s", p);
		setenv("SLURM_JOB_NAME", p, 1);
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

	// Getting the number of nodes
	for (i = 0; i < argc; ++i) {
		if ((strlen(argv[i]) > 7) && (strncmp("--nodes=", argv[i], 8) == 0)) {
			setenv("SLURM_NNODES", &argv[i][8], 1);
		}
	}

	char *nnodes = getenv("SLURM_NNODES");
	
	if (nnodes == NULL)
	{
		char *size_world = getenv("OMPI_COMM_WORLD_SIZE");
		char *size_local = getenv("OMPI_COMM_WORLD_LOCAL_SIZE");
		char nnodes_b[8];
		int  nnodes_i;

		if (size_world != NULL && size_local != NULL) {
			nnodes_i = atoi(size_world) / atoi(size_local);
			sprintf(nnodes_b, "%d", nnodes_i);
			nnodes = nnodes_b;
		}
	}
	
	if (nnodes != NULL) {
		setenv("SLURM_STEP_NUM_NODES", nnodes, 1);
	}

	// Getting the job id (it cant be created,
	// depends on the queue manager:
	if ((p = getenv("SLURM_JOB_ID")) == NULL) {
		setenv("SLURM_JOB_ID", "0", 1);
	} else {
		_job = atoi(p);
	}
	if ((p = getenv("SLURM_STEP_ID")) == NULL) {
		setenv("SLURM_STEP_ID", "0", 1);
	} else {
		_step = atoi(p);
	}

	// Input parameters final
	print_argv(_argc, _argv);
	
	// Going inactive?
	_inactive = isenv_agnostic(_sp, Var.was_srun.rem, "1");

	if (_inactive) {
		_error = 2;
	}
	
	if (err_pfx | err_etc | err_tmp | err_def) {
		_error = 1;
	}

	return 0;
}

int step(int argc, char *argv[], int job_id, int step_id)
{
	sprintf(buffer, "%d", step_id);
	setenv("SLURM_STEP_ID", buffer, 1);
	
	plug_verbose(_sp, 2, "program: '%s'", path_app);
	plug_verbose(_sp, 2, "job/step id: '%d/%d'", job_id, step_id);
	return 0;
}

int execute(int argc, char *argv[])
{
	system(path_app);
	return 0;
}

int main(int argc, char *argv[])
{
	// Creating job and reading arguments
	job(argc, argv);

	// Clean
	if (_clean) {
		return clean(_argc, _argv);
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
	
	if (_error)
	{
		if (_error == 1)
		{
			if ((_master = lock_master(path_tmp)))
			{
				plug_verbose(_sp, 0, "missing environment vars, is the EAR module loaded?");
				//
				sleep(2);
				//
				lock_clean(path_tmp);
			}
			return 0;	
		}
		if (_error == 2) {
			if ((_master = lock_master(path_tmp))) {
				plug_verbose(_sp, 0, "detected SRUN, going inactive");	
			}
		
			execute(_argc, _argv);
		
			if (_master) {	
				lock_clean(path_tmp);
			}
			return 0;	
		}
	}

	// Simulating the single pipeline
	if ((_master = lock_master(path_tmp)))
	{
		plug_verbose(_sp, 2, "got the lock file");

		_step = lock_step(path_tmp, _job, _step);
	} else {
		plug_verbose(_sp, 4, "missed the lock file, spinlock");
		plug_verbosity_silence(_sp);

		//
		_step = spinlock_step(path_tmp, _step);
	}

	// Creating step
	step(_argc, _argv, _job, _step);

	// Local context initialization
	pipeline(_argc, _argv, Context.srun, Action.init);
	// Remote context initialization
	pipeline(_argc, _argv, Context.remote, Action.init);

	if (_master) {
		unlock_step(path_tmp, _step);
	}

	execute(_argc, _argv);

	if (_master) {
		sleep(1);
	}

	// Remote context finalization
	pipeline(_argc, _argv, Context.remote, Action.exit);
	// Local context finalization
	pipeline(_argc, _argv, Context.srun, Action.exit);

	if (_master) {
		lock_clean(path_tmp);
	}

	return 0;
}
