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

static int plug_rcom_eard(spank_t sp, plug_serialization_t *sd, int new_job)
{
	int port   = sd->pack.eard.servs.eard.port;
	char *node = sd->subject.host;

	// Hostlist get
	sd->pack.eard.connected = 1;

	//
	plug_verbose(sp, 2, "connecting to EARD: '%s:%d'", node, port);

	if (eards_remote_connect(node, port) < 0) {
		plug_error(sp, "while connecting with EAR daemon");
		sd->pack.eard.connected = 0;
		return ESPANK_ERROR;
	}
	if (new_job) {
		eards_new_job(&sd->job.app);
		plug_print_application(sp, &sd->job.app);
	} else {
		eards_end_job(sd->job.app.job.id, sd->job.app.job.step_id);
	}
	eards_remote_disconnect();

	return ESPANK_SUCCESS;
}


int plug_rcom_eard_job_start(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eard_job_start");
	return plug_rcom_eard(sp, sd, 1);
}

int plug_rcom_eard_job_finish(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_rcom_eard_job_finish");
	return plug_rcom_eard(sp, sd, 0);
}
