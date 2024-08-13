/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
	if (getenv_agnostic(sp, Var.gm_secure.mod, buffer, SZ_PATH)) {
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
	if (remote_connect(sd->pack.eargmd.host, sd->pack.eargmd.port) < 0) {
		plug_error(sp, "while connecting with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	if (!eargm_new_job(sd->job.nodes_count)) {
		plug_error(sp, "while speaking with EAR global manager daemon");
	}
	remote_disconnect();

	// Informing that this report has to be finished
	sd->pack.eargmd.connected = 1;
	// Enabling protection (hypercontained)
	setenv_agnostic(sp, Var.gm_secure.mod, "1", 1);

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

	if (remote_connect(sd->pack.eargmd.host, sd->pack.eargmd.port) < 0) {
		plug_error(sp, "while connecting with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	if (!eargm_end_job(sd->job.nodes_count)) {
		plug_error(sp, "while speaking with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	remote_disconnect();

	// Disabling protection (hypercontained)
	setenv_agnostic(sp, Var.gm_secure.mod, "0", 1);

	return ESPANK_SUCCESS;
}
