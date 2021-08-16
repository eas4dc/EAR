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

#include <slurm_plugin/slurm_plugin_rcom.h>

static int plug_rcom_eard_sbatch(spank_t sp, plug_serialization_t *sd, int new_job)
{
	plug_verbose(sp, 2, "function plug_rcom_eard_sbatch");

	char *net_ext = sd->pack.eard.servs.net_ext;
	int port = sd->pack.eard.servs.eard.port;
	int i = 0;

	if (sd->job.nodes_count == 0) {
		plug_error(sp, "nodes counter is 0");
		return ESPANK_ERROR;
	}
	while (i < sd->job.nodes_count) {
		plug_verbose(sp, 2, "connecting to EARD %d/%d: '%s:%d' (by SBATCH/SALLOC)",
			i, sd->job.nodes_count-1, sd->job.nodes_list[i], port);
		++i;
	}
	if (new_job) {
		eards_new_job_nodelist(&sd->job.app, port, net_ext,
			sd->job.nodes_list, sd->job.nodes_count);
	} else {
		eards_end_job_nodelist(sd->job.app.job.id, sd->job.app.job.step_id, port, net_ext,
			sd->job.nodes_list, sd->job.nodes_count);
	}
	return ESPANK_SUCCESS;
}

static int plug_rcom_eard(spank_t sp, plug_serialization_t *sd, int new_job)
{
	int port = sd->pack.eard.servs.eard.port;
	char *node = sd->subject.host;

	// Hostlist get
	sd->pack.eard.connected = 1;
	//
	if (eards_remote_connect(node, port) < 0) {
		plug_verbose(sp, 2, "connecting to EARD: '%s:%d': FAILED (by SRUN)", node, port);
		sd->pack.eard.connected = 0;
		return ESPANK_ERROR;
	}
	if (new_job) {
		eards_new_job(&sd->job.app);
		plug_print_application(sp, &sd->job.app);
	} else {
		eards_end_job(sd->job.app.job.id, sd->job.app.job.step_id);
	}
	plug_verbose(sp, 2, "connecting to EARD: '%s:%d': OK (by SRUN)", node, port);
	eards_remote_disconnect();

	return ESPANK_SUCCESS;
}

int plug_rcom_eard_job_start(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eard_job_start");
	if (plug_context_was(sd, Context.srun)) {
		return plug_rcom_eard(sp, sd, 1);
	} else {
		return plug_rcom_eard_sbatch(sp, sd, 1);
	}
}

int plug_rcom_eard_job_finish(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eard_job_finish");
	if (plug_context_was(sd, Context.srun)) {
		return plug_rcom_eard(sp, sd, 0);
	} else {
		return plug_rcom_eard_sbatch(sp, sd, 0);
	}
}
