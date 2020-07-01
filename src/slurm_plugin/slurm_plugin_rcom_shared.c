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

// Buffers
static char buffer[SZ_PATH];

int plug_shared_readservs(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_shared_readservs");
	services_conf_t *servs = NULL;

	get_services_conf_path(sd->pack.path_temp, buffer);
	servs = attach_services_conf_shared_area(buffer);
	plug_verbose(sp, 3, "looking for services in '%s'", buffer);

	if (servs == NULL) {
		plug_verbose(sp, 2, "while reading the shared services memory in '%s@%s'", sd->subject.host, buffer);
		return ESPANK_ERROR;
	}

	memcpy(&sd->pack.eard.servs, servs, sizeof(services_conf_t));
	sd->pack.eard.port = servs->eard.port;
	dettach_services_conf_shared_area();

	return ESPANK_SUCCESS;
}

int plug_shared_readfreqs(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_shared_readfreqs");
	ulong *freqs = NULL;
	int n_freqs = 0;

	get_frequencies_path(sd->pack.path_temp, buffer);
	plug_verbose(sp, 3, "looking for frequencies in '%s'", buffer);
	freqs = attach_frequencies_shared_area(buffer, &n_freqs);

	if (freqs == NULL) {
		plug_error(sp, "while reading the shared services memory in '%s'", "hostxxx");
		return ESPANK_ERROR;
	}

	sd->pack.eard.freqs.n_freqs = n_freqs / sizeof(ulong);
	sd->pack.eard.freqs.freqs = malloc(n_freqs * sizeof(ulong));
	memcpy(sd->pack.eard.freqs.freqs, freqs, n_freqs * sizeof(ulong));

	dettach_frequencies_shared_area();

	return ESPANK_SUCCESS;
}

static int plug_print_settings(spank_t sp, plug_serialization_t *sd)
{
        settings_conf_t *setts = &sd->pack.eard.setts;

        plug_verbose(sp, 3, "------------- Settings summary ---");
        plug_verbose(sp, 3, "library/user type '%d'/'%d'", setts->lib_enabled, setts->user_type);
        plug_verbose(sp, 3, "freq/P_STATE '%lu'/'%u'", setts->def_freq, setts->def_p_state);
        plug_verbose(sp, 3, "----------------------------------");

	return ESPANK_SUCCESS;
}

int plug_shared_readsetts(spank_t sp, plug_serialization_t *sd)
{
	plug_verbose(sp, 2, "function plug_shared_readsetts");
	settings_conf_t *setts = NULL;

	// Opening settings
	get_settings_conf_path(sd->pack.path_temp, buffer);
	setts = attach_settings_conf_shared_area(buffer);
	plug_verbose(sp, 3, "looking for services in '%s'", buffer);

	if (setts == NULL) {
		plug_error(sp, "while reading the shared configuration memory in node '%s'", "hostxxx");
		return ESPANK_ERROR;
	}

	memcpy(&sd->pack.eard.setts, setts, sizeof(settings_conf_t));

	// Closing shared memory
	dettach_settings_conf_shared_area();

	plug_print_settings(sp, sd);

	return ESPANK_SUCCESS;
}
