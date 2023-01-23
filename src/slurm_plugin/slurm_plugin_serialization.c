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

#define _GNU_SOURCE

#include <pwd.h>
#include <grp.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <common/system/file.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// Buffers
static char buffer1[SZ_BUFFER_EXTRA];
static char buffer2[SZ_BUFFER_EXTRA];

/*
 *
 *
 *
 *
 *
 */

int plug_read_plugstack(spank_t sp, int ac, char **av, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_read_plugstack");

	int found_earmgd_port = 0;
	int found_eargmd_host = 0;
	int found_path_inst = 0;
	int found_path_temp = 0;
	int found_exclusion = 0;
	int found_inclusion = 0;
	int i;

	// Initialization values
	sd->pack.eargmd.min = 1;
	// Search
	for (i = 0; i < ac; ++i) {
		if ((strlen(av[i]) > 8) && (strncmp ("default=", av[i], 8) == 0))
		{
			plug_verbose(sp, 2, "plugstack found library by default '%s'", &av[i][8]);
			// If enabled by default
			if (strncmp ("default=on", av[i], 10) == 0) {
				// Library == 1: enable
				// Library == 0: nothing
				// Library == whatever: enable (bug protection)
				if (!isenv_agnostic(sp, Var.comp_libr.cmp, "0")) {
					plug_component_setenabled(sp, Component.library, 1);
					// CAUTION!!
					// Enabling also the variable to reactivate Component.library
					// in remote environments. It may produce unexpected problems.
					setenv_agnostic(sp, Var.comp_libr.cmp, "1", 1);
				}
			// If disabled by default or de administrator have misswritten
			} else {
				// Library == 1: nothing
				// Library == 0: disable
				// Library == whatever: disable (bug protection)
				if (!isenv_agnostic(sp, Var.comp_libr.cmp, "1")) {
					plug_component_setenabled(sp, Component.library, 0);
				}
			}
		}
		if ((strlen(av[i]) > 14) && (strncmp ("localstatedir=", av[i], 14) == 0)) {
			found_path_temp = 1;
			xstrncpy(sd->pack.path_temp, &av[i][14], SZ_NAME_MEDIUM);
			plug_verbose(sp, 2, "plugstack found temporal files in path '%s'", sd->pack.path_temp);
		}
		if ((strlen(av[i]) > 7) && (strncmp ("prefix=", av[i], 7) == 0)) {
			found_path_inst = 1;
			xstrncpy(sd->pack.path_inst, &av[i][7], SZ_NAME_MEDIUM);
			plug_verbose(sp, 2, "plugstack found prefix in path '%s'", sd->pack.path_inst);
		}
		if ((strlen(av[i]) > 12) && (strncmp ("eargmd_host=", av[i], 12) == 0)) {
			found_eargmd_host = 1;
			xstrncpy(sd->pack.eargmd.host, &av[i][12], SZ_NAME_MEDIUM);
			plug_verbose(sp, 2, "plugstack found eargm host '%s'", sd->pack.eargmd.host);
		}
		if ((strlen(av[i]) > 12) && (strncmp ("eargmd_port=", av[i], 12) == 0)) {
			found_earmgd_port = 1;	
			sd->pack.eargmd.port = atoi(&av[i][12]);
			plug_verbose(sp, 2, "plugstack found eargm port '%d'", sd->pack.eargmd.port);
		}
		if ((strlen(av[i]) > 11) && (strncmp ("eargmd_min=", av[i], 11) == 0)) {
			sd->pack.eargmd.min = atoi(&av[i][11]);
			plug_verbose(sp, 2, "plugstack found eargm min '%d'", sd->pack.eargmd.min);
		}
		if ((strlen(av[i]) > 14) && (strncmp ("nodes_allowed=", av[i], 14) == 0)) {
			xstrncpy(sd->pack.nodes_allowed, &av[i][14], SZ_PATH);
			plug_verbose(sp, 2, "plugstack found allowed nodes: '%s'", sd->pack.nodes_allowed);
			found_inclusion = 1;
		}
		if ((strlen(av[i]) > 15) && (strncmp ("nodes_excluded=", av[i], 15) == 0)) {
			xstrncpy(sd->pack.nodes_excluded, &av[i][15], SZ_PATH);
			plug_verbose(sp, 2, "plugstack found excluded nodes: '%s'", sd->pack.nodes_excluded);
			found_exclusion = 1;
		}
	}
	// EARGMD enabled?
	sd->pack.eargmd.enabled = found_eargmd_host && found_earmgd_port;
	// TMP folder missing?
	if (!found_path_temp) {
		plug_verbose(sp, 2, "missing plugstack localstatedir directory");
		return ESPANK_STOP;
	}
	// Prefix folder missing?
	if (!found_path_inst) {
		plug_verbose(sp, 2, "missing plugstack prefix directory");
		return ESPANK_ERROR;
	}
	// Allowance (from plugstack)
	gethostname(sd->subject.host, SZ_NAME_MEDIUM);
	// If is excluded, then it can't continue
	if (found_exclusion) {
		plug_verbose(sp, 2, "testing exclusion of '%s' in '%s'", sd->subject.host, sd->pack.nodes_excluded);
		if (strinlist(sd->pack.nodes_excluded, ",", sd->subject.host)) {
			plug_error(sp, "found node '%s' in the exclusion list", sd->subject.host);
			return ESPANK_ERROR;
		}
	}
	// If is not included/allowed, then it can't continue
	if (found_inclusion) {
		plug_verbose(sp, 2, "testing inclusion of %s in %s", sd->subject.host, sd->pack.nodes_allowed);
		if (!strinlist(sd->pack.nodes_allowed, ",", sd->subject.host)) {
			plug_error(sp, "not found node '%s' in the allowance list", sd->subject.host);
			return ESPANK_ERROR;
		}
	}

	return ESPANK_SUCCESS;
}

static int frequency_exists(spank_t sp, ulong *freqs, int n_freqs, ulong freq)
{
	int i;
	
	plug_verbose(sp, 3, "number of frequencies %d (looking for %lu)", n_freqs, freq);

	for (i = 0; i < n_freqs; ++i) {
		plug_verbose(sp, 3, "freq #%d: %lu", i, freqs[i]);
		if (freqs[i] == freq) {
			return 1;
		}
	}
	return 0;
}

int plug_print_application(spank_t sp, new_job_req_t *app)
{
	plug_verbose(sp, 3, "------------------------------ application summary ---");
	plug_verbose(sp, 3, "job/step/name '%lu'/'%lu'/'%s'", app->job.id, app->job.step_id, app->job.app_id);
	plug_verbose(sp, 3, "user/group/acc '%s'/'%s'/'%s'", app->job.user_id, app->job.group_id, app->job.user_acc);
	plug_verbose(sp, 3, "policy/th/freq '%s'/'%f'/'%lu'", app->job.policy, app->job.th, app->job.def_f);
	plug_verbose(sp, 3, "learning/tag '%u'/'%s'", app->is_learning, app->job.energy_tag);
	plug_verbose(sp, 3, "------------------------------------------------------");
	return EAR_SUCCESS;
}

int plug_read_application(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_read_application");
	
	new_job_req_t *app = &sd->job.app;
	ulong *freqs = sd->pack.eard.freqs.freqs;
	int n_freqs = sd->pack.eard.freqs.n_freqs;

    // Cleaning
	//memset(app, 0, sizeof(new_job_req_t));
	// Gathering variables
	strcpy(app->job.user_id, sd->job.user.user);
	strcpy(app->job.group_id, sd->job.user.group);
	strcpy(app->job.user_acc, sd->job.user.account);
    app->is_mpi = plug_component_isenabled(sp, Component.library);
    // Getenv things
	if (!getenv_agnostic(sp, Var.name_app.rem, app->job.app_id, GENERIC_NAME)) {
		strcpy(app->job.app_id, "");
	}
	if (!getenv_agnostic(sp, Var.policy.ear, app->job.policy, POLICY_NAME)) {
		strcpy(app->job.policy, "");
	}
	if (!getenv_agnostic(sp, Var.policy_th.ear, buffer1, SZ_NAME_SHORT)) {
		app->job.th = -1.0;
	} else {
		app->job.th = atof(buffer1);
	}
	if (!getenv_agnostic(sp, Var.frequency.ear, buffer1, SZ_NAME_MEDIUM)) {
		app->job.def_f = 0;
	} else {
		app->job.def_f = (ulong) atol(buffer1);
		if (!frequency_exists(sp, freqs, n_freqs, app->job.def_f)) {
			app->job.def_f = 0;
		}
	}
	if (!getenv_agnostic(sp, Var.learning.ear, buffer1, SZ_NAME_MEDIUM)) {
		app->is_learning = 0;
	} else {
		if ((unsigned int) atoi(buffer1) < n_freqs) {
			app->job.def_f = freqs[atoi(buffer1)];
			app->is_learning = 1;
		}
	}
	if (!getenv_agnostic(sp, Var.tag.ear, app->job.energy_tag, ENERGY_TAG_SIZE)) {
		strcpy(app->job.energy_tag, "");
	}
	
    plug_print_application(sp, app);

	return ESPANK_SUCCESS;
}

/*
 *
 *
 * Serialize/deserialize
 *
 *
 */

int plug_print_variables(spank_t sp)
{
	if (plug_verbosity_test(sp, 4) != 1) {
		return ESPANK_SUCCESS;
	}

	printenv_agnostic(sp, Component.plugin);
	printenv_agnostic(sp, Component.library);
	printenv_agnostic(sp, Var.comp_libr.cmp);
	printenv_agnostic(sp, Var.comp_plug.cmp);
	printenv_agnostic(sp, Var.comp_verb.cmp);
	printenv_agnostic(sp, Var.hack_load.hck);
	printenv_agnostic(sp, Var.verbose.loc);
	printenv_agnostic(sp, Var.policy.loc);
	printenv_agnostic(sp, Var.policy_th.loc);
	printenv_agnostic(sp, Var.frequency.loc);
	printenv_agnostic(sp, Var.p_state.loc);
	printenv_agnostic(sp, Var.learning.loc);
	printenv_agnostic(sp, Var.tag.loc);
	printenv_agnostic(sp, Var.path_usdb.loc);
	printenv_agnostic(sp, Var.cpus_nodn.rem);
	//printenv_agnostic(sp, Var.path_trac.loc);
	printenv_agnostic(sp, Var.version.loc);
	//printenv_agnostic(sp, Var.gm_host.loc);
	//printenv_agnostic(sp, Var.gm_port.loc);
	printenv_agnostic(sp, Var.gm_secure.loc);
	printenv_agnostic(sp, Var.user.rem);
	printenv_agnostic(sp, Var.group.rem);
	//printenv_agnostic(sp, Var.path_temp.rem);
	//printenv_agnostic(sp, Var.path_inst.rem);
	//printenv_agnostic(sp, Var.nodes_allowed.rem);
	//printenv_agnostic(sp, Var.nodes_excluded.rem);
	printenv_agnostic(sp, Var.ctx_last.rem);
	printenv_agnostic(sp, Var.was_sbac.rem);
	printenv_agnostic(sp, Var.was_srun.rem);
	printenv_agnostic(sp, Var.node_num.loc);
	printenv_agnostic(sp, Var.name_app.rem);
	printenv_agnostic(sp, Var.account.rem);
	printenv_agnostic(sp, Var.job_nodl.rem);
	printenv_agnostic(sp, Var.job_nodn.rem);
	printenv_agnostic(sp, Var.step_nodl.rem);
	printenv_agnostic(sp, Var.step_nodn.rem);
	printenv_agnostic(sp, Var.verbose.ear);
	printenv_agnostic(sp, Var.policy.ear);
	printenv_agnostic(sp, Var.policy_th.ear);
	printenv_agnostic(sp, Var.frequency.ear);
	printenv_agnostic(sp, Var.p_state.ear);
	printenv_agnostic(sp, Var.learning.ear);
	printenv_agnostic(sp, Var.tag.ear);
	printenv_agnostic(sp, Var.path_usdb.ear);
	printenv_agnostic(sp, Var.path_trac.ear);
	printenv_agnostic(sp, Var.perf_pen.ear);
	printenv_agnostic(sp, Var.eff_gain.ear);
	printenv_agnostic(sp, Var.name_app.ear);
	printenv_agnostic(sp, Var.path_temp.ear);
	printenv_agnostic(sp, Var.ld_prel.ear);
	printenv_agnostic(sp, Var.ld_libr.ear);
    printenv_agnostic(sp, "SLURM_JOBID");
    printenv_agnostic(sp, "SLURM_JOB_ID");
    printenv_agnostic(sp, "SLURM_STEPID");
    printenv_agnostic(sp, "SLURM_STEP_ID");
    printenv_agnostic(sp, "SLURM_LOCALID");

	return ESPANK_SUCCESS;
}

int plug_print_items(spank_t sp)
{
	plug_verbose(sp, 2, "function plug_print_items");
    
    if (plug_verbosity_test(sp, 4) != 1) {
        return ESPANK_SUCCESS;
    }

    #if ERUN
    #define print_item(word, cast, item, format)
    #else
    #define print_item(word, cast, item, format) \
        if (spank_get_item (sp, word, cast &(item)) == ESPANK_SUCCESS) { \
            plug_verbose(sp, 2, #word " = '" format "'", item); \
        }
    #endif
		#if !ERUN
    char  *itemStr;
    ullong item64u;
    uint   item32u;
		#endif

    print_item(S_JOB_UID             ,    (uid_t *), item32u, "%d");
    print_item(S_JOB_GID             ,    (gid_t *), item32u, "%d");
    print_item(S_JOB_ID              , (uint32_t *), item32u, "%u");
    print_item(S_JOB_STEPID          , (uint32_t *), item32u, "%u");
    print_item(S_JOB_NNODES          , (uint32_t *), item32u, "%u");
    print_item(S_JOB_NODEID          , (uint32_t *), item32u, "%u");
    print_item(S_JOB_LOCAL_TASK_COUNT, (uint32_t *), item32u, "%u");
    print_item(S_JOB_TOTAL_TASK_COUNT, (uint32_t *), item32u, "%u");
    print_item(S_JOB_NCPUS           , (uint16_t *), item32u, "%hu");
    print_item(S_TASK_ID             ,      (int *), item32u, "%d");
    print_item(S_TASK_GLOBAL_ID      , (uint32_t *), item32u, "%u");
    print_item(S_TASK_PID            ,    (pid_t *), item32u, "%u");
    print_item(S_SLURM_VERSION       ,    (char **), itemStr, "%s");
    print_item(S_SLURM_VERSION_MAJOR ,    (char **), itemStr, "%s");
    print_item(S_SLURM_VERSION_MINOR ,    (char **), itemStr, "%s");
    print_item(S_SLURM_VERSION_MICRO ,    (char **), itemStr, "%s");
    print_item(S_STEP_CPUS_PER_TASK  , (uint32_t *), item32u, "%u");
    print_item(S_JOB_ALLOC_CORES     ,    (char **), itemStr, "%s");
    print_item(S_JOB_ALLOC_MEM       , (uint64_t *), item64u, "%llu");
    print_item(S_SLURM_RESTART_COUNT , (uint32_t *), item32u, "%u");
    
    #if ERUN
    #define print_item2(word, item1, cast2, item2, format)
    #else
    #define print_item2(word, item1, cast2, item2, format) \
        if (spank_get_item (sp, word, item1, cast2 &(item2)) == ESPANK_SUCCESS) { \
            plug_verbose(sp, 2, #word " = '" format "'", item2); \
        }
    #endif

    print_item2(S_JOB_PID_TO_GLOBAL_ID, getpid(), (uint32_t *), item32u, "%u");    
    print_item2(S_JOB_PID_TO_LOCAL_ID , getpid(), (uint32_t *), item32u, "%u");    
    
    // Unused:
    //  - S_JOB_ARGV
    //  - S_TASK_EXIT_STATUS
    //  - S_JOB_LOCAL_TO_GLOBAL_ID
    //  - S_JOB_GLOBAL_TO_LOCAL_ID
    //  - S_JOB_SUPPLEMENTARY_GIDS
    
	#if 0
    int num_args;
    char **args;
	
    if (spank_get_item (sp, S_JOB_ARGV, (int *) &num_args, (char ***) &args) == ESPANK_SUCCESS) {
        uint i;
		for (i = 0; i < num_args; i++){
			plug_verbose(sp, 2, "Arg[%d] = %s", i, args[i]);
		}
	}
	#endif
 
    return ESPANK_SUCCESS;
}

int plug_deserialize_components(spank_t sp)
{
	plug_component_setenabled(sp, Component.plugin, 0);
	plug_component_setenabled(sp, Component.library, 0);

	// Components
	if (!isenv_agnostic(sp, Var.comp_plug.cmp, "0")) {
		plug_component_setenabled(sp, Component.plugin, 1);
	}
	if (isenv_agnostic(sp, Var.comp_libr.cmp, "1")) {
		plug_component_setenabled(sp, Component.library, 1);
	}
	// Return
	if (!plug_component_isenabled(sp, Component.plugin)) {
		return ESPANK_ERROR;
	}

	return ESPANK_SUCCESS;
}

int plug_deserialize_local(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_deserialize_local");

	// Silence
	VERB_SET_EN(0);
	ERROR_SET_EN(0);

	// Cleaning, we don't want inheritance of this variables
	unsetenv_agnostic(sp, Var.verbose.ear);
	unsetenv_agnostic(sp, Var.policy.ear);
	unsetenv_agnostic(sp, Var.policy_th.ear);
	unsetenv_agnostic(sp, Var.perf_pen.ear);
	unsetenv_agnostic(sp, Var.eff_gain.ear);
	unsetenv_agnostic(sp, Var.frequency.ear);
	unsetenv_agnostic(sp, Var.p_state.ear);
	unsetenv_agnostic(sp, Var.learning.ear);
	unsetenv_agnostic(sp, Var.tag.ear);
	unsetenv_agnostic(sp, Var.path_usdb.ear);
	//unsetenv_agnostic(sp, Var.path_trac.ear);
	unsetenv_agnostic(sp, Var.name_app.ear);

	// User information
	uid_t uid = geteuid();
	gid_t gid = getgid();
	struct passwd *upw = getpwuid(uid);
	struct group  *gpw = getgrgid(gid);

	if (upw == NULL || gpw == NULL) {
		plug_verbose(sp, 2, "converting UID/GID in username");
		return ESPANK_ERROR;
	}

	xstrncpy(sd->job.user.user,  upw->pw_name, SZ_NAME_MEDIUM);
	xstrncpy(sd->job.user.group, gpw->gr_name, SZ_NAME_MEDIUM);
	plug_verbose(sp, 2, "user '%u' ('%s')", uid, sd->job.user.user);
	plug_verbose(sp, 2, "user group '%u' ('%s')", gid, sd->job.user.group);

	// Subject
	gethostname(sd->subject.host, SZ_NAME_MEDIUM);

	return ESPANK_SUCCESS;
}

int plug_serialize_remote(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_serialize_remote");

	// User
	setenv_agnostic(sp, Var.user.rem,  sd->job.user.user,  1);
	setenv_agnostic(sp, Var.group.rem, sd->job.user.group, 1);
	
	// Subject
	if (plug_context_is(sp, Context.srun)) {
		setenv_agnostic(sp, Var.ctx_last.rem, Constring.srun, 1);
		setenv_agnostic(sp, Var.was_srun.rem, "1", 1);
	} else
	if (plug_context_is(sp, Context.sbatch)) {
		setenv_agnostic(sp, Var.ctx_last.rem, Constring.sbatch, 1);
		setenv_agnostic(sp, Var.was_sbac.rem, "1", 1);
	}
	
	return ESPANK_SUCCESS;
}

int plug_deserialize_remote(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_deserialize_remote");
	int s1, s2;

	// Silence (not for the plugin, for other EAR APIs)
	VERB_SET_EN(0);
	ERROR_SET_EN(0);

	// Converting option variables to task directly
	repenv_agnostic(sp, Var.verbose.loc,   Var.verbose.ear);
	repenv_agnostic(sp, Var.policy.loc,    Var.policy.ear);
	repenv_agnostic(sp, Var.policy_th.loc, Var.policy_th.ear);
	repenv_agnostic(sp, Var.frequency.loc, Var.frequency.ear);
	repenv_agnostic(sp, Var.learning.loc,  Var.learning.ear);
	repenv_agnostic(sp, Var.tag.loc,       Var.tag.ear);
	repenv_agnostic(sp, Var.path_usdb.loc, Var.path_usdb.ear);
	//repenv_agnostic(sp, Var.path_trac.loc, Var.path_trac.ear);

	// User
	getenv_agnostic(sp, Var.user.rem,      sd->job.user.user,    SZ_NAME_MEDIUM);
	getenv_agnostic(sp, Var.group.rem,     sd->job.user.group,   SZ_NAME_MEDIUM);
	getenv_agnostic(sp, Var.account.rem,   sd->job.user.account, SZ_NAME_MEDIUM);

	// Subject
	if (isenv_agnostic(sp, Var.ctx_last.rem, Constring.srun)) {
		sd->subject.context_local = Context.srun;
	} else if (isenv_agnostic(sp, Var.ctx_last.rem, Constring.sbatch)) {
		sd->subject.context_local = Context.sbatch;
	} else {
		sd->subject.context_local = Context.error;
	}
	if (sd->subject.context_local == Context.error) {
		return ESPANK_ERROR;
	}

    // Job/step
    sd->job.app.job.step_id = (ulong) BATCH_STEP;
    sd->job.app.job.id      = (ulong) BATCH_STEP;
	
    if (spank_get_item (sp, S_JOB_ID, (int *) &s1) == ESPANK_SUCCESS) {
        sd->job.app.job.id = (ulong) s1;
    }
    if (spank_get_item (sp, S_JOB_STEPID, (int *) &s1) == ESPANK_SUCCESS) {
        sd->job.app.job.step_id = (ulong) s1;
    }

	// Job/step list of nodes
	s1 = 0;
	s2 = 0;

	if (plug_context_was(sd, Context.sbatch)) {
		s1 = getenv_agnostic(sp, Var.job_nodl.rem , buffer1, SZ_BUFFER_EXTRA);
		s2 = getenv_agnostic(sp, Var.job_nodn.rem , buffer2, SZ_BUFFER_EXTRA);
	} else if (plug_context_was(sd, Context.srun)) {
		s1 = getenv_agnostic(sp, Var.step_nodl.rem, buffer1, SZ_BUFFER_EXTRA);
		s2 = getenv_agnostic(sp, Var.step_nodn.rem, buffer2, SZ_BUFFER_EXTRA);
	}
	if (s1 && s2) {
		sd->job.nodes_count = atoi(buffer2);
	}

	// Master
	hostlist_t hostlist;
	int from_sbatch;
	int from_srun;
	char *node;
	int i = 0;

	// "Context was" means if local was.
	from_sbatch = plug_context_was(sd, Context.sbatch);
	from_srun   = plug_context_was(sd, Context.srun);
	sd->subject.is_master = from_sbatch;

	// If the node list and node count were present
	if (s1 && s2) {
		hostlist = slurm_hostlist_create(buffer1);
		node = slurm_hostlist_shift(hostlist);
		// If SRUN, just master is required
		if (from_srun && node != NULL) {
			s1 = strlen(sd->subject.host);
			s2 = strlen(node);
			// Taking the smallest
			if (s1 < s2) {
				s2 = s1;
			}
			// If this node is the master
			sd->subject.is_master = (strncmp(sd->subject.host, node, s2) == 0);
			plug_verbose(sp, 2, "first node from list of nodes '%s'", node);
		}
		// If SBATCH/SALLOC then build a node list
		if (from_sbatch && node != NULL) {
			sd->job.nodes_list = calloc(sd->job.nodes_count, sizeof(char *));
			while(node != NULL) {
				sd->job.nodes_list[i] = calloc(1, (size_t) (strlen(node) + 1));
				strcpy(sd->job.nodes_list[i], node);
				node = slurm_hostlist_shift(hostlist);
				++i;
			}
			if (i != sd->job.nodes_count) {
				sd->job.nodes_count = i;
			}
		}
	}

	plug_verbose(sp, 2, "subject '%s' is node master? '%d'",
		sd->subject.host, sd->subject.is_master);

    // CPUs 
    if (getenv_agnostic(sp, Var.cpus_nodn.rem, buffer1, SZ_BUFFER_EXTRA)) {
        sd->job.app.job.procs = atoi(buffer1);
    }
    plug_verbose(sp, 2, "%lu cpus detected on plugin", sd->job.app.job.procs);

	/*
	 * The .ear variables are set during the APP serialization. But APP
	 * serialization happens if the library is ON. But now we have ERUN.
	 * With ERUN in mind we need a small set of variables definitions
	 * out of APP serialization.
	 */
	setenv_agnostic(sp, Var.path_temp.ear, sd->pack.path_temp, 1);
	setenv_agnostic(sp, Var.path_inst.ear, sd->pack.path_inst, 1);

	// Cleaning
	unsetenv_agnostic(sp, Var.user.rem);
	unsetenv_agnostic(sp, Var.group.rem);
	
	// Last minute hack, if energy tag is present, disable library
	if (exenv_agnostic(sp, Var.tag.loc)) {
		plug_component_setenabled(sp, Component.library, 0);
	}

	// Last minute hack, if energy tag is present, disable library
	if (exenv_agnostic(sp, Var.tag.loc)) {
		plug_component_setenabled(sp, Component.library, 0);
	}

	return ESPANK_SUCCESS;
}

int plug_serialize_task_settings(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_serialize_task_settings");
	settings_conf_t *setts = &sd->pack.eard.setts;

	// Variable EAR_ENERGY_TAG, unset
	if (setts->user_type != ENERGY_TAG) {
		unsetenv_agnostic(sp, Var.tag.ear);
	}

	// Default P_STATE, frequency (KHz) and threshold (%).
	snprintf(buffer1, 16, "%u", setts->def_p_state);
	setenv_agnostic(sp, Var.p_state.ear, buffer1, 1);
	
	snprintf(buffer1, 16, "%lu", setts->def_freq);
	setenv_agnostic(sp, Var.frequency.ear, buffer1, 1);
	
	snprintf(buffer1, 16, "%0.2f", setts->settings[0]);
	setenv_agnostic(sp, Var.eff_gain.ear, buffer1, 1);
	setenv_agnostic(sp, Var.perf_pen.ear, buffer1, 1);

	// Policy 
	/*if(policy_id_to_name(setts->policy, buffer1) != EAR_ERROR) {
		setenv_agnostic(sp, Var.policy.ear, buffer1, 1);
	}*/
	if (strlen(setts->policy_name) > 0) {
		setenv_agnostic(sp, Var.policy.ear, setts->policy_name, 1);
	}

	// If is not learning, P_STATE is canceled.
	if(!setts->learning) {
		unsetenv_agnostic(sp, Var.p_state.ear);
		unsetenv_agnostic(sp, Var.learning.ear);
	}
		
	return ESPANK_SUCCESS;
}

int plug_serialize_task_preload(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_serialize_task_preload");
	
	const int bsize = SZ_BUFFER_EXTRA;
	buffer1[0] = '\0';
	buffer2[0] = '\0';

	if (getenv_agnostic(sp, Var.hack_load.hck, buffer2, bsize)) {
		xsprintf(buffer1, "%s", buffer2);
	} else {	
		// Copying INSTALL_PATH into buffer and adding library and the folder.
		apenv_agnostic(buffer2, sd->pack.path_inst, bsize);
		xsprintf(buffer1, "%s/%s", buffer2, REL_PATH_LOAD);
	}
	plug_verbose(sp, 2, "trying to load file '%s'", buffer1);

	// Testing the access to the library folder.
	if (access(buffer1, X_OK) == 0)
	{
		char *ld_buf = sd->job.user.env.ld_preload;
		// If LD_PRELOAD already exists.
		if (getenv_agnostic(sp, Var.ld_prel.ear, ld_buf, bsize))
		{
			xsprintf(buffer2, "%s:", buffer1);
			// Append the current LD_PRELOAD content to our library.
			xsprintf(buffer1, "%s%s", buffer2, ld_buf);
		}
		// Setting LD_PRELOAD environment variable.
		setenv_agnostic(sp, Var.ld_prel.ear, buffer1, 1);
	} else {
		plug_verbose(sp, 2, "this file can't be loaded '%s' (%d, %s)",
			buffer1, errno, strerror(errno));
	}

	// These environment variables are required if loader is loaded.	
	if (getenv_agnostic(sp, Var.name_app.rem, buffer1, sizeof(buffer1)) == 1) {
		setenv_agnostic(sp, Var.name_app.ear, buffer1, 1);
	}

	return ESPANK_SUCCESS;
}

int plug_deserialize_task(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_deserialize_task");
    int cpu_set;
    int task;
    int i, j;

    // By default is error
    cpu_set = -1;
    task    = -1;
    // Local task
	if (spank_get_item(sp, S_TASK_ID, &task) == ESPANK_ERROR) {
        if (getenv_agnostic(sp, "SLURM_LOCALID", buffer2, 32)) {
            task = atoi(buffer2);
        }
    }
    // Getting task mastery
    sd->subject.is_task_master = (task == 0);
    // Getting mask and number of processors
    CPU_ZERO(&sd->job.task.mask);
    
    sd->job.task.jid = sd->job.app.job.id;
    sd->job.task.sid = sd->job.app.job.step_id;
    sd->job.task.pid = getpid();
    
    // Below code doesn't work unless the SLURM user has explicitey set the number of CPUs per task.
	spank_get_item(sp, S_STEP_CPUS_PER_TASK, (int *)&sd->job.task.num_cpus);

    #if !ERUN
    #define SPACE "                            "
    #else
    #define SPACE "            "
    #endif
    xsprintf(buffer1, "task summary:\n");
    xsprintf(buffer2, "%s" SPACE "------------------------------------------------------\n", buffer1);
	xsprintf(buffer1, "%s" SPACE "jid.sid.tid.pid: %lu.%lu.%d.%d\n", buffer2,
        sd->job.task.jid, sd->job.task.sid, task, sd->job.task.pid);
    xsprintf(buffer2, "%s" SPACE "affinity mask  : ", buffer1);

    // Iterating over the mask
    if (sched_getaffinity(0, sizeof(cpu_set_t), &sd->job.task.mask) == -1) {
        xsprintf(buffer1, "%sERROR\n", buffer2);
    } else {
        for (i = get_nprocs()-1, j = 1; i >= 0; --i, ++j)
        {
            cpu_set = CPU_ISSET(i, &sd->job.task.mask);
            strcpy(buffer1, buffer2);
            
            if ((j % 8) == 0) {
                xsprintf(buffer2, "%s%d ", buffer1, cpu_set);
            } else {
                xsprintf(buffer2, "%s%d", buffer1, cpu_set);
            }
        }
        xsprintf(buffer1, "%s\n", buffer2);
    }
    xsprintf(buffer2, "%s" SPACE "cpus per task  : %d\n", buffer1, sd->job.task.num_cpus);
	xsprintf(buffer1, "%s" SPACE "------------------------------------------------------", buffer2);
    plug_verbose(sp, 3, "%s", buffer1);

	return ESPANK_SUCCESS;
}
