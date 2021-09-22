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

#include <pwd.h>
#include <grp.h>
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
	plug_verbose(sp, 3, "---------- application summary ---");
	plug_verbose(sp, 3, "job/step/name '%lu'/'%lu'/'%s'", app->job.id, app->job.step_id, app->job.app_id);
	plug_verbose(sp, 3, "user/group/acc '%s'/'%s'/'%s'", app->job.user_id, app->job.group_id, app->job.user_acc);
	plug_verbose(sp, 3, "policy/th/freq '%s'/'%f'/'%lu'", app->job.policy, app->job.th, app->job.def_f);
	plug_verbose(sp, 3, "learning/tag '%u'/'%s'", app->is_learning, app->job.energy_tag);
	plug_verbose(sp, 3, "----------------------------------");
	return EAR_SUCCESS;
}

int plug_read_application(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_read_application");
	
	new_job_req_t *app = &sd->job.app;
	ulong *freqs = sd->pack.eard.freqs.freqs;
	int n_freqs = sd->pack.eard.freqs.n_freqs;
	uint32_t uitem;

	memset(app,0,sizeof(new_job_req_t));

	// Gathering variables
	app->is_mpi = plug_component_isenabled(sp, Component.library);
	
	strcpy(app->job.user_id, sd->job.user.user);
	strcpy(app->job.group_id, sd->job.user.group);
	strcpy(app->job.user_acc, sd->job.user.account);
	
	if (spank_get_item (sp, S_JOB_ID, (int *)&uitem) == ESPANK_SUCCESS) {
		app->job.id = (ulong) uitem;
	} else {
		app->job.id = NO_VAL;
	}
	if (spank_get_item (sp, S_JOB_STEPID, (int *)&uitem) == ESPANK_SUCCESS) {
		app->job.step_id = (ulong) uitem;
	} else {
		app->job.step_id = NO_VAL;
	}
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

	plug_verbose(sp, 2, "subject '%s' is master? '%d'",
		sd->subject.host, sd->subject.is_master);
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

	// If no EARD connectiond or not library...
	if (sd->pack.eard.connected && !setts->lib_enabled) {
		return ESPANK_SUCCESS;
	}

	// Default P_STATE, frequency (KHz) and threshold (%).
	snprintf(buffer1, 16, "%u", setts->def_p_state);
	setenv_agnostic(sp, Var.p_state.ear, buffer1, 1);
	snprintf(buffer1, 16, "%lu", setts->def_freq);
	setenv_agnostic(sp, Var.frequency.ear, buffer1, 1);
	snprintf(buffer1, 16, "%0.2f", setts->settings[0]);
	setenv_agnostic(sp, Var.eff_gain.ear, buffer1, 1);
	setenv_agnostic(sp, Var.perf_pen.ear, buffer1, 1);
	
	/*if(policy_id_to_name(setts->policy, buffer1) != EAR_ERROR) {
		setenv_agnostic(sp, Var.policy.ear, buffer1, 1);
	}*/
	// Policy 
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
	
	char *lib_path = REL_PATH_LOAD;
	const int m = SZ_BUFFER_EXTRA;
	int n;
	
	buffer1[0] = '\0';
	buffer2[0] = '\0';

	#define t(max, val) \
		((val + 1) > max) ? max : val + 1

	if (getenv_agnostic(sp, Var.hack_load.hck, buffer2, m)) {
		n = snprintf(      0,       0, "%s", buffer2);
		n = snprintf(buffer1, t(m, n), "%s", buffer2);
	} else {	
		// Copying INSTALL_PATH into buffer and adding library and the folder.
		apenv_agnostic(buffer2, sd->pack.path_inst, 64);
		n = snprintf(      0,       0, "%s/%s", buffer2, lib_path);
		n = snprintf(buffer1, t(m, n), "%s/%s", buffer2, lib_path);
	}
	plug_verbose(sp, 2, "trying to load file '%s'", buffer1);

	// Testing the access to the library folder.
	if (access(buffer1, X_OK) == 0)
	{
		char *ld_buf = sd->job.user.env.ld_preload;

		// If LD_PRELOAD already exists.
		if (getenv_agnostic(sp, Var.ld_prel.ear, ld_buf, m))
		{
			if (n > 0) {
				n = xsprintf(buffer2, "%s:", buffer1);
			}
			// Append the current LD_PRELOAD content to our library.
			n = snprintf(      0,       0, "%s%s", buffer2, ld_buf);
			n = snprintf(buffer1, t(m, n), "%s%s", buffer2, ld_buf);
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
