/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/types/application.h>
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
        plug_error(sp, "while reading services memory in '%s@%s'", sd->subject.host, buffer);
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
    int n_freqs  = 0;

    get_frequencies_path(sd->pack.path_temp, buffer);
    plug_verbose(sp, 3, "looking for frequencies in '%s'", buffer);
    freqs = attach_frequencies_shared_area(buffer, &n_freqs);

    if (freqs == NULL) {
        plug_error(sp, "while reading frequencies memory in node '%s@%s'", sd->subject.host, buffer);
        return ESPANK_ERROR;
    }

    sd->pack.eard.freqs.n_freqs = n_freqs / sizeof(ulong);
    sd->pack.eard.freqs.freqs   = malloc(n_freqs * sizeof(ulong));
    memcpy(sd->pack.eard.freqs.freqs, freqs, n_freqs * sizeof(ulong));

    dettach_frequencies_shared_area();

    return ESPANK_SUCCESS;
}

static int plug_print_settings(spank_t sp, plug_serialization_t *sd)
{
    settings_conf_t *setts = &sd->pack.eard.setts;

    plug_verbose(sp, 3, "--------------------------------- Settings summary ---");
    plug_verbose(sp, 3, "library.user type: %d.%d", setts->lib_enabled, setts->user_type);
    plug_verbose(sp, 3, "frequency.P_STATE: %lu.%u", setts->def_freq, setts->def_p_state);
    plug_verbose(sp, 3, "------------------------------------------------------");

    return ESPANK_SUCCESS;
}

int plug_shared_readsetts(spank_t sp, plug_serialization_t *sd)
{
    plug_verbose(sp, 2, "function plug_shared_readsetts");
    settings_conf_t *setts = NULL;

    // Opening settings
    /* This must be done after EARD contact */
    uint ID = create_ID(sd->job.app.job.id, sd->job.app.job.step_id);
    get_settings_conf_path(sd->pack.path_temp, ID, buffer);
    setts = attach_settings_conf_shared_area(buffer);
    plug_verbose(sp, 3, "looking for settings in '%s' (cid %u, job %lu, step %lu)", buffer, ID, sd->job.app.job.id,
                 sd->job.app.job.step_id);

    if (setts == NULL) {
        plug_error(sp, "while reading the settings memory in node '%s@%s'", sd->subject.host, buffer);
        return ESPANK_ERROR;
    }
    // It is OK, you can save the returned settings.
    memcpy(&sd->pack.eard.setts, setts, sizeof(settings_conf_t));
    // Closing shared memory
    dettach_settings_conf_shared_area();
    plug_print_settings(sp, sd);
    // If returned !lib_enabled, disable library component
    if (!setts->lib_enabled) {
        return plug_component_setenabled(sp, Component.library, 0);
    }

    return ESPANK_SUCCESS;
}
