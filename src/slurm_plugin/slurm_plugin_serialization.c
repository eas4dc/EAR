/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#include <pwd.h>
#include <grp.h>
#include <common/system/file.h>
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

// Buffers
static char buffer1[SZ_BUFF_EXTRA];
static char buffer2[SZ_BUFF_EXTRA];

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
	int found_eargmd_minn = 0;
	int found_path_inst = 0;
	int found_path_temp = 0;
	int i;

	// Initialization values
	sd->pack.eargmd.min = 1;

	// Search
	for (i = 0; i < ac; ++i)
	{
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
		if ((strlen(av[i]) > 14) && (strncmp ("localstatedir=", av[i], 14) == 0))
		{
			strncpy(sd->pack.path_temp, &av[i][14], SZ_NAME_MEDIUM);
	
			found_path_temp = 1;

			plug_verbose(sp, 2, "plugstack found temporal files in path '%s'", sd->pack.path_temp);
		}
		if ((strlen(av[i]) > 7) && (strncmp ("prefix=", av[i], 7) == 0))
		{
			strncpy(sd->pack.path_inst, &av[i][7], SZ_NAME_MEDIUM);
			found_path_inst = 1;

			plug_verbose(sp, 2, "plugstack found prefix in path '%s'", sd->pack.path_inst);
		}
		if ((strlen(av[i]) > 12) && (strncmp ("eargmd_host=", av[i], 12) == 0))
		{
			strncpy(sd->pack.eargmd.host, &av[i][12], SZ_NAME_MEDIUM);
			found_eargmd_host = 1;
			
			plug_verbose(sp, 2, "plugstack found eargm host '%s'", sd->pack.eargmd.host);
		}
		if ((strlen(av[i]) > 12) && (strncmp ("eargmd_port=", av[i], 12) == 0))
		{
			sd->pack.eargmd.port = atoi(&av[i][12]);
			found_earmgd_port = 1;
			
			plug_verbose(sp, 2, "plugstack found eargm port '%d'", sd->pack.eargmd.port);
		}
		if ((strlen(av[i]) > 11) && (strncmp ("eargmd_min=", av[i], 11) == 0))
		{
			sd->pack.eargmd.min = atoi(&av[i][11]);
			
			plug_verbose(sp, 2, "plugstack found eargm min '%d'", sd->pack.eargmd.min);
		}
	}

	// EARGMD enabled?
	sd->pack.eargmd.enabled = found_eargmd_host && found_earmgd_port;

	// TMP folder missing?
	if (!found_path_temp) {
		plug_verbose(sp, 2, "missing plugstack localstatedir directory");
		return (ESPANK_STOP);
	}

	// Prefix folder missing?
	if (!found_path_inst) {
		plug_verbose(sp, 2, "missing plugstack prefix directory");
		return (ESPANK_ERROR);
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

int plug_print_application(spank_t sp, application_t *app)
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
	
	application_t *app = &sd->job.app;
	ulong *freqs = sd->pack.eard.freqs.freqs;
	int n_freqs = sd->pack.eard.freqs.n_freqs;
	int item;

	init_application(app);

	// Gathering variables
	app->is_mpi = plug_component_isenabled(sp, Component.library);
	
	strcpy(app->job.user_id, sd->job.user.user);
	strcpy(app->job.group_id, sd->job.user.group);
	strcpy(app->job.user_acc, sd->job.user.account);
	
	if (spank_get_item (sp, S_JOB_ID, &item) == ESPANK_SUCCESS) {
		app->job.id = (ulong) item;
	} else {
		app->job.id = NO_VAL;
	}
	if (spank_get_item (sp, S_JOB_STEPID, &item) == ESPANK_SUCCESS) {
		app->job.step_id = (ulong) item;
	} else {
		app->job.step_id = NO_VAL;
	}
	if (!getenv_agnostic(sp, Var.name_app.rem, app->job.app_id, SZ_NAME_MEDIUM)) {
		strcpy(app->job.app_id, "");
	}
	if (!getenv_agnostic(sp, Var.policy.ear, app->job.policy, SZ_NAME_MEDIUM)) {
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
	if (!getenv_agnostic(sp, Var.tag.ear, app->job.energy_tag, 32)) {
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
	printenv_agnostic(sp, Component.monitor);
	printenv_agnostic(sp, Component.test);
	printenv_agnostic(sp, Component.verbose);
	printenv_agnostic(sp, Var.comp_libr.cmp);
	printenv_agnostic(sp, Var.comp_plug.cmp);
	printenv_agnostic(sp, Var.comp_moni.cmp);
	printenv_agnostic(sp, Var.comp_test.cmp);
	printenv_agnostic(sp, Var.comp_verb.cmp);
	printenv_agnostic(sp, Var.hack_libr.hck);

	printenv_agnostic(sp, Var.verbose.loc);
	printenv_agnostic(sp, Var.policy.loc);
	printenv_agnostic(sp, Var.policy_th.loc);
	printenv_agnostic(sp, Var.frequency.loc);
	printenv_agnostic(sp, Var.p_state.loc);
	printenv_agnostic(sp, Var.learning.loc);
	printenv_agnostic(sp, Var.tag.loc);
	printenv_agnostic(sp, Var.path_usdb.loc);
	printenv_agnostic(sp, Var.path_trac.loc);
	printenv_agnostic(sp, Var.version.loc);
	printenv_agnostic(sp, Var.gm_host.loc);
	printenv_agnostic(sp, Var.gm_port.loc);
	printenv_agnostic(sp, Var.gm_min.loc);
	printenv_agnostic(sp, Var.gm_secure.loc);
	
	printenv_agnostic(sp, Var.user.rem);
	printenv_agnostic(sp, Var.group.rem);
	printenv_agnostic(sp, Var.path_temp.rem);
	printenv_agnostic(sp, Var.path_inst.rem);
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

int plug_clean_components(spank_t sp)
{
	int test;

	plug_component_setenabled(sp, Component.plugin, 0);
	plug_component_setenabled(sp, Component.library, 0);
	plug_component_setenabled(sp, Component.monitor, 0);
	plug_component_setenabled(sp, Component.test, 0);

	/*
	 * Components
	 */
	if (valenv_agnostic(sp, Var.comp_test.cmp, &test)) {
		plug_component_setenabled(sp, Component.test, test);
	}
	if (test || !isenv_agnostic(sp, Var.comp_plug.cmp, "0")) {
		plug_component_setenabled(sp, Component.plugin, 1);
	}
	if (test || isenv_agnostic(sp, Var.comp_libr.cmp, "1")) {
		plug_component_setenabled(sp, Component.library, 1);
	}
	if (isenv_agnostic(sp, Var.comp_moni.cmp, "1")) {
		plug_component_setenabled(sp, Component.monitor, 1);
	}

	return ESPANK_SUCCESS;
}

int plug_deserialize_local(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_deserialize_local");

	// Silence
	VERB_SET_EN(0);
	ERROR_SET_EN(0);

	/*
	 * Components
	 */
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
	unsetenv_agnostic(sp, Var.path_trac.ear);
	unsetenv_agnostic(sp, Var.name_app.ear);

	// Exception (in testing)
	//unsetenv_agnostic(sp, Var.path_temp.ear);

	/*
	 * User information
	 */
	uid_t uid = geteuid();
	gid_t gid = getgid();
	struct passwd *upw = getpwuid(uid);
	struct group  *gpw = getgrgid(gid);

	if (upw == NULL || gpw == NULL) {
		plug_verbose(sp, 2, "converting UID/GID in username");
		return (ESPANK_ERROR);
	}

	strncpy(sd->job.user.user,  upw->pw_name, SZ_NAME_MEDIUM);
	strncpy(sd->job.user.group, gpw->gr_name, SZ_NAME_MEDIUM);
	plug_verbose(sp, 2, "user '%u' ('%s')", uid, sd->job.user.user);
	plug_verbose(sp, 2, "user group '%u' ('%s')", gid, sd->job.user.group);

	/*
	 * Subject
	 */
	gethostname(sd->subject.host, SZ_NAME_MEDIUM);

	return ESPANK_SUCCESS;
}

int plug_serialize_remote(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_serialize_remote");

	/*
	 * User
	 */
	setenv_agnostic(sp, Var.user.rem,  sd->job.user.user,  1);
	setenv_agnostic(sp, Var.group.rem, sd->job.user.group, 1);
	setenv_agnostic(sp, Var.path_temp.rem, sd->pack.path_temp, 1);
	setenv_agnostic(sp, Var.path_inst.rem, sd->pack.path_inst, 1);

	/*
	 * Subject
	 */

	if (plug_context_is(sp, Context.srun)) {
		setenv_agnostic(sp, Var.ctx_last.rem, Constring.srun, 1);
		setenv_agnostic(sp, Var.was_srun.rem, "1", 1);
	} else
	if (plug_context_is(sp, Context.sbatch)) {
		setenv_agnostic(sp, Var.ctx_last.rem, Constring.sbatch, 1);
		setenv_agnostic(sp, Var.was_sbac.rem, "1", 1);
	}

	/*
	 * EARGMD
	 */
	if (sd->pack.eargmd.enabled == 1)
	{
		sprintf(buffer1, "%d", sd->pack.eargmd.port);
		sprintf(buffer2, "%d", sd->pack.eargmd.min );
		
		setenv_agnostic(sp, Var.gm_host.loc, sd->pack.eargmd.host, 1);
		setenv_agnostic(sp, Var.gm_port.loc, buffer1             , 1);
		setenv_agnostic(sp, Var.gm_min.loc , buffer2             , 1);
	}

	return ESPANK_SUCCESS;
}

int plug_deserialize_remote(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_deserialize_remote");
	int s1, s2;

	// Silence
	VERB_SET_EN(0);
	ERROR_SET_EN(0);
	
	/*
	 * Options
	 */
	repenv_agnostic(sp, Var.verbose.loc, Var.verbose.ear);
	repenv_agnostic(sp, Var.policy.loc, Var.policy.ear);
	repenv_agnostic(sp, Var.policy_th.loc, Var.policy_th.ear);
	repenv_agnostic(sp, Var.frequency.loc, Var.frequency.ear);
	repenv_agnostic(sp, Var.learning.loc, Var.learning.ear);
	repenv_agnostic(sp, Var.tag.loc, Var.tag.ear);
	repenv_agnostic(sp, Var.path_usdb.loc, Var.path_usdb.ear);
	repenv_agnostic(sp, Var.path_trac.loc, Var.path_trac.ear);

	/*
	 * User
	 */
	getenv_agnostic(sp, Var.user.rem, sd->job.user.user, SZ_NAME_MEDIUM);
	getenv_agnostic(sp, Var.group.rem, sd->job.user.group, SZ_NAME_MEDIUM);
	getenv_agnostic(sp, Var.account.rem, sd->job.user.account, SZ_NAME_MEDIUM);
	getenv_agnostic(sp, Var.path_temp.rem, sd->pack.path_temp, SZ_PATH);
	getenv_agnostic(sp, Var.path_inst.rem, sd->pack.path_inst, SZ_PATH);

	/*
	 * Subject
	 */
	gethostname(sd->subject.host, SZ_NAME_MEDIUM);

	if (isenv_agnostic(sp, Var.ctx_last.rem, Constring.srun)) {
		sd->subject.context_local = Context.srun;
	} else if (isenv_agnostic(sp, Var.ctx_last.rem, Constring.sbatch)) {
		sd->subject.context_local = Context.sbatch;
	} else {
		sd->subject.context_local = Context.error;
	}

	/*
	 * Job/step
	 */
	s1 = 0;
	s2 = 0;

	if (plug_context_was(sd, Context.sbatch)) {
		s1 = getenv_agnostic(sp, Var.job_nodl.rem , buffer1, SZ_BUFF_EXTRA);
		s2 = getenv_agnostic(sp, Var.job_nodn.rem , buffer2, SZ_BUFF_EXTRA);
	} else if (plug_context_was(sd, Context.srun)) {
		s1 = getenv_agnostic(sp, Var.step_nodl.rem, buffer1, SZ_BUFF_EXTRA);
		s2 = getenv_agnostic(sp, Var.step_nodn.rem, buffer2, SZ_BUFF_EXTRA);
	}

	if (s1 && s2) {
		sd->job.node_n = atoi(buffer2);
	}

	/*
	 * Master
	 */
	hostlist_t p1;
	char *p2;

	if (plug_context_was(sd, Context.sbatch)) {
		sd->subject.is_master = 1;
		s1 = s2 = 0;
	}

	if (s1 && s2)
	{
		p1 = slurm_hostlist_create(buffer1);
		p2 = slurm_hostlist_shift(p1);

		if (p2 != NULL)
		{
			s1 = strlen(sd->subject.host);
			s2 = strlen(p2);

			if (s1 < s2) {
				s2 = s1;
			}

			sd->subject.is_master = (strncmp(sd->subject.host, p2, s2) == 0);
			plug_verbose(sp, 2, "first node from list of nodes '%s'", p2);
		}
	}

	plug_verbose(sp, 2, "subject '%s' is master? '%d'",
		sd->subject.host, sd->subject.is_master);

	/*
	 * EARMGD
	 */
	sd->pack.eargmd.min = 1;	

	s1 = getenv_agnostic(sp, Var.gm_host.loc, sd->pack.eargmd.host, SZ_NAME_MEDIUM);
	s2 = getenv_agnostic(sp, Var.gm_port.loc, buffer1             , SZ_NAME_MEDIUM);
	     getenv_agnostic(sp, Var.gm_min.loc , buffer2             , SZ_NAME_MEDIUM);

	if (s1 && s2) {
		sd->pack.eargmd.port = atoi(buffer1);
		sd->pack.eargmd.min  = atoi(buffer2);
		sd->pack.eargmd.enabled = 1;
	}

	/*
	 * The .ear variables are set during the APP serialization. But APP
	 * serialization happens if the library is ON. But now we have ERUN.
	 * With ERUN in mind we need a small set of variables definitions
	 * out of APP serialization.
	 */ 	
	repenv_agnostic(sp, Var.path_temp.rem, Var.path_temp.ear);

	/*
	 * Clean
	 */
	unsetenv_agnostic(sp, Var.user.rem);
	unsetenv_agnostic(sp, Var.group.rem);
	unsetenv_agnostic(sp, Var.path_temp.rem);
	unsetenv_agnostic(sp, Var.path_inst.rem);
	
	return ESPANK_SUCCESS;
}

int plug_serialize_task(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_serialize_task");

	/*
  	 * EAR variables
  	 */
	settings_conf_t *setts = &sd->pack.eard.setts;

	// Variable EAR_ENERGY_TAG, unset
	if (setts->user_type != ENERGY_TAG) {
		unsetenv_agnostic(sp, Var.tag.ear);
	}
	
	if (sd->pack.eard.connected && !setts->lib_enabled) {
		return ESPANK_SUCCESS;
	}

	snprintf(buffer1, 16, "%u", setts->def_p_state);
	setenv_agnostic(sp, Var.p_state.ear, buffer1, 1);

	snprintf(buffer1, 16, "%lu", setts->def_freq);
	setenv_agnostic(sp, Var.frequency.ear, buffer1, 1);

	/*if(policy_id_to_name(setts->policy, buffer1) != EAR_ERROR) {
		setenv_agnostic(sp, Var.policy.ear, buffer1, 1);
	}*/
	if (strlen(setts->policy_name) > 0) {
        	setenv_agnostic(sp, Var.policy.ear, setts->policy_name, 1);
	}

	snprintf(buffer1, 16, "%0.2f", setts->settings[0]);
	setenv_agnostic(sp, Var.eff_gain.ear, buffer1, 1);
	setenv_agnostic(sp, Var.perf_pen.ear, buffer1, 1);

	if(!setts->learning) {
		unsetenv_agnostic(sp, Var.p_state.ear);
		unsetenv_agnostic(sp, Var.learning.ear);
	}

	if (getenv_agnostic(sp, Var.name_app.rem, buffer1, sizeof(buffer1)) == 1) {
		setenv_agnostic(sp, Var.name_app.ear, buffer1, 1);
	}

	/*
	 * LD_PRELOAD
	 */
	#if !EAR_CORE
	char *lib_path = MPI_C_LIB_PATH;
	char ext1[64];
	char ext2[64];

	buffer1[0] = '\0';
	buffer2[0] = '\0';
	
	if(getenv_agnostic(sp, Var.version.loc, ext1, 64)) {
		snprintf(ext2, 64, "%s.so", ext1);
	} else {
		snprintf(ext2, 64, "so");
	}

	// Appending libraries to LD_PRELOAD
	apenv_agnostic(buffer2, sd->pack.path_inst, 64);

	#define t(max, val) \
		((val + 1) > max) ? max : val + 1

	//
	const int m = SZ_BUFF_EXTRA;
	int n;
	
	n = snprintf(      0,       0, "%s/%s.%s", buffer2, lib_path, ext2);
	n = snprintf(buffer1, t(m, n), "%s/%s.%s", buffer2, lib_path, ext2);

	if (file_is_regular(buffer1))
	{
		char *ld_buf = sd->job.user.env.ld_preload;

		if (getenv_agnostic(sp, Var.hack_libr.hck, buffer2, m)) {
			n = snprintf(      0,       0, "%s", buffer2);
			n = snprintf(buffer1, t(m, n), "%s", buffer2);
		}
		if (getenv_agnostic(sp, Var.ld_prel.ear, ld_buf, m))
		{
			if (n > 0) {
				n = sprintf(buffer2,    "%s:", buffer1);
			}

			n = snprintf(      0,       0, "%s%s", buffer2, ld_buf);
			n = snprintf(buffer1, t(m, n), "%s%s", buffer2, ld_buf);
		}

		setenv_agnostic(sp, Var.ld_prel.ear, buffer1, 1);
	}
	#endif	

	return ESPANK_SUCCESS;
}
