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

#include <slurm_plugin/slurm_plugin_rcom.h>

// Externs
static char buffer[SZ_PATH];

int plug_rcom_eargmd_job_start(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eargmd_job_start");

	// Limit
	if (sd->job.node_n < sd->pack.eargmd.min) {
		plug_verbose(sp, 2, "EARGMD is not connected because not enough nodes (%d < %d)",
			sd->job.node_n, sd->pack.eargmd.min);
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
	if (sd->job.node_n == 0) {
		plug_error(sp, "while getting the node number '%u'", sd->job.node_n);
		return ESPANK_ERROR;
	}
	// Verbosity
	plug_verbose(sp, 2, "connecting EARGMD with host '%s', port '%u', and nnodes '%u'",
		sd->pack.eargmd.host, sd->pack.eargmd.port, sd->job.node_n);
	// Connection
	if (eargm_connect(sd->pack.eargmd.host, sd->pack.eargmd.port) < 0) {
		plug_error(sp, "while connecting with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	if (!eargm_new_job(sd->job.node_n)) {
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
                sd->pack.eargmd.host, sd->pack.eargmd.port, sd->job.node_n);

	if (eargm_connect(sd->pack.eargmd.host, sd->pack.eargmd.port) < 0) {
		plug_error(sp, "while connecting with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	if (!eargm_end_job(sd->job.node_n)) {
		plug_error(sp, "while speaking with EAR global manager daemon");
		return ESPANK_ERROR;
	}
	eargm_disconnect();

	// Disabling protection (hypercontained)
	setenv_agnostic(sp, Var.gm_secure.loc, "0", 1);

	return ESPANK_SUCCESS;
}
