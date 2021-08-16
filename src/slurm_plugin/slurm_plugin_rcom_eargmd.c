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

// Externs
static char buffer[SZ_PATH];

int plug_rcom_eargmd_job_start(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eargmd_job_start");

	// Limit
	if (sd->job.nodes_count < sd->pack.eargmd.min) {
		plug_verbose(sp, 2, "EARGMD is not connected because not enough nodes (%d < %d)",
			sd->job.nodes_count, sd->pack.eargmd.min);
		return ESPANK_SUCCESS;
	}
	// Pack deserialization
	if (getenv_agnostic(sp, Var.gm_secure.loc, buffer, SZ_PATH)) {
		sd->pack.eargmd.secured = atoi(buffer);
	}
	//
	if (!sd->pack.eargmd.enabled || sd->pack.eargmd.secured) {
		plug_error(sp, "connection with EARGMD not enabled or secured (%d,%d)",
			sd->pack.eargmd.enabled, sd->pack.eargmd.secured);
		return ESPANK_ERROR;
	}
	//
	if (sd->job.nodes_count == 0) {
		plug_error(sp, "while getting the node number '%u'", sd->job.nodes_count);
		return ESPANK_ERROR;
	}
	// Verbosity
	plug_verbose(sp, 2, "connecting EARGMD with host '%s', port '%u', and nnodes '%u'",
		sd->pack.eargmd.host, sd->pack.eargmd.port, sd->job.nodes_count);
	// Connection
	if (eargm_connect(sd->pack.eargmd.host, sd->pack.eargmd.port) < 0) {
		plug_error(sp, "while connecting with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	if (!eargm_new_job(sd->job.nodes_count)) {
		plug_error(sp, "while speaking with EAR global manager daemon");
	}
	eargm_disconnect();

	// Informing that this report has to be finished
	sd->pack.eargmd.connected = 1;
	// Enabling protection (hypercontained)
	setenv_agnostic(sp, Var.gm_secure.loc, "1", 1);

	return ESPANK_SUCCESS;
}

int plug_rcom_eargmd_job_finish(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eargmd_job_finish");

	if (!sd->pack.eargmd.connected) {
		return ESPANK_SUCCESS;
	}

	plug_verbose(sp, 2, "trying to disconnect EARGMD with host '%s', port '%u', and nnodes '%u'",
                sd->pack.eargmd.host, sd->pack.eargmd.port, sd->job.nodes_count);

	if (eargm_connect(sd->pack.eargmd.host, sd->pack.eargmd.port) < 0) {
		plug_error(sp, "while connecting with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	if (!eargm_end_job(sd->job.nodes_count)) {
		plug_error(sp, "while speaking with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	eargm_disconnect();

	// Disabling protection (hypercontained)
	setenv_agnostic(sp, Var.gm_secure.loc, "0", 1);

	return ESPANK_SUCCESS;
}
